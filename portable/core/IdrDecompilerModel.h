#pragma once

#include "IdrCoreTypes.h"

#include <array>
#include <string>

namespace idr::core {

namespace Precedence {
inline constexpr Byte None = 0;
inline constexpr Byte Not = 6;
inline constexpr Byte Compare = 9;
inline constexpr Byte Add = 14;
inline constexpr Byte Multiply = 15;
inline constexpr Byte Unary = 16;
inline constexpr Byte Atom = 24;
}

namespace ItemFlags {
inline constexpr DWord Arg = 1;
inline constexpr DWord Var = 2;
inline constexpr DWord StackPtr = 4;
inline constexpr DWord CallResult = 8;
inline constexpr DWord VmtAddress = 16;
inline constexpr DWord CycleVar = 32;
inline constexpr DWord Field = 64;
inline constexpr DWord ArrayPtr = 128;
inline constexpr DWord IntValue = 256;
inline constexpr DWord Interface = 512;
inline constexpr DWord ExternVar = 1024;
inline constexpr DWord RecordFieldOffset = 2048;
}

struct DecompilerItem {
    Byte precedence = Precedence::Atom;
    int size = 0;
    int offset = 0;
    DWord intValue = 0;
    DWord flags = 0;
    std::string value;
    std::string value1;
    std::string type;
    std::string name;
};

struct CompareItem {
    std::string left;
    char operation = 0;
    std::string right;
};

struct IndexInfo {
    Byte indexType = 0;
    int indexValue = 0;
    std::string indexText;
};

using RegisterItems = std::array<DecompilerItem, 8>;

void InitDecompilerItem(DecompilerItem &item);
void AssignDecompilerItem(DecompilerItem &destination, const DecompilerItem &source);
std::string DirectCondition(char condition);
std::string InvertCondition(char condition);

} // namespace idr::core
