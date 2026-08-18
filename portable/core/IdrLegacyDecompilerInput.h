#pragma once

#include "IdrDecompilerInput.h"
#include "IdrLegacyProcedureAdapter.h"

namespace idr::core {

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

// Active-session adapter around the neutral decompiler-input builder. Legacy
// Infos[] is only a prototype source; headless resolution remains read-time and
// does not mutate the backing InfoRec/InfoProcInfo state.
inline bool BuildProcedureDecompileInputFromActiveLegacySession(
    const ControlFlowResult &flow,
    DWord procedureAddress,
    Byte functionKind,
    ProcedureDecompileInput &input,
    const HeadlessPrototypeResolver &resolver = {}) {
    const ProcedurePrototypeLookup lookup = [](DWord address,
                                                ProcedurePrototypeMetadata &metadata) {
        return CaptureActiveLegacyProcedurePrototype(address, metadata);
    };
    return BuildProcedureDecompileInput(flow, procedureAddress, functionKind,
                                        lookup, resolver, input);
}

} // namespace idr::core
