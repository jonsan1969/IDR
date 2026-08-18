#pragma once

#include "IdrLegacyProcedureAdapter.h"

#include <vector>

namespace idr::core {

// Neutral, read-only package of the facts required before legacy procedure
// decompilation may begin. The current procedure carries its CFG observations
// and prototype metadata; direct callees are represented once per address so
// callers do not need to query legacy InfoRec state while walking call sites.
// legacy procSize is deliberately absent from this boundary.
struct ProcedureCalleePrototype {
    DWord address = 0;
    ProcedurePrototypeMetadata prototype;
};

struct ProcedureDecompileInput {
    ProcedureAnalysisInput analysis;
    ProcedurePrototypeMetadata prototype;
    std::vector<ProcedureCalleePrototype> callees;
};

inline bool CaptureActiveLegacyProcedurePrototype(DWord address,
                                                  ProcedurePrototypeMetadata &metadata) {
    metadata = {};
    const auto session = GetLegacyImageSessionView();
    if (!Infos || !session.infos ||
        session.infos != reinterpret_cast<void *const *>(Infos))
        return false;

    const int offset = AddressToOffset(address);
    if (offset < 0) return false;
    const auto pos = static_cast<std::size_t>(offset);
    if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;
    if (!Infos[pos]) return false;

    return CaptureLegacyProcedurePrototypeMetadata(*Infos[pos], metadata);
}

// Build one decompiler-facing read model from portable CFG observations and
// the already-reconciled active legacy metadata. Prototype completeness is
// enforced for the current procedure and every unique direct callee. Missing
// or incomplete metadata is a hard failure here; a future headless resolver
// can decide how to supply such metadata without falling back to GUI input.
inline bool BuildProcedureDecompileInputFromActiveLegacySession(
    const ControlFlowResult &flow,
    DWord procedureAddress,
    Byte functionKind,
    ProcedureDecompileInput &input) {
    input = {};

    if (!BuildProcedureAnalysisInput(flow, procedureAddress, input.analysis)) return false;
    if (!CaptureActiveLegacyProcedurePrototype(procedureAddress, input.prototype)) {
        input = {};
        return false;
    }
    if (!IsProcedurePrototypeComplete(input.prototype, functionKind)) {
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
        if (!CaptureActiveLegacyProcedurePrototype(call.callee, prototype) ||
            !IsProcedurePrototypeComplete(prototype, functionKind)) {
            input = {};
            return false;
        }
        input.callees.push_back({call.callee, std::move(prototype)});
    }

    return true;
}

} // namespace idr::core
