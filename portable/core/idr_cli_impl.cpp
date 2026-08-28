#include "IdrControlFlow.h"
#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrLegacyCompat.h"
#include "IdrLegacyProcedureAdapter.h"
#include "IdrLegacyDecompilerInput.h"
#include "IdrLegacyDecompilerRunner.h"
#include "IdrPeLoader.h"
#include "../../Disasm.h"

#include <climits>
#include <cwchar>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

extern MDisasm Disasm;

namespace {

void PrintHex(const char *label, idr::core::DWord value) {
    std::cout << label << "=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value
              << std::dec << std::setfill(' ') << '\n';
}

void PrintInstruction(const char *label, std::size_t firstIndex, std::size_t secondIndex,
                      const idr::core::TraceInstruction &instruction, bool nested) {
    std::cout << label << "[" << firstIndex;
    if (nested) std::cout << ":" << secondIndex;
    std::cout << "] address=0x" << std::uppercase << std::hex
              << std::setw(8) << std::setfill('0') << instruction.address
              << std::dec << std::setfill(' ')
              << " len=" << instruction.length
              << " mnemonic=" << instruction.mnemonic
              << " disasm=" << instruction.disasm << '\n';
}

const char *EdgeKindName(idr::core::ControlFlowEdgeKind kind) {
    switch (kind) {
        case idr::core::ControlFlowEdgeKind::Call: return "call";
        case idr::core::ControlFlowEdgeKind::BranchTaken: return "branch-taken";
        case idr::core::ControlFlowEdgeKind::FallThrough: return "fallthrough";
    }
    return "unknown";
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc != 2 && argc != 4) {
        std::cerr << "usage: idr-cli.exe <target.exe> [--entry-size <bytes>]\n";
        return 2;
    }

    int explicitEntrySize = 0;
    if (argc == 4) {
        if (std::wcscmp(argv[2], L"--entry-size") != 0) {
            std::cerr << "usage: idr-cli.exe <target.exe> [--entry-size <bytes>]\n";
            return 2;
        }
        wchar_t *end = nullptr;
        const long parsed = std::wcstol(argv[3], &end, 0);
        if (!argv[3][0] || !end || *end != L'\0' || parsed <= 0 || parsed > INT_MAX) {
            std::cerr << "idr-cli: invalid --entry-size\n";
            return 2;
        }
        explicitEntrySize = static_cast<int>(parsed);
    }

    const std::filesystem::path target(argv[1]);
    idr::core::LoadedPeImage image;
    std::string error;
    if (!idr::core::LoadPe32File(target, image, &error)) {
        std::cerr << "idr-cli: cannot load target: " << error << '\n';
        return 3;
    }

    idr::core::ActivateLegacyLoadedPeSession(image);
    const auto session = idr::core::GetLegacyImageSessionView();
    if (session.entryPoint != image.entryPoint ||
        session.imageBase != image.imageBase ||
        session.imageSize != image.imageSize ||
        session.codeBase != image.codeBase ||
        session.codeSize != image.codeSize ||
        session.analysisSize != image.bytes.size() ||
        session.code != image.bytes.data()) {
        std::cerr << "idr-cli: loaded image and legacy session views disagree\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 4;
    }

    if (!Disasm.Init()) {
        std::cerr << "idr-cli: cannot initialize legacy disassembler\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 5;
    }

    DISINFO entryInfo{};
    char entryLine[1024] = {};
    const int entryLength = Disasm.Disassemble(session.entryPoint, &entryInfo, entryLine);
    if (entryLength <= 0 || entryInfo.Mnem[0] == '\0') {
        std::cerr << "idr-cli: cannot decode entry point\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 6;
    }

    const idr::core::InstructionDecoder decoder = [](idr::core::DWord address,
                                                       idr::core::DecodedInstruction &decoded) {
        DISINFO info{};
        char line[1024] = {};
        const int length = Disasm.Disassemble(address, &info, line);
        if (length <= 0 || info.Mnem[0] == '\0') return false;
        decoded.length = length;
        decoded.mnemonic = info.Mnem;
        decoded.disasm = line;
        decoded.call = info.Call;
        decoded.branch = info.Branch;
        decoded.conditional = info.Conditional;
        decoded.ret = info.Ret;
        decoded.target = (info.Call || info.Branch) ? info.Immediate : 0;
        return true;
    };

    const idr::core::AddressMapper addressToOffset = [](idr::core::DWord address) {
        return idr::core::AddressToOffset(address);
    };

    idr::core::ControlFlowResult flow;
    idr::core::ControlFlowOptions options;
    auto &analysis = idr::core::LegacyAnalysisState();
    if (!idr::core::AnalyzeBoundedControlFlow(session.entryPoint, analysis, decoder,
                                               addressToOffset, options, flow)) {
        std::cerr << "idr-cli: control-flow analysis failed: " << flow.error << '\n';
        idr::core::ResetLegacyLoadedPeSession();
        return 7;
    }

    const int entryOffset = idr::core::AddressToOffset(session.entryPoint);
    const auto entryFlags = (entryOffset >= 0 && session.flags)
        ? session.flags[static_cast<std::size_t>(entryOffset)] : 0;
    const auto requiredEntryFlags = idr::core::CodeFlags::ProcStart |
                                    idr::core::CodeFlags::Instruction |
                                    idr::core::CodeFlags::Code;
    if ((entryFlags & requiredEntryFlags) != requiredEntryFlags) {
        std::cerr << "idr-cli: legacy flags view did not observe entry procedure state\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 8;
    }

    for (const auto &candidate : flow.candidates) {
        const int offset = idr::core::AddressToOffset(candidate.address);
        const auto flags = (offset >= 0 && session.flags)
            ? session.flags[static_cast<std::size_t>(offset)] : 0;
        const auto required = idr::core::CodeFlags::Loc |
                              idr::core::CodeFlags::ProcStart |
                              idr::core::CodeFlags::Instruction |
                              idr::core::CodeFlags::Code;
        if ((flags & required) != required) {
            std::cerr << "idr-cli: legacy flags view did not observe procedure candidate state\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 9;
        }
    }

    if (flow.procedures.size() != flow.candidates.size() + 1) {
        std::cerr << "idr-cli: procedure summary count does not match analyzed procedures\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 10;
    }
    for (const auto &procedure : flow.procedures) {
        if (procedure.observedStart == 0 ||
            procedure.observedEndExclusive <= procedure.observedStart ||
            procedure.observedSpan != static_cast<std::size_t>(procedure.observedEndExclusive - procedure.observedStart)) {
            std::cerr << "idr-cli: procedure summary has invalid observed instruction span\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 11;
        }
    }

    std::size_t providerCalls = 0;
    const idr::core::LegacyProcedurePrototypeProvider fallbackProvider =
        [&](const idr::core::ProcedureSummary &, idr::core::ProcedurePrototypeMetadata &metadata) {
            ++providerCalls;
            metadata.kind = ikProc;
            return true;
        };
    std::vector<idr::core::DWord> installedProcedures;
    std::vector<idr::core::DWord> reusedProcedures;
    if (!idr::core::ApplyDiscoveredProceduresToActiveLegacySession(
            flow, ikFunc, fallbackProvider, &installedProcedures, &reusedProcedures)) {
        std::cerr << "idr-cli: cannot reconcile discovered procedures with legacy metadata\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 12;
    }
    if (installedProcedures.size() + reusedProcedures.size() != flow.procedures.size() ||
        providerCalls != installedProcedures.size()) {
        std::cerr << "idr-cli: reconciled procedure count does not match CFG procedures\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 13;
    }
    const auto materializedSession = idr::core::GetLegacyImageSessionView();
    for (const auto &procedure : flow.procedures) {
        const int offset = idr::core::AddressToOffset(procedure.address);
        if (offset < 0 || !materializedSession.infos) {
            std::cerr << "idr-cli: materialized procedure address is not mapped\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 14;
        }
        PInfoRec record = static_cast<PInfoRec>(materializedSession.infos[static_cast<std::size_t>(offset)]);
        idr::core::ProcedurePrototypeMetadata captured;
        if (!record || !idr::core::CaptureLegacyProcedurePrototypeMetadata(*record, captured)) {
            std::cerr << "idr-cli: materialized procedure lacks valid legacy metadata\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 15;
        }
    }

    std::size_t decompileInputCount = 0;
    std::size_t decompileCalleePrototypeCount = 0;
    for (const auto &procedure : flow.procedures) {
        idr::core::ProcedureDecompileInput decompileInput;
        if (!idr::core::BuildProcedureDecompileInputFromActiveLegacySession(
                flow, procedure.address, ikFunc, decompileInput)) {
            std::cerr << "idr-cli: cannot build decompiler input from reconciled procedure metadata\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 16;
        }
        if (decompileInput.analysis.summary.address != procedure.address) {
            std::cerr << "idr-cli: decompiler input procedure does not match CFG procedure\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 17;
        }
        ++decompileInputCount;
        decompileCalleePrototypeCount += decompileInput.callees.size();
    }

    bool entryDecompiled = false;
    std::size_t manualInputCalls = 0;
    idr::core::ProcedureSourceResult entrySource;
    auto services = idr::core::MakeHeadlessServices();
    if (explicitEntrySize > 0) {
        std::cout << "entry-decompile-stage=services-prepare\n" << std::flush;
        services.manualInput = [&](idr::core::DWord, idr::core::DWord,
                                   const std::string &, const std::string &)
            -> std::optional<std::string> {
            ++manualInputCalls;
            return std::nullopt;
        };
        idr::core::SetLegacyServices(&services);
        std::cout << "entry-decompile-stage=services-installed\n" << std::flush;

        const idr::core::HeadlessProcedureSizeResolver entrySizeResolver =
            [&](const idr::core::ProcedureSizeResolutionRequest &request) {
                idr::core::ProcedureSizeResolutionResult result;
                if (request.procedureAddress != session.entryPoint || request.storedSize != 0)
                    return result;
                result.status = idr::core::ProcedureSizeResolutionStatus::Resolved;
                result.size = explicitEntrySize;
                return result;
            };

        std::cout << "entry-decompile-stage=decompile-enter\n" << std::flush;
        const bool decompileOk = idr::core::DecompileActiveLegacyProcedureSource(
            session.entryPoint, entrySource, entrySizeResolver);
        std::cout << "entry-decompile-stage=decompile-returned ok="
                  << (decompileOk ? 1 : 0) << '\n' << std::flush;
        if (!decompileOk) {
            std::cerr << "idr-cli: entry decompile failed with explicit size\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 18;
        }
        if (!entrySource.completed || entrySource.procedureAddress != session.entryPoint ||
            entrySource.procedureSize != explicitEntrySize ||
            entrySource.procedureSizeSource != idr::core::ProcedureSizeSource::HeadlessResolver) {
            std::cerr << "idr-cli: entry decompile result does not match explicit size contract\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 19;
        }
        if (manualInputCalls != 0) {
            std::cerr << "idr-cli: entry decompile requested unexpected manual input\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 20;
        }
        if (entrySource.body.size() < 2 || entrySource.body.front() != "begin" ||
            entrySource.body.back() != "end") {
            std::cerr << "idr-cli: entry decompile source envelope is incomplete\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 21;
        }
        std::cout << "entry-decompile-stage=contract-ok\n" << std::flush;
        entryDecompiled = true;
    }

    std::cout << "IDR portable CLI\n";
    std::cout << "file=" << target.u8string() << '\n';
    PrintHex("image-base", image.imageBase);
    PrintHex("image-size", image.imageSize);
    PrintHex("entry-point", image.entryPoint);
    PrintHex("code-base", image.codeBase);
    PrintHex("code-size", image.codeSize);
    std::cout << "analysis-bytes=" << image.bytes.size() << '\n';
    std::cout << "segments=" << image.segments.size() << '\n';

    for (std::size_t i = 0; i < image.segments.size(); ++i) {
        const auto &segment = image.segments[i];
        std::cout << "segment[" << i << "] start=0x"
                  << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << segment.start
                  << " size=0x" << std::setw(8) << segment.size
                  << " flags=0x" << std::setw(8) << segment.flags
                  << std::dec << std::setfill(' ')
                  << " state=" << ((segment.flags & idr::core::SegmentFlags::Unbacked) ? "unbacked" : "backed")
                  << '\n';
    }

    std::cout << "entry-instruction-len=" << entryLength << '\n';
    std::cout << "entry-mnemonic=" << entryInfo.Mnem << '\n';
    std::cout << "entry-disasm=" << entryLine << '\n';

    for (std::size_t i = 0; i < flow.entryTrace.size(); ++i)
        PrintInstruction("trace", i, 0, flow.entryTrace[i], false);

    std::size_t callEdgeCount = 0;
    std::size_t branchTakenEdgeCount = 0;
    std::size_t fallThroughEdgeCount = 0;
    for (std::size_t i = 0; i < flow.edges.size(); ++i) {
        const auto &edge = flow.edges[i];
        switch (edge.kind) {
            case idr::core::ControlFlowEdgeKind::Call: ++callEdgeCount; break;
            case idr::core::ControlFlowEdgeKind::BranchTaken: ++branchTakenEdgeCount; break;
            case idr::core::ControlFlowEdgeKind::FallThrough: ++fallThroughEdgeCount; break;
        }
        std::cout << "edge[" << i << "] kind=" << EdgeKindName(edge.kind)
                  << " from=0x" << std::uppercase << std::hex
                  << std::setw(8) << std::setfill('0') << edge.from
                  << " to=0x" << std::setw(8) << edge.to
                  << std::dec << std::setfill(' ') << " state=mapped\n";
    }

    std::size_t candidateInstructionCount = 0;
    std::size_t candidateBlockCount = 0;
    for (std::size_t i = 0; i < flow.candidates.size(); ++i) {
        const auto &candidate = flow.candidates[i];
        candidateBlockCount += candidate.blocks.size();
        std::cout << "candidate[" << i << "] address=0x"
                  << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
                  << candidate.address << std::dec << std::setfill(' ') << '\n';
        for (std::size_t j = 0; j < candidate.instructions.size(); ++j) {
            PrintInstruction("candidate-trace", i, j, candidate.instructions[j], true);
            ++candidateInstructionCount;
        }
    }

    for (std::size_t i = 0; i < flow.procedures.size(); ++i) {
        const auto &procedure = flow.procedures[i];
        std::cout << "procedure[" << i << "] address=0x"
                  << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << procedure.address
                  << " observed-start=0x" << std::setw(8) << procedure.observedStart
                  << " observed-end-exclusive=0x" << std::setw(8) << procedure.observedEndExclusive
                  << std::dec << std::setfill(' ')
                  << " observed-span=" << procedure.observedSpan
                  << " blocks=" << procedure.blockCount
                  << " instructions=" << procedure.instructionCount
                  << " incoming-calls=" << procedure.incomingCallCount << '\n';
    }

    std::cout << "trace-count=" << flow.entryTrace.size() << '\n';
    std::cout << "entry-block-count=" << flow.entryBlocks.size() << '\n';
    std::cout << "edge-count=" << flow.edges.size() << '\n';
    std::cout << "call-edge-count=" << callEdgeCount << '\n';
    std::cout << "branch-taken-edge-count=" << branchTakenEdgeCount << '\n';
    std::cout << "fallthrough-edge-count=" << fallThroughEdgeCount << '\n';
    std::cout << "candidate-count=" << flow.candidates.size() << '\n';
    std::cout << "candidate-discovered-count=" << flow.discoveredCandidateCount << '\n';
    std::cout << "candidate-block-count=" << candidateBlockCount << '\n';
    std::cout << "candidate-instruction-count=" << candidateInstructionCount << '\n';
    std::cout << "procedure-start-count=" << (flow.candidates.size() + 1) << '\n';
    std::cout << "procedure-summary-count=" << flow.procedures.size() << '\n';
    std::cout << "legacy-procedure-installed-count=" << installedProcedures.size() << '\n';
    std::cout << "legacy-procedure-reused-count=" << reusedProcedures.size() << '\n';
    std::cout << "decompile-input-count=" << decompileInputCount << '\n';
    std::cout << "decompile-callee-prototype-count=" << decompileCalleePrototypeCount << '\n';
    if (entryDecompiled) {
        std::cout << "entry-decompile-size=" << entrySource.procedureSize << '\n';
        std::cout << "entry-decompile-size-source=headless-resolver\n";
        std::cout << "entry-decompile-manual-input-calls=" << manualInputCalls << '\n';
        std::cout << "entry-source-count=" << entrySource.body.size() << '\n';
        for (std::size_t i = 0; i < entrySource.body.size(); ++i)
            std::cout << "entry-source[" << i << "]=" << entrySource.body[i] << '\n';
        std::cout << "entry-decompile-source-envelope=ok\n";
    }
    std::cout << "entry-flags=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << entryFlags
              << std::dec << std::setfill(' ') << '\n';
    std::cout << "legacy-session=bound\n";

    idr::core::ResetLegacyLoadedPeSession();
    return 0;
}