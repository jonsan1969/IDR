#pragma once

#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"

#include <cstddef>

namespace idr::core {

struct LoadedPeImage;

struct LegacyImageSessionView {
    DWord entryPoint = 0;
    DWord imageBase = 0;
    DWord imageSize = 0;
    DWord totalSize = 0;
    DWord codeBase = 0;
    DWord codeSize = 0;
    std::size_t analysisSize = 0;
    const DWord *flags = nullptr;
    void *const *infos = nullptr;
    const Byte *code = nullptr;
};

// Transitional bridges used while the real legacy translation units still
// call original global/Main-style APIs. Session owners can replace both the
// analysis state and interactive/headless policy surface explicitly.
void SetLegacyAnalysisState(AnalysisState *state);
AnalysisState &LegacyAnalysisState();
void SetLegacyServices(Services *services);
Services &LegacyServices();

// Bind one loaded PE image to both the neutral image context and the legacy
// analysis-facing Main-style state. This keeps the two views authoritative
// from one loaded image rather than requiring ad-hoc host setup.
void ActivateLegacyLoadedPeSession(LoadedPeImage &image);
void ResetLegacyLoadedPeSession();
LegacyImageSessionView GetLegacyImageSessionView();

} // namespace idr::core
