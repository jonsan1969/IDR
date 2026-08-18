#pragma once

#include "IdrHeadlessPrototypePolicy.h"

#include <functional>
#include <utility>
#include <vector>

namespace idr::core {

struct ProcedureCalleePrototype {
    DWord address = 0;
    ProcedurePrototypeMetadata prototype;
};

struct ProcedureDecompileInput {
    ProcedureAnalysisInput analysis;
    ProcedurePrototypeMetadata prototype;
    std::vector<ProcedureCalleePrototype> callees;
};

using ProcedurePrototypeLookup =
    std::function<bool(DWord, ProcedurePrototypeMetadata &)>;

// Build one decompiler-facing read model from portable CFG observations and a
// caller-supplied prototype lookup. Complete prototypes pass through untouched;
// incomplete prototypes may be supplied by the explicit headless resolver.
// No legacy object is mutated here and legacy procSize is deliberately absent.
inline bool BuildProcedureDecompileInput(const ControlFlowResult &flow,
                                         DWord procedureAddress,
                                         Byte functionKind,
                                         const ProcedurePrototypeLookup &lookup,
                                         const HeadlessPrototypeResolver &resolver,
                                         ProcedureDecompileInput &input) {
    input = {};
    if (!lookup) return false;
    if (!BuildProcedureAnalysisInput(flow, procedureAddress, input.analysis)) return false;

    ProcedurePrototypeMetadata currentPrototype;
    if (!lookup(procedureAddress, currentPrototype)) {
        input = {};
        return false;
    }

    PrototypeResolutionRequest currentRequest;
    currentRequest.procedureAddress = procedureAddress;
    currentRequest.callSite = 0;
    currentRequest.current = std::move(currentPrototype);
    if (!ResolveProcedurePrototype(currentRequest, functionKind, resolver, input.prototype)) {
        input = {};
        return false;
    }

    for (const auto &call : input.analysis.outgoingCalls) {
        bool alreadyCaptured = false;
        for (const auto &callee : input.callees) {
            if (callee.address == call.callee) {
                alreadyCaptured = true;
                break;
            }
        }
        if (alreadyCaptured) continue;

        ProcedurePrototypeMetadata prototype;
        if (!lookup(call.callee, prototype)) {
            input = {};
            return false;
        }

        PrototypeResolutionRequest calleeRequest;
        calleeRequest.procedureAddress = call.callee;
        calleeRequest.callSite = call.callSite;
        calleeRequest.current = std::move(prototype);

        ProcedurePrototypeMetadata resolvedPrototype;
        if (!ResolveProcedurePrototype(calleeRequest, functionKind, resolver, resolvedPrototype)) {
            input = {};
            return false;
        }
        input.callees.push_back({call.callee, std::move(resolvedPrototype)});
    }

    return true;
}

} // namespace idr::core
