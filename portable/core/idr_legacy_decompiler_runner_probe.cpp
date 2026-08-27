#include "IdrLegacyBridge.h"
#include "IdrLegacyDecompilerRunner.h"
#include "IdrLegacyProcedureAdapter.h"
#include "IdrPeLoader.h"
#include "../../Disasm.h"

#include <algorithm>
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

    if (!Disasm.Init()) return 18;

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

    idr::core::ProcedureDecompileResult run;
    if (!idr::core::DecompileActiveLegacyProcedure(kAddress, run)) return 19;
    if (!run.completed || !run.wasRet || run.procedureAddress != kAddress ||
        run.procedureSize != 1 ||
        run.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata)
        return 20;
    if (manualInputCalls != 0) return 21;
    if (record->procInfo->procSize != 1) return 22;

    idr::core::ResetLegacyLoadedPeSession();
    if (idr::core::LegacyProcedureSizeResolver()) return 23;

    idr::core::LoadedPeImage frameImage;
    frameImage.bytes = {0x55, 0x8B, 0xEC, 0x5D, 0xC3};
    frameImage.segments.push_back({kAddress, frameImage.bytes.size(), 0});
    frameImage.imageBase = 0x00400000u;
    frameImage.imageSize = 0x4000u;
    frameImage.entryPoint = kAddress;
    frameImage.codeBase = kAddress;
    frameImage.codeSize = static_cast<idr::core::DWord>(frameImage.bytes.size());
    idr::core::ActivateLegacyLoadedPeSession(frameImage);
    if (!idr::core::ApplyLegacyProcedureMetadataSeedToActiveSession(kAddress, seed)) return 24;

    const auto frameSession = idr::core::GetLegacyImageSessionView();
    if (!frameSession.infos || !frameSession.infos[0]) return 25;
    PInfoRec frameRecord = static_cast<PInfoRec>(frameSession.infos[0]);
    if (!frameRecord || !frameRecord->procInfo) return 26;
    frameRecord->procInfo->procSize = 5;
    idr::core::LegacyAnalysisState().SetFlag(idr::core::CodeFlags::ProcStart, 0);

    manualInputCalls = 0;
    idr::core::SetLegacyServices(&services);
    idr::core::ProcedureDecompileResult frameRun;
    if (!idr::core::DecompileActiveLegacyProcedure(kAddress, frameRun)) return 27;
    if (!frameRun.completed || !frameRun.wasRet || frameRun.procedureAddress != kAddress ||
        frameRun.procedureSize != 5 ||
        frameRun.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata)
        return 28;
    if (manualInputCalls != 0) return 29;
    if (frameRecord->procInfo->procSize != 5) return 30;

    manualInputCalls = 0;
    idr::core::ProcedureSourceResult sourceRun;
    if (!idr::core::DecompileActiveLegacyProcedureSource(kAddress, sourceRun)) return 31;
    if (!sourceRun.completed || sourceRun.procedureAddress != kAddress ||
        sourceRun.procedureSize != 5 ||
        sourceRun.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata)
        return 32;
    if (manualInputCalls != 0) return 33;
    if (sourceRun.body.size() < 2) return 34;
    if (sourceRun.body.front() != "begin") return 35;
    if (sourceRun.body.back() != "end") return 36;
    if (frameRecord->procInfo->procSize != 5) return 37;

    idr::core::ResetLegacyLoadedPeSession();

    constexpr idr::core::DWord kCalleeAddress = kAddress + 0x10u;
    idr::core::LoadedPeImage callImage;
    callImage.bytes.assign(0x20, 0x90);
    callImage.bytes[0] = 0xE8;
    callImage.bytes[1] = 0x0B;
    callImage.bytes[2] = 0x00;
    callImage.bytes[3] = 0x00;
    callImage.bytes[4] = 0x00;
    callImage.bytes[5] = 0xC3;
    callImage.bytes[0x10] = 0xB8;
    callImage.bytes[0x11] = 0x07;
    callImage.bytes[0x12] = 0x00;
    callImage.bytes[0x13] = 0x00;
    callImage.bytes[0x14] = 0x00;
    callImage.bytes[0x15] = 0xC3;
    callImage.segments.push_back({kAddress, callImage.bytes.size(), 0});
    callImage.imageBase = 0x00400000u;
    callImage.imageSize = 0x4000u;
    callImage.entryPoint = kAddress;
    callImage.codeBase = kAddress;
    callImage.codeSize = static_cast<idr::core::DWord>(callImage.bytes.size());
    std::cout << "direct-call-stage=activate\n" << std::flush;
    idr::core::ActivateLegacyLoadedPeSession(callImage);
    std::cout << "direct-call-stage=session-active\n" << std::flush;

    if (!idr::core::ApplyLegacyProcedureMetadataSeedToActiveSession(kAddress, seed)) return 38;
    idr::core::ProcedurePrototypeMetadata calleePrototype;
    calleePrototype.kind = ikProc;
    idr::core::LegacyProcedureMetadataSeed calleeSeed;
    if (!idr::core::BuildLegacyProcedureMetadataSeed(calleePrototype, ikFunc, calleeSeed)) return 39;
    if (!idr::core::ApplyLegacyProcedureMetadataSeedToActiveSession(kCalleeAddress, calleeSeed)) return 40;
    std::cout << "direct-call-stage=metadata-seeded\n" << std::flush;

    const auto callSession = idr::core::GetLegacyImageSessionView();
    if (!callSession.infos || !callSession.infos[0] || !callSession.infos[0x10]) return 41;
    PInfoRec callerRecord = static_cast<PInfoRec>(callSession.infos[0]);
    PInfoRec calleeRecord = static_cast<PInfoRec>(callSession.infos[0x10]);
    if (!callerRecord || !callerRecord->procInfo || !calleeRecord || !calleeRecord->procInfo) return 42;
    callerRecord->procInfo->procSize = 6;
    calleeRecord->procInfo->procSize = 6;
    idr::core::LegacyAnalysisState().SetFlag(idr::core::CodeFlags::ProcStart, 0);
    idr::core::LegacyAnalysisState().SetFlag(idr::core::CodeFlags::ProcStart, 0x10);
    std::cout << "direct-call-stage=records-ready\n" << std::flush;

    manualInputCalls = 0;
    idr::core::SetLegacyServices(&services);
    idr::core::ProcedureSourceResult callRun;
    std::cout << "direct-call-stage=decompile-enter\n" << std::flush;
    if (!idr::core::DecompileActiveLegacyProcedureSource(kAddress, callRun)) return 43;
    std::cout << "direct-call-stage=decompile-returned\n" << std::flush;
    if (!callRun.completed || callRun.procedureAddress != kAddress ||
        callRun.procedureSize != 6 ||
        callRun.procedureSizeSource != idr::core::ProcedureSizeSource::LegacyMetadata)
        return 44;
    if (manualInputCalls != 0) return 45;
    if (callRun.body.size() < 3 || callRun.body.front() != "begin" || callRun.body.back() != "end") return 46;
    if (callerRecord->procInfo->procSize != 6 || calleeRecord->procInfo->procSize != 6) return 47;

    idr::core::ResetLegacyLoadedPeSession();
    std::cout << "legacy-decompiler-runner-preflight=ok\n";
    std::cout << "headless-procedure-size-policy=ok\n";
    std::cout << "legacy-procedure-size-bridge=ok\n";
    std::cout << "legacy-decompiler-decompile=ok\n";
    std::cout << "neutral-decompiler-result=ok\n";
    std::cout << "legacy-decompiler-stack-frame=ok\n";
    std::cout << "neutral-procedure-source=ok\n";
    std::cout << "legacy-decompiler-direct-call=ok\n";
    return 0;
}
