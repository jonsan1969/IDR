#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrLegacyCompat.h"
#include "IdrPeLoader.h"
#include "../../Disasm.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

extern MDisasm Disasm;

namespace {

void PrintHex(const char *label, idr::core::DWord value) {
    std::cout << label << "=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value
              << std::dec << std::setfill(' ') << '\n';
}

void PrintTraceAddress(std::size_t index, idr::core::DWord address) {
    std::cout << "trace[" << index << "] address=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << address
              << std::dec << std::setfill(' ');
}

void PrintEdge(std::size_t index, const char *kind,
               idr::core::DWord from, idr::core::DWord to) {
    std::cout << "edge[" << index << "] kind=" << kind
              << " from=0x" << std::uppercase << std::hex
              << std::setw(8) << std::setfill('0') << from
              << " to=0x" << std::setw(8) << to
              << std::dec << std::setfill(' ')
              << " state=mapped\n";
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

    const int entryDirectTargetOffset =
        (entryInfo.Call && entryInfo.Immediate != 0)
            ? idr::core::AddressToOffset(entryInfo.Immediate)
            : -1;

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

    auto &analysis = idr::core::LegacyAnalysisState();
    constexpr std::size_t kTraceLimit = 8;
    idr::core::DWord traceAddress = session.entryPoint;
    std::size_t traceCount = 0;
    std::size_t edgeCount = 0;

    for (; traceCount < kTraceLimit; ++traceCount) {
        const int offset = idr::core::AddressToOffset(traceAddress);
        if (offset < 0) break;

        DISINFO info{};
        char line[1024] = {};
        const int length = Disasm.Disassemble(traceAddress, &info, line);
        if (length <= 0 || info.Mnem[0] == '\0') break;

        const auto pos = static_cast<std::size_t>(offset);
        if (!analysis.SetFlag(idr::core::CodeFlags::Instruction, pos) ||
            !analysis.SetFlags(idr::core::CodeFlags::Code, pos, static_cast<std::size_t>(length))) {
            std::cerr << "idr-cli: cannot mark decoded instruction state\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 7;
        }

        if (info.Call && !analysis.SetFlag(idr::core::CodeFlags::Call, pos)) {
            std::cerr << "idr-cli: cannot mark call instruction state\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 8;
        }

        PrintTraceAddress(traceCount, traceAddress);
        std::cout << " len=" << length
                  << " mnemonic=" << info.Mnem
                  << " disasm=" << line << '\n';

        if ((info.Call || info.Branch) && info.Immediate != 0) {
            const int targetOffset = idr::core::AddressToOffset(info.Immediate);
            if (targetOffset >= 0) {
                if (!analysis.SetFlag(idr::core::CodeFlags::Loc,
                                      static_cast<std::size_t>(targetOffset))) {
                    std::cerr << "idr-cli: cannot mark control-flow target state\n";
                    idr::core::ResetLegacyLoadedPeSession();
                    return 9;
                }
                PrintEdge(edgeCount, info.Call ? "call" : "branch",
                          traceAddress, info.Immediate);
                ++edgeCount;
            }
        }

        if (info.Ret || (info.Branch && !info.Conditional)) {
            ++traceCount;
            break;
        }

        if (static_cast<unsigned int>(length) >
            (std::numeric_limits<idr::core::DWord>::max)() - traceAddress) {
            ++traceCount;
            break;
        }
        traceAddress += static_cast<idr::core::DWord>(length);
    }

    if (traceCount == 0) {
        std::cerr << "idr-cli: entry trace produced no instructions\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 10;
    }

    const int entryOffset = idr::core::AddressToOffset(session.entryPoint);
    const auto entryFlags = (entryOffset >= 0 && session.flags)
        ? session.flags[static_cast<std::size_t>(entryOffset)]
        : 0;
    const auto requiredEntryFlags = idr::core::CodeFlags::Instruction | idr::core::CodeFlags::Code;
    if ((entryFlags & requiredEntryFlags) != requiredEntryFlags) {
        std::cerr << "idr-cli: legacy flags view did not observe entry trace state\n";
        idr::core::ResetLegacyLoadedPeSession();
        return 11;
    }

    if (entryDirectTargetOffset >= 0) {
        const auto targetFlags = session.flags
            ? session.flags[static_cast<std::size_t>(entryDirectTargetOffset)]
            : 0;
        if (edgeCount == 0 ||
            (entryFlags & idr::core::CodeFlags::Call) == 0 ||
            (targetFlags & idr::core::CodeFlags::Loc) == 0) {
            std::cerr << "idr-cli: direct entry call did not propagate control-flow state\n";
            idr::core::ResetLegacyLoadedPeSession();
            return 12;
        }
    }

    std::cout << "trace-count=" << traceCount << '\n';
    std::cout << "edge-count=" << edgeCount << '\n';
    std::cout << "entry-flags=0x"
              << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << entryFlags
              << std::dec << std::setfill(' ') << '\n';
    std::cout << "legacy-session=bound\n";
    idr::core::ResetLegacyLoadedPeSession();
    return 0;
}
