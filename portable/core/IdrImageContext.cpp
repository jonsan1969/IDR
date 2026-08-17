#include "IdrImageContext.h"

#include <utility>

namespace idr::core {
namespace {
ImageView g_image;
std::vector<SegmentView> g_segments;
}

void SetImageView(ImageView image) {
    g_image = image;
    g_segments.clear();
}

void SetImageSegments(ImageView image, std::vector<SegmentView> segments) {
    g_image = image;
    g_segments = std::move(segments);
}

const ImageView &GetImageView() { return g_image; }
const std::vector<SegmentView> &GetImageSegments() { return g_segments; }

int AddressToOffset(DWord address) {
    if (!g_image.data) return -2;

    if (g_segments.empty()) {
        if (address < g_image.imageBase) return -2;
        const auto offset = static_cast<std::size_t>(address - g_image.imageBase);
        if (offset >= g_image.size) return -2;
        return static_cast<int>(offset);
    }

    std::size_t packedOffset = 0;
    for (const auto &segment : g_segments) {
        const auto segmentEnd = static_cast<std::uint64_t>(segment.start) + segment.size;
        if (address >= segment.start && static_cast<std::uint64_t>(address) < segmentEnd) {
            if (segment.flags & SegmentFlags::Unbacked) return -1;
            const auto offset = packedOffset + static_cast<std::size_t>(address - segment.start);
            return offset < g_image.size ? static_cast<int>(offset) : -2;
        }
        if (!(segment.flags & SegmentFlags::Unbacked)) packedOffset += segment.size;
    }
    return -2;
}

std::optional<DWord> OffsetToAddress(std::size_t offset) {
    if (!g_image.data || offset >= g_image.size) return std::nullopt;

    if (g_segments.empty()) {
        return g_image.imageBase + static_cast<DWord>(offset);
    }

    std::size_t fromOffset = 0;
    for (const auto &segment : g_segments) {
        if (segment.flags & SegmentFlags::Unbacked) continue;
        const auto toOffset = fromOffset + segment.size;
        if (offset >= fromOffset && offset < toOffset) {
            return segment.start + static_cast<DWord>(offset - fromOffset);
        }
        fromOffset = toOffset;
    }
    return std::nullopt;
}

} // namespace idr::core

using Byte = idr::core::Byte;
using DWord = idr::core::DWord;

Byte *Code = nullptr;

int __fastcall Adr2Pos(DWord address) {
    Code = idr::core::GetImageView().data;
    return idr::core::AddressToOffset(address);
}
