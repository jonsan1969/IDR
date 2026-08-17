#include "IdrAnalysis.h"
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

    MDisasm disasm;
    const auto op = disasm.GetOp(const_cast<char *>("mov"));
    std::cout << "portable-core link probe: OP_MOV=" << static_cast<int>(op)
              << ", name=" << idr::core::DefaultProcName(0x401000) << '\n';
    return op == OP_MOV ? 0 : 8;
}
