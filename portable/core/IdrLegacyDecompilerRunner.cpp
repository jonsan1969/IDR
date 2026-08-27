#include "IdrLegacyDecompilerRunner.h"

#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrLegacyProcedureAdapter.h"
#include "Decompiler.portable.h"

namespace idr::core {
namespace {

bool ResolveActiveLegacyProcedure(
    DWord address,
    const HeadlessProcedureSizeResolver &sizeResolver,
    PInfoRec &record,
    ResolvedProcedureSize &resolvedSize) {
    record = nullptr;
    resolvedSize = {};

    const auto session = GetLegacyImageSessionView();
    if (!Infos || !session.infos ||
        session.infos != reinterpret_cast<void *const *>(Infos))
        return false;

    const int offset = AddressToOffset(address);
    if (offset < 0) return false;
    const auto pos = static_cast<std::size_t>(offset);
    if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;

    record = Infos[pos];
    if (!record || record->kind < ikRefine || record->kind > ikFunc || !record->procInfo)
        return false;

    ProcedureSizeResolutionRequest sizeRequest;
    sizeRequest.procedureAddress = address;
    sizeRequest.storedSize = record->procInfo->procSize;
    const auto &effectiveResolver = sizeResolver ? sizeResolver : LegacyProcedureSizeResolver();
    return ResolveProcedureSize(sizeRequest, effectiveResolver, resolvedSize);
}

void CaptureBody(TDecompileEnv &environment, std::vector<std::string> &body) {
    body.clear();
    if (!environment.Body) return;
    body.reserve(static_cast<std::size_t>(environment.Body->Count));
    for (int index = 0; index < environment.Body->Count; ++index)
        body.push_back(environment.Body->Strings[index]);
}

} // namespace

bool PreflightActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerPreflightResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver) {
    result = {};

    PInfoRec record = nullptr;
    ResolvedProcedureSize resolvedSize;
    if (!ResolveActiveLegacyProcedure(address, sizeResolver, record, resolvedSize)) return false;

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

bool DecompileActiveLegacyProcedure(
    DWord address,
    ProcedureDecompileResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver) {
    result = {};

    PInfoRec record = nullptr;
    ResolvedProcedureSize resolvedSize;
    if (!ResolveActiveLegacyProcedure(address, sizeResolver, record, resolvedSize)) return false;

    TDecompileEnv environment(address, resolvedSize.size, record);
    TDecompiler decompiler(&environment);
    if (!decompiler.Init(address)) return false;
    decompiler.InitFlags();
    decompiler.SetStop(address + static_cast<DWord>(resolvedSize.size));

    const DWord endAddress = decompiler.Decompile(address, 0, nullptr);

    result.procedureAddress = address;
    result.procedureSize = environment.Size;
    result.procedureSizeSource = resolvedSize.source;
    result.endAddress = endAddress;
    result.wasRet = decompiler.WasRet;
    result.completed = true;
    CaptureBody(environment, result.body);
    return true;
}

bool DecompileActiveLegacyProcedureSource(
    DWord address,
    ProcedureSourceResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver) {
    result = {};

    ProcedureDecompileResult lowLevel;
    if (!DecompileActiveLegacyProcedure(address, lowLevel, sizeResolver)) return false;

    result.procedureAddress = lowLevel.procedureAddress;
    result.procedureSize = lowLevel.procedureSize;
    result.procedureSizeSource = lowLevel.procedureSizeSource;
    result.completed = lowLevel.completed;
    result.body.reserve(lowLevel.body.size() + 2);
    result.body.push_back("begin");
    result.body.insert(result.body.end(), lowLevel.body.begin(), lowLevel.body.end());
    result.body.push_back("end");
    return true;
}

} // namespace idr::core
