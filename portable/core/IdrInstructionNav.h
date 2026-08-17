#pragma once

#include "IdrAnalysisState.h"

class MDisasm;

namespace idr::core {

// Flag-only instruction navigation extracted from Misc.cpp. These helpers do
// not disassemble bytes; they operate solely on the neutral analysis state.
int GetNearestArgA(const AnalysisState &state, int fromPos);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos);
int GetNthUpInstruction(const AnalysisState &state, int fromPos, int count);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos);
int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos, int count);

// Decoder-backed navigation. These preserve the legacy Misc.cpp search
// semantics while routing position/address translation through ImageContext.
int GetNearestUpPrefixFs(const AnalysisState &state, MDisasm &disasm, int fromPos);
int GetNearestUpInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                    int fromPos, int toPos, const char *instruction);
int GetNearestUpInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                    int fromPos, int toPos,
                                    const char *instruction1, const char *instruction2);
int GetNearestDownInstruction(const AnalysisState &state, MDisasm &disasm, int fromPos);
int GetNearestDownInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                      int fromPos, const char *instruction);

} // namespace idr::core
