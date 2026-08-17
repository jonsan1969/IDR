#include "IdrAnalysis.h"
#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"
#include "IdrDecompilerModel.h"
#include "IdrImageContext.h"
#include "IdrInstructionNav.h"

#include <Windows.h>
#include <array>
#include <cstdint>
#include <iostream>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;

#include "../../Disasm.h"

int main() {
    const auto services = idr::core::MakeHeadlessServices();
    if (!services.confirmEmbeddedProcedure || !services.lookupMethod || !services.manualInput) return 2;

    if (idr::core::Hex(0x2A, 8) != "0000002A") return 3;
    if (idr::core::DefaultProcName(0x401000) != "sub_00401000") return 4;
    if (idr::core::GlobalVarName(0x402000) != "gvar_00402000") return 5;
    if (!idr::core::CanReplaceTypeName("DWORD", "Integer")) return 6;
    if (idr::core::CanReplaceTypeName("Integer", "DWORD")) return 7;

    if (idr::core::ExtractClassName("TForm1.ButtonClick") != "TForm1") return 8;
    if (idr::core::ExtractProcName("TForm1.ButtonClick") != "ButtonClick") return 9;
    if (idr::core::ExtractProcName("ButtonClick") != "ButtonClick") return 10;
    if (idr::core::ExtractName("Value:Integer") != "Value") return 11;
    if (idr::core::ExtractType("Value:Integer") != "Integer") return 12;
    if (idr::core::TrimTypeName("System.Integer") != "Integer") return 13;
    if (idr::core::TrimTypeName("1..10") != "1..10") return 14;

    idr::core::DecompilerItem source;
    source.flags = idr::core::ItemFlags::Arg | idr::core::ItemFlags::IntValue;
    source.precedence = idr::core::Precedence::Add;
    source.size = 4;
    source.offset = 99;
    source.intValue = 42;
    source.value = "eax + 4";
    source.value1 = "aux";
    source.type = "Integer";
    source.name = "Value";
    idr::core::DecompilerItem destination;
    destination.offset = 7;
    idr::core::AssignDecompilerItem(destination, source);
    if (destination.flags != source.flags || destination.intValue != 42 || destination.value != "eax + 4") return 15;
    if (destination.offset != 7) return 16;
    idr::core::InitDecompilerItem(destination);
    if (destination.flags != 0 || destination.precedence != idr::core::Precedence::Atom || !destination.value.empty()) return 17;
    if (idr::core::DirectCondition('E') != "=") return 18;
    if (idr::core::InvertCondition('E') != "<>") return 19;
    if (idr::core::DirectCondition('@') != "?") return 20;
    if (idr::core::DirectCondition('Z') != "?") return 21;

    idr::core::AnalysisState state(16);
    if (state.Size() != 16) return 22;
    if (!state.SetFlag(idr::core::CodeFlags::Instruction, 3)) return 23;
    if (!state.IsFlagSet(idr::core::CodeFlags::Instruction, 3)) return 24;
    if (!state.SetFlags(idr::core::CodeFlags::SetA, 5, 3)) return 25;
    if (!state.IsFlagSet(idr::core::CodeFlags::SetA, 5) || !state.IsFlagSet(idr::core::CodeFlags::SetA, 6) || !state.IsFlagSet(idr::core::CodeFlags::SetA, 7)) return 26;
    if (!state.ClearFlag(idr::core::CodeFlags::SetA, 6)) return 27;
    if (state.IsFlagSet(idr::core::CodeFlags::SetA, 6)) return 28;
    if (!state.ClearFlags(idr::core::CodeFlags::SetA, 5, 3)) return 29;
    if (state.IsFlagSet(idr::core::CodeFlags::SetA, 5) || state.IsFlagSet(idr::core::CodeFlags::SetA, 7)) return 30;
    if (state.SetFlag(idr::core::CodeFlags::Code, 16)) return 31;
    if (state.SetFlags(idr::core::CodeFlags::Code, 15, 2)) return 32;

    idr::core::AnalysisState nav(20);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 2);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 5);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 8);
    nav.SetFlag(idr::core::CodeFlags::SetA, 5);
    nav.SetFlag(idr::core::CodeFlags::ProcStart, 3);
    if (idr::core::GetNearestUpInstruction(nav, 9) != 8) return 33;
    if (idr::core::GetNthUpInstruction(nav, 9, 2) != 5) return 34;
    if (idr::core::GetNearestUpInstruction(nav, 8, 4) != 5) return 35;
    if (idr::core::GetNearestUpInstruction(nav, 9, 0, 3) != 2) return 36;
    if (idr::core::GetNearestArgA(nav, 8) != 5) return 37;
    if (idr::core::GetNearestUpInstruction(nav, 5) != -1) return 38;

    std::array<Byte, 9> image{{0x90, 0x64, 0xA1, 0, 0, 0, 0, 0x90, 0xC3}};
    idr::core::SetImageSegments({image.data(), image.size(), 0}, {{0x00401000, 4, 0}, {0x00500000, 0x10, idr::core::SegmentFlags::Unbacked}, {0x00402000, 5, 0}});
    if (idr::core::AddressToOffset(0x00401002) != 2) return 39;
    if (idr::core::AddressToOffset(0x00402002) != 6) return 40;
    if (idr::core::AddressToOffset(0x00500004) != -1) return 41;
    if (idr::core::AddressToOffset(0x00600000) != -2) return 42;
    const auto segmentedAddress = idr::core::OffsetToAddress(6);
    if (!segmentedAddress || *segmentedAddress != 0x00402002) return 43;

    idr::core::SetImageView({image.data(), image.size(), 0x00401000});
    if (idr::core::AddressToOffset(0x00401001) != 1) return 44;
    const auto address = idr::core::OffsetToAddress(8);
    if (!address || *address != 0x00401008) return 45;

    idr::core::AnalysisState decoded(image.size());
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 0);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 1);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 7);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 8);

    MDisasm disasm;
    if (!disasm.Init()) return 46;
    if (idr::core::GetNearestUpPrefixFs(decoded, disasm, 8) != 1) return 47;
    if (idr::core::GetNearestUpInstructionMatching(decoded, disasm, 8, 0, "mov") != 1) return 48;
    if (idr::core::GetNearestUpInstructionMatching(decoded, disasm, 8, 0, "push", "mov") != 1) return 49;
    if (idr::core::GetNearestDownInstruction(decoded, disasm, 0) != 1) return 50;
    if (idr::core::GetNearestDownInstructionMatching(decoded, disasm, 0, "ret") != 8) return 51;

    const auto op = disasm.GetOp(const_cast<char *>("mov"));
    std::cout << "portable-core link probe: OP_MOV=" << static_cast<int>(op)
              << ", decompiler-condition=" << idr::core::DirectCondition('E')
              << ", name=" << idr::core::DefaultProcName(0x401000)
              << ", fs=" << idr::core::GetNearestUpPrefixFs(decoded, disasm, 8)
              << ", segmented=00402002->6\n";
    return op == OP_MOV ? 0 : 52;
}
