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
    std::cout << "legacy-session=bound\n";
    idr::core::ResetLegacyLoadedPeSession();
    return 0;
}
