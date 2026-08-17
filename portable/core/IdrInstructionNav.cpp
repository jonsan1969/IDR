#include "IdrInstructionNav.h"
#include "IdrImageContext.h"

#include <Windows.h>
#include <cctype>
#include <cstdint>
#include <cstring>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;

#include "../../Disasm.h"

namespace idr::core {
namespace {
int PreviousValidPos(const AnalysisState &state, int fromPos) {
    if (fromPos <= 0 || state.Size() == 0) return -1;
    const auto last = static_cast<std::size_t>(fromPos - 1);
    const auto maxIndex = state.Size() - 1;
    return static_cast<int>(last < maxIndex ? last : maxIndex);
}

bool PrefixEqualsInsensitive(const char *actual, const char *expected) {
    if (!actual || !expected || !*expected) return false;
    for (; *expected; ++actual, ++expected) {
        if (!*actual) return false;
        const auto a = static_cast<unsigned char>(*actual);
        const auto e = static_cast<unsigned char>(*expected);
        if (std::tolower(a) != std::tolower(e)) return false;
    }
    return true;
}

bool PrefixEqualsExact(const char *actual, const char *expected) {
    if (!actual || !expected || !*expected) return false;
    const auto length = std::strlen(expected);
    return std::strncmp(actual, expected, length) == 0;
}

bool DecodeAt(MDisasm &disasm, std::size_t pos, DISINFO &info) {
    const auto address = OffsetToAddress(pos);
    if (!address) return false;
    return disasm.Disassemble(*address, &info, nullptr) > 0;
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
        if (state.IsFlagSet(CodeFlags::Instruction, static_cast<std::size_t>(pos)) && --count == 0) return pos;
    }
    return -1;
}

int GetNearestUpPrefixFs(const AnalysisState &state, MDisasm &disasm, int fromPos) {
    for (int pos = PreviousValidPos(state, fromPos); pos >= 0; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            DISINFO info{};
            if (DecodeAt(disasm, index, info) && info.SegPrefix == 4) return pos;
        }
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNearestUpInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                    int fromPos, int toPos, const char *instruction) {
    if (toPos < 0) toPos = 0;
    for (int pos = PreviousValidPos(state, fromPos); pos >= toPos; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            DISINFO info{};
            if (DecodeAt(disasm, index, info) && PrefixEqualsInsensitive(info.Mnem, instruction)) return pos;
        }
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNearestUpInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                    int fromPos, int toPos,
                                    const char *instruction1, const char *instruction2) {
    if (toPos < 0) toPos = 0;
    for (int pos = PreviousValidPos(state, fromPos); pos >= toPos; --pos) {
        const auto index = static_cast<std::size_t>(pos);
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            DISINFO info{};
            if (DecodeAt(disasm, index, info) &&
                (PrefixEqualsInsensitive(info.Mnem, instruction1) || PrefixEqualsInsensitive(info.Mnem, instruction2))) return pos;
        }
        if (state.IsFlagSet(CodeFlags::ProcStart, index)) break;
    }
    return -1;
}

int GetNearestDownInstruction(const AnalysisState &state, MDisasm &disasm, int fromPos) {
    if (fromPos < -1) fromPos = -1;
    for (std::size_t index = static_cast<std::size_t>(fromPos + 1); index < state.Size(); ++index) {
        if (state.IsFlagSet(CodeFlags::Instruction, index)) return static_cast<int>(index);
        if (state.IsFlagSet(CodeFlags::ProcEnd, index)) break;
    }
    return -1;
}

int GetNearestDownInstructionMatching(const AnalysisState &state, MDisasm &disasm,
                                      int fromPos, const char *instruction) {
    if (fromPos < -1) fromPos = -1;
    for (std::size_t index = static_cast<std::size_t>(fromPos + 1); index < state.Size(); ++index) {
        if (state.IsFlagSet(CodeFlags::Instruction, index)) {
            DISINFO info{};
            if (DecodeAt(disasm, index, info) && PrefixEqualsExact(info.Mnem, instruction)) return static_cast<int>(index);
        }
        if (state.IsFlagSet(CodeFlags::ProcEnd, index)) break;
    }
    return -1;
}

} // namespace idr::core
