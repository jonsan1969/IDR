#pragma once

#include "IdrControlFlow.h"

#include <string>
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

// Minimal neutral prototype/stack metadata read by the legacy decompiler
// before it can begin procedure decompilation. This is intentionally smaller
// than InfoRec/InfoProcInfo and does not contain legacy procSize.
struct ProcedureArgumentMetadata {
    Byte tag = 0;
    bool inRegister = false;
    int index = 0;
    int size = 0;
    std::string name;
    std::string type;
};

struct ProcedureLocalMetadata {
    int offset = 0;
    int size = 0;
    std::string name;
    std::string type;
};

struct ProcedurePrototypeMetadata {
    Byte kind = 0;
    std::string returnType;
    DWord flags = 0;
    Word bpBase = 0;
    Word retBytes = 0;
    int stackSize = 0;
    std::vector<ProcedureArgumentMetadata> arguments;
    std::vector<ProcedureLocalMetadata> locals;
};

inline bool IsProcedurePrototypeComplete(const ProcedurePrototypeMetadata &metadata, Byte functionKind) {
    for (const auto &argument : metadata.arguments) {
        if (argument.type.empty()) return false;
    }
    if (metadata.kind == functionKind && metadata.returnType.empty()) return false;
    return true;
}

// Explicit value-only bridge toward the legacy InfoRec/InfoProcInfo surface.
// These field names intentionally mirror the legacy concepts, but no legacy
// object is allocated or mutated here. procSize remains deliberately absent.
struct LegacyProcedureArgumentSeed {
    Byte tag = 0;
    bool registerArgument = false;
    int ndx = 0;
    int size = 0;
    std::string name;
    std::string typeDef;
};

struct LegacyProcedureLocalSeed {
    int ofs = 0;
    int size = 0;
    std::string name;
    std::string typeDef;
};

struct LegacyProcedureMetadataSeed {
    Byte kind = 0;
    std::string returnType;
    DWord flags = 0;
    Word bpBase = 0;
    Word retBytes = 0;
    int stackSize = 0;
    std::vector<LegacyProcedureArgumentSeed> arguments;
    std::vector<LegacyProcedureLocalSeed> locals;
};

inline bool BuildLegacyProcedureMetadataSeed(const ProcedurePrototypeMetadata &metadata,
                                             Byte functionKind,
                                             LegacyProcedureMetadataSeed &seed) {
    seed = {};
    if (!IsProcedurePrototypeComplete(metadata, functionKind)) return false;

    seed.kind = metadata.kind;
    seed.returnType = metadata.returnType;
    seed.flags = metadata.flags;
    seed.bpBase = metadata.bpBase;
    seed.retBytes = metadata.retBytes;
    seed.stackSize = metadata.stackSize;

    seed.arguments.reserve(metadata.arguments.size());
    for (const auto &argument : metadata.arguments) {
        seed.arguments.push_back({argument.tag,
                                  argument.inRegister,
                                  argument.index,
                                  argument.size,
                                  argument.name,
                                  argument.type});
    }

    seed.locals.reserve(metadata.locals.size());
    for (const auto &local : metadata.locals) {
        seed.locals.push_back({local.offset, local.size, local.name, local.type});
    }
    return true;
}

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
