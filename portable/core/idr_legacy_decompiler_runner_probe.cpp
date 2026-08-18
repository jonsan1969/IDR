#include "IdrLegacyDecompilerRunner.h"
#include "IdrLegacyProcedureAdapter.h"
#include "IdrPeLoader.h"

#include <iostream>

int main() {
    constexpr idr::core::DWord kAddress = 0x00403000u;

    idr::core::LoadedPeImage image;
    image.bytes.assign(4, 0x90);
    image.bytes[0] = 0xC3;
    image.segments.push_back({kAddress, image.bytes.size(), 0});
    image.imageBase = 0x00400000u;
    image.imageSize = 0x4000u;
    image.entryPoint = kAddress;
    image.codeBase = kAddress;
    image.codeSize = static_cast<idr::core::DWord>(image.bytes.size());
    idr::core::ActivateLegacyLoadedPeSession(image);

    idr::core::ProcedurePrototypeMetadata prototype;
    prototype.kind = ikProc;
    idr::core::LegacyProcedureMetadataSeed seed;
    if (!idr::core::BuildLegacyProcedureMetadataSeed(prototype, ikFunc, seed)) return 1;
    if (!idr::core::ApplyLegacyProcedureMetadataSeedToActiveSession(kAddress, seed)) return 2;

    const auto session = idr::core::GetLegacyImageSessionView();
    if (!session.infos || !session.infos[0]) return 3;
    PInfoRec record = static_cast<PInfoRec>(session.infos[0]);
    if (!record || !record->procInfo || record->procInfo->procSize != 0) return 4;

    idr::core::LegacyDecompilerPreflightResult preflight;
    if (idr::core::PreflightActiveLegacyProcedure(kAddress, preflight)) return 5;
    if (preflight.initialized || preflight.procedureSize != 0 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::None ||
        preflight.stackSize != 0)
        return 6;

    std::size_t resolverCalls = 0;
    const idr::core::HeadlessProcedureSizeResolver resolver =
        [&](const idr::core::ProcedureSizeResolutionRequest &request) {
            ++resolverCalls;
            idr::core::ProcedureSizeResolutionResult result;
            if (request.procedureAddress != kAddress || request.storedSize != 0)
                return result;
            result.status = idr::core::ProcedureSizeResolutionStatus::Resolved;
            result.size = 1;
            return result;
        };

    if (!idr::core::PreflightActiveLegacyProcedure(kAddress, preflight, resolver)) return 7;
    if (resolverCalls != 1 || !preflight.initialized || preflight.procedureSize != 1 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::HeadlessResolver ||
        preflight.stackSize != 0x8000u || preflight.bpBased)
        return 8;
    if (record->procInfo->procSize != 0) return 9;

    record->procInfo->procSize = 1;
    resolverCalls = 0;
    if (!idr::core::PreflightActiveLegacyProcedure(kAddress, preflight, resolver)) return 10;
    if (resolverCalls != 0 || !preflight.initialized || preflight.procedureSize != 1 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata ||
        preflight.stackSize != 0x8000u || preflight.bpBased)
        return 11;
    if (record->procInfo->procSize != 1) return 12;

    const idr::core::HeadlessProcedureSizeResolver unavailable =
        [](const idr::core::ProcedureSizeResolutionRequest &) {
            return idr::core::ProcedureSizeResolutionResult{};
        };
    record->procInfo->procSize = 0;
    if (idr::core::PreflightActiveLegacyProcedure(kAddress, preflight, unavailable)) return 13;
    if (preflight.initialized || preflight.procedureSize != 0 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::None)
        return 14;

    idr::core::ResetLegacyLoadedPeSession();
    std::cout << "legacy-decompiler-runner-preflight=ok\n";
    std::cout << "headless-procedure-size-policy=ok\n";
    return 0;
}
