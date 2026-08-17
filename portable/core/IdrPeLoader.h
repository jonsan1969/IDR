#pragma once

#include "IdrImageContext.h"

#include <filesystem>
#include <string>
#include <vector>

namespace idr::core {

struct LoadedPeImage {
    std::vector<Byte> bytes;
    std::vector<SegmentView> segments;
    DWord imageBase = 0;
    DWord imageSize = 0;
    DWord entryPoint = 0;
    DWord codeBase = 0;
    DWord codeSize = 0;
};

bool LoadPe32File(const std::filesystem::path &path, LoadedPeImage &image, std::string *error = nullptr);
void ActivateLoadedPeImage(LoadedPeImage &image);

} // namespace idr::core
