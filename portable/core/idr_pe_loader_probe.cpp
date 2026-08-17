#include "IdrImageContext.h"
#include "IdrLegacyBridge.h"
#include "IdrPeLoader.h"

#define NOMINMAX
#include <Windows.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

template <typename T>
void WriteObject(std::vector<idr::core::Byte> &file, std::size_t offset, const T &value) {
    std::memcpy(file.data() + offset, &value, sizeof(T));
}

bool BuildSyntheticPe32(const std::filesystem::path &path) {
    std::vector<idr::core::Byte> file(0xA00, 0);

    IMAGE_DOS_HEADER dos{};
    dos.e_magic = IMAGE_DOS_SIGNATURE;
    dos.e_lfanew = 0x80;
    WriteObject(file, 0, dos);

    const std::size_t ntOffset = 0x80;
    const DWORD signature = IMAGE_NT_SIGNATURE;
    WriteObject(file, ntOffset, signature);

    IMAGE_FILE_HEADER fh{};
    fh.Machine = IMAGE_FILE_MACHINE_I386;
    fh.NumberOfSections = 4;
    fh.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    fh.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
    WriteObject(file, ntOffset + sizeof(DWORD), fh);

    IMAGE_OPTIONAL_HEADER32 oh{};
    oh.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    oh.ImageBase = 0x00400000;
    oh.AddressOfEntryPoint = 0x1000;
    oh.SectionAlignment = 0x1000;
    oh.FileAlignment = 0x200;
    oh.SizeOfImage = 0x5000;
    oh.SizeOfHeaders = 0x400;
    oh.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = 0x2000;
    oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = 0x200;
    WriteObject(file, ntOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), oh);

    const auto sectionOffset = ntOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER32);
    IMAGE_SECTION_HEADER sections[4]{};

    std::memcpy(sections[0].Name, ".text", 5);
    sections[0].Misc.VirtualSize = 0x600;
    sections[0].VirtualAddress = 0x1000;
    sections[0].SizeOfRawData = 0x200;
    sections[0].PointerToRawData = 0x400;
    sections[0].Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    std::memcpy(sections[1].Name, ".rsrc", 5);
    sections[1].Misc.VirtualSize = 0x300;
    sections[1].VirtualAddress = 0x2000;
    sections[1].SizeOfRawData = 0x200;
    sections[1].PointerToRawData = 0x600;
    sections[1].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    std::memcpy(sections[2].Name, ".data", 5);
    sections[2].Misc.VirtualSize = 0x500;
    sections[2].VirtualAddress = 0x3000;
    sections[2].SizeOfRawData = 0x200;
    sections[2].PointerToRawData = 0x800;
    sections[2].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    std::memcpy(sections[3].Name, ".bss", 4);
    sections[3].Misc.VirtualSize = 0x400;
    sections[3].VirtualAddress = 0x4000;
    sections[3].SizeOfRawData = 0;
    sections[3].Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    std::memcpy(file.data() + sectionOffset, sections, sizeof(sections));
    file[0x400] = 0x90;
    file[0x401] = 0xC3;
    file[0x600] = 0x52;
    file[0x800] = 0xCC;
    file[0x802] = 0x2A;

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char *>(file.data()), static_cast<std::streamsize>(file.size()));
    return static_cast<bool>(out);
}

} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path() / "idr-portable-pe32-probe.exe";
    if (!BuildSyntheticPe32(path)) return 1;

    idr::core::LoadedPeImage image;
    std::string error;
    const bool loaded = idr::core::LoadPe32File(path, image, &error);
    std::error_code removeError;
    std::filesystem::remove(path, removeError);
    if (!loaded) {
        std::cerr << error << '\n';
        return 2;
    }

    if (image.imageBase != 0x00400000) return 3;
    if (image.imageSize != 0x5000) return 4;
    if (image.entryPoint != 0x00401000) return 5;
    if (image.codeBase != 0x00401000) return 6;
    if (image.codeSize != 0x2000 || image.bytes.size() != 0x2000) return 7;
    if (image.segments.size() != 4) return 8;
    if (image.segments[0].start != 0x00401000 || image.segments[0].size != 0x1000) return 9;
    if (!(image.segments[1].flags & idr::core::SegmentFlags::Unbacked)) return 10;
    if (image.segments[2].start != 0x00403000 || image.segments[2].size != 0x1000) return 11;
    if (!(image.segments[3].flags & idr::core::SegmentFlags::Unbacked)) return 12;
    if (image.bytes[0] != 0x90 || image.bytes[1] != 0xC3) return 13;
    if (image.bytes[0x1000] != 0xCC || image.bytes[0x1002] != 0x2A) return 14;

    idr::core::ActivateLegacyLoadedPeSession(image);
    if (idr::core::AddressToOffset(0x00401001) != 1) return 15;
    if (idr::core::AddressToOffset(0x00402010) != -1) return 16;
    if (idr::core::AddressToOffset(0x00403002) != 0x1002) return 17;
    if (idr::core::AddressToOffset(0x00404010) != -1) return 18;
    const auto address = idr::core::OffsetToAddress(0x1002);
    if (!address || *address != 0x00403002) return 19;

    const auto session = idr::core::GetLegacyImageSessionView();
    if (session.entryPoint != 0x00401000 || session.imageBase != 0x00400000) return 20;
    if (session.imageSize != 0x5000 || session.totalSize != 0x2000) return 21;
    if (session.codeBase != 0x00401000 || session.codeSize != 0x2000) return 22;
    if (session.analysisSize != 0x2000 || !session.flags || !session.infos || !session.code) return 23;
    if (session.code != image.bytes.data() || session.code[0] != 0x90 || session.code[0x1002] != 0x2A) return 24;
    if (session.flags[0] != 0 || session.infos[0] != nullptr) return 25;
    if (!idr::core::LegacyAnalysisState().SetFlag(idr::core::CodeFlags::Code, 0)) return 26;
    if ((session.flags[0] & idr::core::CodeFlags::Code) == 0) return 27;

    idr::core::ResetLegacyLoadedPeSession();
    const auto reset = idr::core::GetLegacyImageSessionView();
    if (reset.entryPoint || reset.imageBase || reset.imageSize || reset.totalSize || reset.codeBase || reset.codeSize) return 28;
    if (reset.analysisSize != 0 || reset.flags || reset.infos || reset.code) return 29;
    if (idr::core::GetImageView().data != nullptr || !idr::core::GetImageSegments().empty()) return 30;

    std::cout << "portable PE32 session probe: base=00400000 entry=00401000 packed=0x2000 legacy-state=bound\n";
    return 0;
}
