#include "IdrAnalysis.h"
#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"

#include <Windows.h>
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

    MDisasm disasm;
    const auto op = disasm.GetOp(const_cast<char *>("mov"));
    std::cout << "portable-core link probe: OP_MOV=" << static_cast<int>(op)
              << ", name=" << idr::core::DefaultProcName(0x401000)
              << ", type=" << idr::core::TrimTypeName("System.Integer")
              << ", flags=" << state.Size() << '\n';
    return op == OP_MOV ? 0 : 26;
}
