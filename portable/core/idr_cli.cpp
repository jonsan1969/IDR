#include "IdrControlFlow.h"
#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrLegacyCompat.h"
#include "IdrPeLoader.h"
#include "../../Disasm.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

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

} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) {
        std::cerr << "usage: idr-cli.exe <target.exe>\n";
        return 2;
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

    idr::core::ControlFlowResult flow;
    idr::core::ControlFlowOptions options;
    auto &analysis = idr::core::LegacyAnalysisState();
    if (!idr::core::AnalyzeBoundedControlFlow(session.entryPoint, analysis, decoder, options, flow)) {
        std::cerr << "idr-cli: control-flow analysis failed: " << flow.error << '\n';
        idr::core::ResetLegacyLoadedPeSession();
        return 7;
    }

    const int entryOffset = idr::core::AddressToOffset(session.entryPoint);
    const auto entryFlags = (entryOffset >= 0 && session.flags)
        ? session.flags[static_cast<std::size_t>(entryOffset)] : 0;
    const auto requiredEntryFlags = idr::core::CodeFlags::Instruction | idr::core::CodeFlags::Code;
    if ((entryFlags & requiredEntryFlags) != requiredEntryFlags) {
        std::cerr << "idr-cli: legacy flags view did not observe entry trace state\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 8;
    }

    for (const auto &candidate : flow.candidates) {
        const int offset = idr::core::AddressToOffset(candidate.address);
        const auto flags = (offset >= 0 && session.flags)
            ? session.flags[static_cast<std::size_t>(offset)] : 0;
        const auto required = idr::core::CodeFlags::Loc |
                              idr::core::CodeFlags::Instruction |
                              idr::core::CodeFlags::Code;
        if ((flags & required) != required) {
            std::cerr << "idr-cli: legacy flags view did not observe procedure candidate state\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 9;
        }
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

    for (std::size_t i = 0; i < flow.edges.size(); ++i) {
        const auto &edge = flow.edges[i];
        std::cout << "edge[" << i << "] kind=" << (edge.call ? "call" : "branch")
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

    std::cout << "trace-count=" << flow.entryTrace.size() << '\n';
    std::cout << "entry-block-count=" << flow.entryBlocks.size() << '\n';
    std::cout << "edge-count=" << flow.edges.size() << '\n';
    std::cout << "candidate-count=" << flow.candidates.size() << '\n';
    std::cout << "candidate-discovered-count=" << flow.discoveredCandidateCount << '\n';
    std::cout << "candidate-block-count=" << candidateBlockCount << '\n';
    std::cout << "candidate-instruction-count=" << candidateInstructionCount << '\n';
    std::cout << "entry-flags=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << entryFlags
              << std::dec << std::setfill(' ') << '\n';
    std::cout << "legacy-session=bound\n";

    idr::core::ResetLegacyLoadedPeSession();
    return 0;
}
