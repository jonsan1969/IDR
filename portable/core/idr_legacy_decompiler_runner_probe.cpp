#include "IdrLegacyBridge.h"
#include "IdrLegacyDecompilerRunner.h"
#include "IdrLegacyProcedureAdapter.h"
#include "IdrPeLoader.h"
#include "../../Disasm.h"

#include <iostream>

extern MDisasm Disasm;

int PortableEstimateProcSize(DWord address);

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

    // A later ProcStart used to make PortableEstimateProcSize invent a span.
    // With no explicit size policy, that transitional heuristic must stay gone.
    idr::core::LegacyAnalysisState().SetFlag(idr::core::CodeFlags::ProcStart, 2);
    if (PortableEstimateProcSize(kAddress) != 0) return 5;

    idr::core::LegacyDecompilerPreflightResult preflight;
    if (idr::core::PreflightActiveLegacyProcedure(kAddress, preflight)) return 6;
    if (preflight.initialized || preflight.procedureSize != 0 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::None ||
        preflight.stackSize != 0)
        return 7;

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

    idr::core::SetLegacyProcedureSizeResolver(resolver);
    if (PortableEstimateProcSize(kAddress) != 1 || resolverCalls != 1) return 8;

    resolverCalls = 0;
    if (!idr::core::PreflightActiveLegacyProcedure(kAddress, preflight)) return 9;
    if (resolverCalls != 1 || !preflight.initialized || preflight.procedureSize != 1 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::HeadlessResolver ||
        preflight.stackSize != 0x8000u || preflight.bpBased)
        return 10;
    if (record->procInfo->procSize != 0) return 11;

    record->procInfo->procSize = 1;
    resolverCalls = 0;
    if (PortableEstimateProcSize(kAddress) != 1 || resolverCalls != 0) return 12;
    if (!idr::core::PreflightActiveLegacyProcedure(kAddress, preflight)) return 13;
    if (resolverCalls != 0 || !preflight.initialized || preflight.procedureSize != 1 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata ||
        preflight.stackSize != 0x8000u || preflight.bpBased)
        return 14;
    if (record->procInfo->procSize != 1) return 15;

    const idr::core::HeadlessProcedureSizeResolver unavailable =
        [](const idr::core::ProcedureSizeResolutionRequest &) {
            return idr::core::ProcedureSizeResolutionResult{};
        };
    record->procInfo->procSize = 0;
    if (idr::core::PreflightActiveLegacyProcedure(kAddress, preflight, unavailable)) return 16;
    if (preflight.initialized || preflight.procedureSize != 0 ||
        preflight.procedureSizeSource != idr::core::ProcedureSizeSource::None)
        return 17;

    // First controlled execution of the real legacy decompiler loop.
    // Unlike preflight, Decompile() immediately uses the real MDisasm backend.
    // Initialize the shipped x86 dis.dll path explicitly before entering it.
    if (!Disasm.Init()) return 18;

    // The one-byte fixture is RET only, so no interactive path should be reached.
    record->procInfo->procSize = 1;
    std::size_t manualInputCalls = 0;
    auto services = idr::core::MakeHeadlessServices();
    services.manualInput = [&](idr::core::DWord, idr::core::DWord,
                               const std::string &, const std::string &)
        -> std::optional<std::string> {
        ++manualInputCalls;
        return std::nullopt;
    };
    idr::core::SetLegacyServices(&services);

    idr::core::LegacyDecompilerRunResult run;
    if (!idr::core::DecompileActiveLegacyProcedure(kAddress, run)) return 19;
    if (!run.decompiled || !run.wasRet || run.procedureSize != 1 ||
        run.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata)
        return 20;
    if (manualInputCalls != 0) return 21;
    if (record->procInfo->procSize != 1) return 22;

    idr::core::ResetLegacyLoadedPeSession();
    if (idr::core::LegacyProcedureSizeResolver()) return 23;
    std::cout << "legacy-decompiler-runner-preflight=ok\n";
    std::cout << "headless-procedure-size-policy=ok\n";
    std::cout << "legacy-procedure-size-bridge=ok\n";
    std::cout << "legacy-decompiler-decompile=ok\n";
    return 0;
}
