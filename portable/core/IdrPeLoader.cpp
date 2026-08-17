#include "IdrPeLoader.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>

namespace idr::core {
namespace {

bool Fail(std::string *error, const char *message) {
    if (error) *error = message;
    return false;
}

template <typename T>
bool ReadObject(const std::vector<Byte> &file, std::size_t offset, T &value) {
    if (offset > file.size() || sizeof(T) > file.size() - offset) return false;
    std::memcpy(&value, file.data() + offset, sizeof(T));
    return true;
}

} // namespace

bool LoadPe32File(const std::filesystem::path &path, LoadedPeImage &image, std::string *error) {
    image = {};
    if (error) error->clear();

    std::ifstream input(path, std::ios::binary);
    if (!input) return Fail(error, "cannot open file");
    std::vector<Byte> file((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (file.size() < sizeof(IMAGE_DOS_HEADER)) return Fail(error, "file is too small for DOS header");

    IMAGE_DOS_HEADER dos{};
    if (!ReadObject(file, 0, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
        return Fail(error, "not an MZ executable");

    const auto ntOffset = static_cast<std::size_t>(dos.e_lfanew);
    DWord signature = 0;
    IMAGE_FILE_HEADER fileHeader{};
    if (!ReadObject(file, ntOffset, signature) || signature != IMAGE_NT_SIGNATURE)
        return Fail(error, "not a PE executable");
    if (!ReadObject(file, ntOffset + sizeof(DWord), fileHeader))
        return Fail(error, "truncated PE file header");
    if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386)
        return Fail(error, "PE image is not x86");
    if (fileHeader.NumberOfSections == 0)
        return Fail(error, "PE image has no sections");
    if (fileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER32))
        return Fail(error, "truncated PE32 optional header");

    const auto optionalOffset = ntOffset + sizeof(DWord) + sizeof(IMAGE_FILE_HEADER);
    IMAGE_OPTIONAL_HEADER32 optional{};
    if (!ReadObject(file, optionalOffset, optional) || optional.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return Fail(error, "PE image is not PE32");

    const auto sectionsOffset = optionalOffset + fileHeader.SizeOfOptionalHeader;
    const auto sectionBytes = static_cast<std::size_t>(fileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
    if (sectionsOffset > file.size() || sectionBytes > file.size() - sectionsOffset)
        return Fail(error, "truncated section table");

    std::vector<IMAGE_SECTION_HEADER> sections(fileHeader.NumberOfSections);
    std::memcpy(sections.data(), file.data() + sectionsOffset, sectionBytes);
    for (std::size_t i = 1; i < sections.size(); ++i) {
        if (sections[i].VirtualAddress <= sections[i - 1].VirtualAddress)
            return Fail(error, "section virtual addresses are not strictly increasing");
    }

    const DWord resourceVa = optional.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE
        ? optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress : 0;
    const DWord relocVa = optional.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC
        ? optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress : 0;

    image.imageBase = optional.ImageBase;
    image.imageSize = optional.SizeOfImage;
    image.entryPoint = optional.ImageBase + optional.AddressOfEntryPoint;
    image.codeBase = optional.ImageBase + sections.front().VirtualAddress;
    image.segments.reserve(sections.size());

    std::size_t packedSize = 0;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto &section = sections[i];
        DWord span = section.Misc.VirtualSize;
        if (i + 1 < sections.size()) span = sections[i + 1].VirtualAddress - section.VirtualAddress;

        const bool unbacked = section.SizeOfRawData == 0 ||
            (resourceVa != 0 && section.VirtualAddress == resourceVa) ||
            (relocVa != 0 && section.VirtualAddress == relocVa);

        DWord flags = section.Characteristics;
        if (unbacked) flags |= SegmentFlags::Unbacked;
        image.segments.push_back({optional.ImageBase + section.VirtualAddress, span, flags});

        if (!unbacked) {
            if (span > std::numeric_limits<std::size_t>::max() - packedSize)
                return Fail(error, "packed image size overflow");
            packedSize += span;
        }
    }
    if (packedSize == 0 || packedSize > std::numeric_limits<DWord>::max())
        return Fail(error, "PE image has no backed analysis bytes");

    image.bytes.assign(packedSize, 0);
    image.codeSize = static_cast<DWord>(packedSize);

    std::size_t packedOffset = 0;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto &section = sections[i];
        const auto &segment = image.segments[i];
        if (segment.flags & SegmentFlags::Unbacked) continue;

        const auto span = static_cast<std::size_t>(segment.size);
        const auto rawSize = static_cast<std::size_t>(section.SizeOfRawData);
        const auto rawOffset = static_cast<std::size_t>(section.PointerToRawData);
        if (rawSize > span) return Fail(error, "section raw data exceeds legacy analysis span");
        if (rawOffset > file.size() || rawSize > file.size() - rawOffset)
            return Fail(error, "section raw data is outside file");
        if (rawSize) std::memcpy(image.bytes.data() + packedOffset, file.data() + rawOffset, rawSize);
        packedOffset += span;
    }

    return true;
}

void ActivateLoadedPeImage(LoadedPeImage &image) {
    SetImageSegments({image.bytes.data(), image.bytes.size(), image.imageBase}, image.segments);
}

} // namespace idr::core
