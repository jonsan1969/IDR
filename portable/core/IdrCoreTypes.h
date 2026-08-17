#pragma once

#include <cstdint>

namespace idr::core {

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;

namespace CodeFlags {
inline constexpr DWord Undef = 0x00000000;
inline constexpr DWord Code = 0x00000001;
inline constexpr DWord Data = 0x00000002;
inline constexpr DWord Import = 0x00000004;
inline constexpr DWord Call = 0x00000008;
inline constexpr DWord ProcStart = 0x00000010;
inline constexpr DWord ProcEnd = 0x00000020;
inline constexpr DWord Rtti = 0x00000040;
inline constexpr DWord Embedded = 0x00000080;
inline constexpr DWord Pass0 = 0x00000100;
inline constexpr DWord Frame = 0x00000200;
inline constexpr DWord Switch = 0x00000400;
inline constexpr DWord Pass1 = 0x00000800;
inline constexpr DWord ExceptionTable = 0x00001000;
inline constexpr DWord Push = 0x00002000;
inline constexpr DWord DecompilerSkip = 0x00004000;
inline constexpr DWord Pop = 0x00008000;
inline constexpr DWord SetA = 0x00010000;
inline constexpr DWord SetD = 0x00020000;
inline constexpr DWord SetC = 0x00040000;
inline constexpr DWord Bracket = 0x00080000;
inline constexpr DWord Pass2 = 0x00100000;
inline constexpr DWord Export = 0x00200000;
inline constexpr DWord Pass = 0x00400000;
inline constexpr DWord Loc = 0x00800000;
inline constexpr DWord Try = 0x01000000;
inline constexpr DWord Finally = 0x02000000;
inline constexpr DWord Except = 0x04000000;
inline constexpr DWord Loop = 0x08000000;
inline constexpr DWord FinallyExit = 0x10000000;
inline constexpr DWord VTable = 0x20000000;
inline constexpr DWord Skip = 0x40000000;
inline constexpr DWord Instruction = 0x80000000;
} // namespace CodeFlags

namespace InfoKind {
inline constexpr Byte Unknown = 0x00;
inline constexpr Byte Integer = 0x01;
inline constexpr Byte Char = 0x02;
inline constexpr Byte Enumeration = 0x03;
inline constexpr Byte Float = 0x04;
inline constexpr Byte String = 0x05;
inline constexpr Byte Set = 0x06;
inline constexpr Byte Class = 0x07;
inline constexpr Byte Method = 0x08;
inline constexpr Byte WideChar = 0x09;
inline constexpr Byte LongString = 0x0A;
inline constexpr Byte WideString = 0x0B;
inline constexpr Byte Variant = 0x0C;
inline constexpr Byte Array = 0x0D;
inline constexpr Byte Record = 0x0E;
inline constexpr Byte Interface = 0x0F;
inline constexpr Byte Int64 = 0x10;
inline constexpr Byte DynamicArray = 0x11;
inline constexpr Byte UnicodeString = 0x12;
inline constexpr Byte ClassRef = 0x13;
inline constexpr Byte Pointer = 0x14;
inline constexpr Byte Procedure = 0x15;
inline constexpr Byte CString = 0x20;
inline constexpr Byte WideCString = 0x21;
inline constexpr Byte ResourceString = 0x22;
inline constexpr Byte Vmt = 0x23;
inline constexpr Byte Guid = 0x24;
inline constexpr Byte Refine = 0x25;
inline constexpr Byte Constructor = 0x26;
inline constexpr Byte Destructor = 0x27;
inline constexpr Byte Proc = 0x28;
inline constexpr Byte Func = 0x29;
inline constexpr Byte Loc = 0x2A;
inline constexpr Byte Data = 0x2B;
inline constexpr Byte DataLink = 0x2C;
inline constexpr Byte ExceptName = 0x2D;
inline constexpr Byte ExceptHandler = 0x2E;
inline constexpr Byte ExceptCase = 0x2F;
inline constexpr Byte Switch = 0x30;
inline constexpr Byte Case = 0x31;
inline constexpr Byte Fixup = 0x32;
inline constexpr Byte ThreadVar = 0x33;
inline constexpr Byte Try = 0x34;
} // namespace InfoKind

enum class NameVersion : std::uint8_t { Primary, AfterScan, ByUser };
enum class OrdType : std::uint8_t { SignedByte, UnsignedByte, SignedWord, UnsignedWord, SignedLong, UnsignedLong };
enum class FloatType : std::uint8_t { Single, Double, Extended, Comp, Currency };
enum class MethodKind : std::uint8_t { Procedure, Function, Constructor, Destructor, ClassProcedure, ClassFunction };

struct CaseInfo { int caseNo = 0; int count = 0; };
struct ProcHistory { DWord address = 0; int itemIndex = 0; int xrefIndex = 0; int topIndex = 0; };

static_assert(sizeof(Byte) == 1);
static_assert(sizeof(Word) == 2);
static_assert(sizeof(DWord) == 4);
static_assert(InfoKind::Interface == 0x0F);
static_assert(CodeFlags::Instruction == 0x80000000u);

} // namespace idr::core
