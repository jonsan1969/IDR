#include "IdrAnalysis.h"
#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"
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

    idr::core::AnalysisState state(16);
    if (state.Size() != 16) return 15;
    if (!state.SetFlag(idr::core::CodeFlags::Instruction, 3)) return 16;
    if (!state.IsFlagSet(idr::core::CodeFlags::Instruction, 3)) return 17;
    if (!state.SetFlags(idr::core::CodeFlags::SetA, 5, 3)) return 18;
    if (!state.IsFlagSet(idr::core::CodeFlags::SetA, 5) ||
        !state.IsFlagSet(idr::core::CodeFlags::SetA, 6) ||
        !state.IsFlagSet(idr::core::CodeFlags::SetA, 7)) return 19;
    if (!state.ClearFlag(idr::core::CodeFlags::SetA, 6)) return 20;
    if (state.IsFlagSet(idr::core::CodeFlags::SetA, 6)) return 21;
    if (!state.ClearFlags(idr::core::CodeFlags::SetA, 5, 3)) return 22;
    if (state.IsFlagSet(idr::core::CodeFlags::SetA, 5) ||
        state.IsFlagSet(idr::core::CodeFlags::SetA, 7)) return 23;
    if (state.SetFlag(idr::core::CodeFlags::Code, 16)) return 24;
    if (state.SetFlags(idr::core::CodeFlags::Code, 15, 2)) return 25;

    idr::core::AnalysisState nav(20);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 2);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 5);
    nav.SetFlag(idr::core::CodeFlags::Instruction, 8);
    nav.SetFlag(idr::core::CodeFlags::SetA, 5);
    nav.SetFlag(idr::core::CodeFlags::ProcStart, 3);
    if (idr::core::GetNearestUpInstruction(nav, 9) != 8) return 26;
    if (idr::core::GetNthUpInstruction(nav, 9, 2) != 5) return 27;
    if (idr::core::GetNearestUpInstruction(nav, 8, 4) != 5) return 28;
    if (idr::core::GetNearestUpInstruction(nav, 9, 0, 3) != 2) return 29;
    if (idr::core::GetNearestArgA(nav, 8) != 5) return 30;
    if (idr::core::GetNearestUpInstruction(nav, 5) != -1) return 31;

    std::array<Byte, 9> image{{
        0x90,
        0x64, 0xA1, 0, 0, 0, 0,
        0x90,
        0xC3
    }};

    idr::core::SetImageSegments(
        {image.data(), image.size(), 0},
        {{0x00401000, 4, 0},
         {0x00500000, 0x10, idr::core::SegmentFlags::Unbacked},
         {0x00402000, 5, 0}});
    if (idr::core::AddressToOffset(0x00401002) != 2) return 32;
    if (idr::core::AddressToOffset(0x00402002) != 6) return 33;
    if (idr::core::AddressToOffset(0x00500004) != -1) return 34;
    if (idr::core::AddressToOffset(0x00600000) != -2) return 35;
    const auto segmentedAddress = idr::core::OffsetToAddress(6);
    if (!segmentedAddress || *segmentedAddress != 0x00402002) return 36;

    idr::core::SetImageView({image.data(), image.size(), 0x00401000});
    if (idr::core::AddressToOffset(0x00401001) != 1) return 37;
    const auto address = idr::core::OffsetToAddress(8);
    if (!address || *address != 0x00401008) return 38;

    idr::core::AnalysisState decoded(image.size());
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 0);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 1);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 7);
    decoded.SetFlag(idr::core::CodeFlags::Instruction, 8);

    MDisasm disasm;
    if (!disasm.Init()) return 39;
    if (idr::core::GetNearestUpPrefixFs(decoded, disasm, 8) != 1) return 40;
    if (idr::core::GetNearestUpInstructionMatching(decoded, disasm, 8, 0, "mov") != 1) return 41;
    if (idr::core::GetNearestUpInstructionMatching(decoded, disasm, 8, 0, "push", "mov") != 1) return 42;
    if (idr::core::GetNearestDownInstruction(decoded, disasm, 0) != 1) return 43;
    if (idr::core::GetNearestDownInstructionMatching(decoded, disasm, 0, "ret") != 8) return 44;

    const auto op = disasm.GetOp(const_cast<char *>("mov"));
    std::cout << "portable-core link probe: OP_MOV=" << static_cast<int>(op)
              << ", name=" << idr::core::DefaultProcName(0x401000)
              << ", type=" << idr::core::TrimTypeName("System.Integer")
              << ", flags=" << state.Size()
              << ", nearest=" << idr::core::GetNearestUpInstruction(nav, 9)
              << ", fs=" << idr::core::GetNearestUpPrefixFs(decoded, disasm, 8)
              << ", segmented=00402002->6\n";
    return op == OP_MOV ? 0 : 45;
}
