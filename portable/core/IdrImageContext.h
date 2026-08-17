#pragma once

#include "IdrCoreTypes.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace idr::core {

struct ImageView {
    Byte *data = nullptr;
    std::size_t size = 0;
    DWord imageBase = 0;
};

struct SegmentView {
    DWord start = 0;
    DWord size = 0;
    DWord flags = 0;
};

namespace SegmentFlags {
inline constexpr DWord Unbacked = 0x00080000;
}

void SetImageView(ImageView image);
void SetImageSegments(ImageView image, std::vector<SegmentView> segments);
const ImageView &GetImageView();
const std::vector<SegmentView> &GetImageSegments();
int AddressToOffset(DWord address);
std::optional<DWord> OffsetToAddress(std::size_t offset);

} // namespace idr::core
