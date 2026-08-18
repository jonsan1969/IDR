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
    if (preflight.initialized || preflight.procedureSize != 0 || preflight.stackSize != 0) return 6;

    record->procInfo->procSize = 1;
    if (!idr::core::PreflightActiveLegacyProcedure(kAddress, preflight)) return 7;
    if (!preflight.initialized || preflight.procedureSize != 1 ||
        preflight.stackSize != 0x8000u || preflight.bpBased)
        return 8;
    if (record->procInfo->procSize != 1) return 9;

    idr::core::ResetLegacyLoadedPeSession();
    std::cout << "legacy-decompiler-runner-preflight=ok\n";
    return 0;
}
