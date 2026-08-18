#include "IdrLegacyDecompilerRunner.h"

#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrLegacyProcedureAdapter.h"
#include "Decompiler.portable.h"

namespace idr::core {

bool PreflightActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerPreflightResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver) {
    result = {};

    const auto session = GetLegacyImageSessionView();
    if (!Infos || !session.infos ||
        session.infos != reinterpret_cast<void *const *>(Infos))
        return false;

    const int offset = AddressToOffset(address);
    if (offset < 0) return false;
    const auto pos = static_cast<std::size_t>(offset);
    if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;

    PInfoRec record = Infos[pos];
    if (!record || record->kind < ikRefine || record->kind > ikFunc || !record->procInfo)
        return false;

    ProcedureSizeResolutionRequest sizeRequest;
    sizeRequest.procedureAddress = address;
    sizeRequest.storedSize = record->procInfo->procSize;
    ResolvedProcedureSize resolvedSize;
    const auto &effectiveResolver = sizeResolver ? sizeResolver : LegacyProcedureSizeResolver();
    if (!ResolveProcedureSize(sizeRequest, effectiveResolver, resolvedSize)) return false;

    TDecompileEnv environment(address, resolvedSize.size, record);
    TDecompiler decompiler(&environment);
    if (!decompiler.Init(address)) return false;
    decompiler.InitFlags();

    result.procedureSize = environment.Size;
    result.procedureSizeSource = resolvedSize.source;
    result.stackSize = environment.StackSize;
    result.bpBased = environment.BpBased;
    result.initialized = true;
    return true;
}

} // namespace idr::core
