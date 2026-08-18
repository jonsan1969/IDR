#pragma once

#include "IdrAnalysisState.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace idr::core {

struct DecodedInstruction {
    int length = 0;
    std::string mnemonic;
    std::string disasm;
    bool call = false;
    bool branch = false;
    bool conditional = false;
    bool ret = false;
    DWord target = 0;
};

using InstructionDecoder = std::function<bool(DWord, DecodedInstruction &)>;
using AddressMapper = std::function<int(DWord)>;

struct TraceInstruction {
    DWord address = 0;
    int length = 0;
    std::string mnemonic;
    std::string disasm;
};

enum class ControlFlowEdgeKind {
    Call,
    BranchTaken,
    FallThrough,
};

struct ControlFlowEdge {
    DWord procedure = 0;
    DWord from = 0;
    DWord to = 0;
    ControlFlowEdgeKind kind = ControlFlowEdgeKind::BranchTaken;
};

struct CallXref {
    DWord caller = 0;
    DWord callSite = 0;
    DWord callee = 0;
};

struct BasicBlockTrace {
    DWord address = 0;
    std::vector<TraceInstruction> instructions;
};

struct ProcedureTrace {
    DWord address = 0;
    std::vector<TraceInstruction> instructions;
    std::vector<BasicBlockTrace> blocks;
};

struct ProcedureSummary {
    DWord address = 0;
    std::size_t blockCount = 0;
    std::size_t instructionCount = 0;
    std::size_t callEdgeCount = 0;
    std::size_t branchTakenEdgeCount = 0;
    std::size_t fallThroughEdgeCount = 0;
    std::size_t incomingCallCount = 0;
    DWord observedStart = 0;
    DWord observedEndExclusive = 0;
    std::size_t observedSpan = 0;
};

struct ControlFlowOptions {
    std::size_t traceLimit = 8;
    std::size_t candidateLimit = 8;
    std::size_t blockLimit = 8;
};

struct ControlFlowResult {
    std::vector<TraceInstruction> entryTrace;
    std::vector<BasicBlockTrace> entryBlocks;
    std::vector<ControlFlowEdge> edges;
    std::vector<CallXref> callXrefs;
    std::vector<ProcedureTrace> candidates;
    std::vector<ProcedureSummary> procedures;
    std::size_t discoveredCandidateCount = 0;
    std::size_t discoveredBlockCount = 0;
    std::string error;
};

inline const ProcedureSummary *FindProcedureSummary(const ControlFlowResult &result, DWord address) {
    for (const auto &procedure : result.procedures) {
        if (procedure.address == address) return &procedure;
    }
    return nullptr;
}

inline std::vector<CallXref> FindIncomingCallXrefs(const ControlFlowResult &result, DWord callee) {
    std::vector<CallXref> matches;
    for (const auto &xref : result.callXrefs) {
        if (xref.callee == callee) matches.push_back(xref);
    }
    return matches;
}

inline std::vector<CallXref> FindOutgoingCallXrefs(const ControlFlowResult &result, DWord caller) {
    std::vector<CallXref> matches;
    for (const auto &xref : result.callXrefs) {
        if (xref.caller == caller) matches.push_back(xref);
    }
    return matches;
}

inline std::vector<ControlFlowEdge> FindProcedureEdges(const ControlFlowResult &result, DWord procedureAddress) {
    std::vector<ControlFlowEdge> matches;
    for (const auto &edge : result.edges) {
        if (edge.procedure == procedureAddress) matches.push_back(edge);
    }
    return matches;
}

inline bool AnalyzeBoundedControlFlow(DWord entryPoint,
                                      AnalysisState &analysis,
                                      const InstructionDecoder &decoder,
                                      const AddressMapper &addressToOffset,
                                      const ControlFlowOptions &options,
                                      ControlFlowResult &result) {
    result = {};
    const auto fail = [&](const char *message) {
        result.error = message;
        return false;
    };
    if (!decoder) return fail("decoder is not available");
    if (!addressToOffset) return fail("address mapper is not available");
    if (options.traceLimit == 0) return fail("trace limit is zero");
    if (options.blockLimit == 0) return fail("block limit is zero");

    std::deque<DWord> candidateQueue;
    std::unordered_set<DWord> seenCandidates;
    seenCandidates.insert(entryPoint);

    const auto enqueueCandidate = [&](DWord address) {
        if (result.discoveredCandidateCount >= options.candidateLimit) return;
        if (addressToOffset(address) < 0) return;
        if (!seenCandidates.insert(address).second) return;
        candidateQueue.push_back(address);
        ++result.discoveredCandidateCount;
    };

    const auto analyzeProcedure = [&](DWord procedureAddress,
                                      std::vector<TraceInstruction> &flatTrace,
                                      std::vector<BasicBlockTrace> &blocks) {
        std::deque<DWord> blockQueue;
        std::unordered_set<DWord> seenBlocks;
        std::unordered_set<DWord> decodedAddresses;

        const auto enqueueBlock = [&](DWord address) {
            if (seenBlocks.size() >= options.blockLimit) return;
            if (addressToOffset(address) < 0) return;
            if (!seenBlocks.insert(address).second) return;
            blockQueue.push_back(address);
            ++result.discoveredBlockCount;
        };

        enqueueBlock(procedureAddress);

        while (!blockQueue.empty() && blocks.size() < options.blockLimit) {
            const DWord blockAddress = blockQueue.front();
            blockQueue.pop_front();

            BasicBlockTrace block;
            block.address = blockAddress;
            DWord address = blockAddress;

            for (std::size_t i = 0; i < options.traceLimit; ++i) {
                if (address != blockAddress && seenBlocks.find(address) != seenBlocks.end()) break;
                if (!decodedAddresses.insert(address).second) break;

                const int offset = addressToOffset(address);
                if (offset < 0) break;

                DecodedInstruction decoded;
                if (!decoder(address, decoded) || decoded.length <= 0 || decoded.mnemonic.empty()) break;

                const auto pos = static_cast<std::size_t>(offset);
                if (!analysis.SetFlag(CodeFlags::Instruction, pos) ||
                    !analysis.SetFlags(CodeFlags::Code, pos, static_cast<std::size_t>(decoded.length)))
                    return fail("cannot mark decoded instruction state");
                if (decoded.call && !analysis.SetFlag(CodeFlags::Call, pos))
                    return fail("cannot mark call instruction state");

                const TraceInstruction traced{address, decoded.length, decoded.mnemonic, decoded.disasm};
                block.instructions.push_back(traced);
                flatTrace.push_back(traced);

                if ((decoded.call || decoded.branch) && decoded.target != 0) {
                    const int targetOffset = addressToOffset(decoded.target);
                    if (targetOffset >= 0) {
                        if (!analysis.SetFlag(CodeFlags::Loc, static_cast<std::size_t>(targetOffset)))
                            return fail("cannot mark control-flow target state");
                        result.edges.push_back({procedureAddress, address, decoded.target,
                                                decoded.call ? ControlFlowEdgeKind::Call
                                                             : ControlFlowEdgeKind::BranchTaken});
                        if (decoded.call) {
                            result.callXrefs.push_back({procedureAddress, address, decoded.target});
                            enqueueCandidate(decoded.target);
                        } else {
                            enqueueBlock(decoded.target);
                        }
                    }
                }

                if (decoded.ret) break;

                if (static_cast<unsigned int>(decoded.length) >
                    (std::numeric_limits<DWord>::max)() - address)
                    break;
                const DWord fallThrough = address + static_cast<DWord>(decoded.length);

                if (decoded.branch) {
                    if (decoded.conditional && addressToOffset(fallThrough) >= 0) {
                        result.edges.push_back({procedureAddress, address, fallThrough,
                                                ControlFlowEdgeKind::FallThrough});
                        enqueueBlock(fallThrough);
                    }
                    break;
                }

                address = fallThrough;
            }

            if (!block.instructions.empty()) blocks.push_back(std::move(block));
        }

        if (flatTrace.empty()) return false;
        const int procedureOffset = addressToOffset(procedureAddress);
        if (procedureOffset < 0 ||
            !analysis.SetFlag(CodeFlags::ProcStart, static_cast<std::size_t>(procedureOffset)))
            return fail("cannot mark procedure start state");
        return true;
    };

    if (!analyzeProcedure(entryPoint, result.entryTrace, result.entryBlocks))
        return result.error.empty() ? fail("entry trace produced no instructions") : false;

    while (!candidateQueue.empty() && result.candidates.size() < options.candidateLimit) {
        ProcedureTrace candidate;
        candidate.address = candidateQueue.front();
        candidateQueue.pop_front();
        if (!analyzeProcedure(candidate.address, candidate.instructions, candidate.blocks))
            return result.error.empty() ? fail("procedure candidate trace produced no instructions") : false;
        result.candidates.push_back(std::move(candidate));
    }

    const auto appendSummary = [&](DWord address,
                                   const std::vector<BasicBlockTrace> &blocks,
                                   const std::vector<TraceInstruction> &instructions) {
        ProcedureSummary summary;
        summary.address = address;
        summary.blockCount = blocks.size();
        summary.instructionCount = instructions.size();
        if (!instructions.empty()) {
            DWord observedStart = (std::numeric_limits<DWord>::max)();
            DWord observedEndExclusive = 0;
            for (const auto &instruction : instructions) {
                if (instruction.address < observedStart) observedStart = instruction.address;
                if (instruction.length <= 0) continue;
                const auto length = static_cast<DWord>(instruction.length);
                if (length > (std::numeric_limits<DWord>::max)() - instruction.address) continue;
                const DWord endExclusive = instruction.address + length;
                if (endExclusive > observedEndExclusive) observedEndExclusive = endExclusive;
            }
            if (observedStart != (std::numeric_limits<DWord>::max)() && observedEndExclusive >= observedStart) {
                summary.observedStart = observedStart;
                summary.observedEndExclusive = observedEndExclusive;
                summary.observedSpan = static_cast<std::size_t>(observedEndExclusive - observedStart);
            }
        }
        for (const auto &edge : result.edges) {
            if (edge.procedure != address) continue;
            switch (edge.kind) {
                case ControlFlowEdgeKind::Call: ++summary.callEdgeCount; break;
                case ControlFlowEdgeKind::BranchTaken: ++summary.branchTakenEdgeCount; break;
                case ControlFlowEdgeKind::FallThrough: ++summary.fallThroughEdgeCount; break;
            }
        }
        for (const auto &xref : result.callXrefs) {
            if (xref.callee == address) ++summary.incomingCallCount;
        }
        result.procedures.push_back(summary);
    };

    appendSummary(entryPoint, result.entryBlocks, result.entryTrace);
    for (const auto &candidate : result.candidates)
        appendSummary(candidate.address, candidate.blocks, candidate.instructions);

    return true;
}

} // namespace idr::core
