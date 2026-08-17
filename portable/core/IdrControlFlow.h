#pragma once

#include "IdrAnalysisState.h"
#include "IdrImageContext.h"

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

struct TraceInstruction {
    DWord address = 0;
    int length = 0;
    std::string mnemonic;
    std::string disasm;
};

struct ControlFlowEdge {
    DWord from = 0;
    DWord to = 0;
    bool call = false;
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

struct ControlFlowOptions {
    std::size_t traceLimit = 8;
    std::size_t candidateLimit = 8;
    std::size_t blockLimit = 8;
};

struct ControlFlowResult {
    std::vector<TraceInstruction> entryTrace;
    std::vector<BasicBlockTrace> entryBlocks;
    std::vector<ControlFlowEdge> edges;
    std::vector<ProcedureTrace> candidates;
    std::size_t discoveredCandidateCount = 0;
    std::size_t discoveredBlockCount = 0;
    std::string error;
};

inline bool AnalyzeBoundedControlFlow(DWord entryPoint,
                                      AnalysisState &analysis,
                                      const InstructionDecoder &decoder,
                                      const ControlFlowOptions &options,
                                      ControlFlowResult &result) {
    result = {};
    const auto fail = [&](const char *message) {
        result.error = message;
        return false;
    };
    if (!decoder) return fail("decoder is not available");
    if (options.traceLimit == 0) return fail("trace limit is zero");
    if (options.blockLimit == 0) return fail("block limit is zero");

    std::deque<DWord> candidateQueue;
    std::unordered_set<DWord> seenCandidates;
    seenCandidates.insert(entryPoint);

    const auto enqueueCandidate = [&](DWord address) {
        if (result.discoveredCandidateCount >= options.candidateLimit) return;
        if (AddressToOffset(address) < 0) return;
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
            if (AddressToOffset(address) < 0) return;
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

                const int offset = AddressToOffset(address);
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
                    const int targetOffset = AddressToOffset(decoded.target);
                    if (targetOffset >= 0) {
                        if (!analysis.SetFlag(CodeFlags::Loc, static_cast<std::size_t>(targetOffset)))
                            return fail("cannot mark control-flow target state");
                        result.edges.push_back({address, decoded.target, decoded.call});
                        if (decoded.call)
                            enqueueCandidate(decoded.target);
                        else
                            enqueueBlock(decoded.target);
                    }
                }

                if (decoded.ret) break;

                if (static_cast<unsigned int>(decoded.length) >
                    (std::numeric_limits<DWord>::max)() - address)
                    break;
                const DWord fallThrough = address + static_cast<DWord>(decoded.length);

                if (decoded.branch) {
                    if (decoded.conditional) enqueueBlock(fallThrough);
                    break;
                }

                address = fallThrough;
            }

            if (!block.instructions.empty()) blocks.push_back(std::move(block));
        }

        if (flatTrace.empty()) return false;
        const int procedureOffset = AddressToOffset(procedureAddress);
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

    return true;
}

} // namespace idr::core
