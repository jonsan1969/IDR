#include "IdrImageContext.h"

namespace idr::core {
namespace {
ImageView g_image;
}

void SetImageView(ImageView image) { g_image = image; }
const ImageView &GetImageView() { return g_image; }

int AddressToOffset(DWord address) {
    if (!g_image.data || address < g_image.imageBase) return -1;
    const auto offset = static_cast<std::size_t>(address - g_image.imageBase);
    if (offset >= g_image.size) return -1;
    return static_cast<int>(offset);
}

std::optional<DWord> OffsetToAddress(std::size_t offset) {
    if (!g_image.data || offset >= g_image.size) return std::nullopt;
    return g_image.imageBase + static_cast<DWord>(offset);
}

} // namespace idr::core

using Byte = idr::core::Byte;
using DWord = idr::core::DWord;

Byte *Code = nullptr;

int __fastcall Adr2Pos(DWord address) {
    Code = idr::core::GetImageView().data;
    return idr::core::AddressToOffset(address);
}
