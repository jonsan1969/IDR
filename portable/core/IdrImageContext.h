#pragma once

#include "IdrCoreTypes.h"

#include <cstddef>
#include <optional>

namespace idr::core {

struct ImageView {
    Byte *data = nullptr;
    std::size_t size = 0;
    DWord imageBase = 0;
};

void SetImageView(ImageView image);
const ImageView &GetImageView();
int AddressToOffset(DWord address);
std::optional<DWord> OffsetToAddress(std::size_t offset);

} // namespace idr::core
