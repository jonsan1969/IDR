#pragma once

#include "IdrControlFlow.h"

#include <vector>

namespace idr::core {

// Neutral procedure-level analysis payload. This deliberately describes only
// facts observed by the portable CFG. It is not legacy InfoRec/procInfo and
// observedSpan is not asserted to be legacy procSize.
struct ProcedureAnalysisInput {
    ProcedureSummary summary;
    std::vector<TraceInstruction> instructions;
    std::vector<BasicBlockTrace> blocks;
    std::vector<ControlFlowEdge> edges;
    std::vector<CallXref> incomingCalls;
    std::vector<CallXref> outgoingCalls;
};

inline bool BuildProcedureAnalysisInput(const ControlFlowResult &result,
                                        DWord procedureAddress,
                                        ProcedureAnalysisInput &input) {
    input = {};

    const ProcedureSummary *summary = FindProcedureSummary(result, procedureAddress);
    if (!summary) return false;

    const std::vector<TraceInstruction> *instructions = nullptr;
    const std::vector<BasicBlockTrace> *blocks = nullptr;

    if (!result.procedures.empty() && result.procedures.front().address == procedureAddress) {
        instructions = &result.entryTrace;
        blocks = &result.entryBlocks;
    } else {
        for (const auto &candidate : result.candidates) {
            if (candidate.address != procedureAddress) continue;
            instructions = &candidate.instructions;
            blocks = &candidate.blocks;
            break;
        }
    }

    if (!instructions || !blocks) return false;
    if (instructions->size() != summary->instructionCount ||
        blocks->size() != summary->blockCount)
        return false;

    input.summary = *summary;
    input.instructions = *instructions;
    input.blocks = *blocks;
    input.edges = FindProcedureEdges(result, procedureAddress);
    input.incomingCalls = FindIncomingCallXrefs(result, procedureAddress);
    input.outgoingCalls = FindOutgoingCallXrefs(result, procedureAddress);
    return true;
}

} // namespace idr::core
