#pragma once

#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"

namespace idr::core {

// Transitional bridges used while the real legacy translation units still
// call original global/Main-style APIs. Session owners can replace both the
// analysis state and interactive/headless policy surface explicitly.
void SetLegacyAnalysisState(AnalysisState *state);
AnalysisState &LegacyAnalysisState();
void SetLegacyServices(Services *services);
Services &LegacyServices();

} // namespace idr::core
