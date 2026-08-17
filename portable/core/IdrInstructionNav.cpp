#include "IdrInstructionNav.h"

#include <algorithm>

namespace idr::core {
namespace {
int PreviousValidPos(const AnalysisState &state, int fromPos) {
    if (fromPos <= 0 || state.Size() == 0) return -1;
    const auto last = static_cast<std::size_t>(fromPos - 1);
    return static_cast<int>(std::min(last, state.Size() - 1));
}
} // namespace

int GetNearestArgA(const AnalysisState &state, int fromPos) {
    for (int pos = PreviousValidPos(state, fromPos); pos >= 0; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
            if (state.IsFlagSet(CodeFlags::SetA, index)) return pos;
        }
    }
    return -1;
}

int GetNearestUpInstruction(const AnalysisState &state, int fromPos) {
    for (int pos = PreviousValidPos(state, fromPos); pos >= 0; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) return pos;
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNthUpInstruction(const AnalysisState &state, int fromPos, int count) {
    if (count <= 0) return -1;
    for (int pos = PreviousValidPos(state, fromPos); pos >= 0; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            if (--count == 0) return pos;
        }
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos) {
    if (toPos < 0) toPos = 0;
    for (int pos = PreviousValidPos(state, fromPos); pos >= toPos; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) return pos;
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNearestUpInstruction(const AnalysisState &state, int fromPos, int toPos, int count) {
    if (count <= 0) return -1;
    if (toPos < 0) toPos = 0;
    for (int pos = PreviousValidPos(state, fromPos); pos >= toPos; --pos) {
        if (state.IsFlagSet(CodeFlags::Instruction, static_cast<std::size_t>(pos)) && --count == 0) {
            return pos;
        }
    }
    return -1;
}

} // namespace idr::core
