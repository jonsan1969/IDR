#include "IdrAnalysis.h"
#include "IdrAnalysisState.h"
#include "IdrCoreServices.h"
#include "IdrDecompilerModel.h"
#include "IdrImageContext.h"
#include "IdrInstructionNav.h"
#include "IdrLegacyProcedureAdapter.h"

#include <array>
#include <iostream>

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

    idr::core::ProcedurePrototypeMetadata prototype;
    prototype.kind = ikFunc;
    prototype.returnType = "Integer";
    prototype.flags = PF_BPBASED | 1u;
    prototype.bpBase = 8;
    prototype.retBytes = 4;
    prototype.stackSize = 64;
    prototype.arguments.push_back({0x21, true, 0, 4, "Value", "Integer"});
    prototype.arguments.push_back({0x22, false, 8, 4, "Other", "Pointer"});
    prototype.locals.push_back({-4, 4, "Temp", "Integer"});

    idr::core::LegacyProcedureMetadataSeed seed;
    if (!idr::core::BuildLegacyProcedureMetadataSeed(prototype, ikFunc, seed)) return 52;

    InfoRec legacyRecord(-1, ikProc);
    if (!legacyRecord.procInfo) return 53;
    legacyRecord.procInfo->procSize = 77;
    if (!idr::core::ApplyLegacyProcedureMetadataSeed(legacyRecord, seed)) return 54;
    if (legacyRecord.kind != ikFunc || legacyRecord.type != "Integer") return 55;
    if (legacyRecord.procInfo->flags != prototype.flags ||
        legacyRecord.procInfo->bpBase != 8 ||
        legacyRecord.procInfo->retBytes != 4 ||
        legacyRecord.procInfo->stackSize != 64 ||
        legacyRecord.procInfo->procSize != 77) return 56;
    if (!legacyRecord.procInfo->args || legacyRecord.procInfo->args->Count != 2) return 57;
    PARGINFO firstArg = legacyRecord.procInfo->GetArg(0);
    PARGINFO secondArg = legacyRecord.procInfo->GetArg(1);
    if (!firstArg || !secondArg || !firstArg->Register || secondArg->Register ||
        firstArg->Ndx != 0 || secondArg->Ndx != 8 ||
        firstArg->TypeDef != "Integer" || secondArg->TypeDef != "Pointer") return 58;
    PLOCALINFO local = legacyRecord.procInfo->GetLocal(-4);
    if (!local || local->Name != "Temp" || local->TypeDef != "Integer") return 59;
    if (idr::core::ApplyLegacyProcedureMetadataSeed(legacyRecord, seed)) return 60;

    idr::core::ProcedurePrototypeMetadata captured;
    if (!idr::core::CaptureLegacyProcedurePrototypeMetadata(legacyRecord, captured)) return 61;
    if (captured.kind != prototype.kind || captured.returnType != prototype.returnType ||
        captured.flags != prototype.flags || captured.bpBase != prototype.bpBase ||
        captured.retBytes != prototype.retBytes || captured.stackSize != prototype.stackSize ||
        captured.arguments.size() != prototype.arguments.size() ||
        captured.locals.size() != prototype.locals.size()) return 62;
    for (std::size_t i = 0; i < prototype.arguments.size(); ++i) {
        const auto &expected = prototype.arguments[i];
        const auto &actual = captured.arguments[i];
        if (actual.tag != expected.tag || actual.inRegister != expected.inRegister ||
            actual.index != expected.index || actual.size != expected.size ||
            actual.name != expected.name || actual.type != expected.type) return 63;
    }
    for (std::size_t i = 0; i < prototype.locals.size(); ++i) {
        const auto &expected = prototype.locals[i];
        const auto &actual = captured.locals[i];
        if (actual.offset != expected.offset || actual.size != expected.size ||
            actual.name != expected.name || actual.type != expected.type) return 64;
    }
    if (legacyRecord.procInfo->procSize != 77) return 65;

    InfoRec nonProcedure(-1, ikInteger);
    idr::core::ProcedurePrototypeMetadata rejected;
    if (idr::core::CaptureLegacyProcedurePrototypeMetadata(nonProcedure, rejected)) return 66;
    if (rejected.kind != 0 || !rejected.returnType.empty() ||
        !rejected.arguments.empty() || !rejected.locals.empty()) return 67;

    const auto op = disasm.GetOp(const_cast<char *>("mov"));
    std::cout << "portable-core link probe: OP_MOV=" << static_cast<int>(op)
              << ", decompiler-condition=" << idr::core::DirectCondition('E')
              << ", name=" << idr::core::DefaultProcName(0x401000)
              << ", fs=" << idr::core::GetNearestUpPrefixFs(decoded, disasm, 8)
              << ", segmented=00402002->6"
              << ", legacy-procedure-seed=ok"
              << ", legacy-prototype-roundtrip=ok\n";
    return op == OP_MOV ? 0 : 68;
}
