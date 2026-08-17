#pragma once

#include "IdrAnalysisState.h"

namespace idr::core {

// Flag-only instruction navigation extracted from Misc.cpp. These helpers do
// not disassemble bytes; they operate solely on the neutral analysis state.
int GetNearestArgA(const AnalysisState &state, int fromPos);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos);
int GetNthUpInstruction(const AnalysisState &state, int fromPos, int count);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos, int count);

} // namespace idr::core
