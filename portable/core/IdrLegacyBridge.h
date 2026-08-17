#pragma once

#include "IdrAnalysisState.h"

namespace idr::core {

// Transitional bridge used while the real legacy Decompiler translation unit
// still calls the original global Misc-style API. The owner of an analysis
// session should point this at its real state before invoking TDecompiler.
void SetLegacyAnalysisState(AnalysisState *state);
AnalysisState &LegacyAnalysisState();

} // namespace idr::core
