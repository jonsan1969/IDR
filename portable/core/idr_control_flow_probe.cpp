#include "IdrProcedureAnalysis.h"

#include <iostream>
#include <vector>

namespace {

using namespace idr::core;

bool RunEstablishedFixture() {
    constexpr DWord kBase = 0x00401000u;
    constexpr DWord kBranchTarget = kBase + 0x09u;
    constexpr DWord kTargetA = kBase + 0x10u;
    constexpr DWord kTargetB = kBase + 0x20u;

    std::vector<Byte> image(0x40, 0);
    AnalysisState analysis(image.size());

    const AddressMapper addressToOffset = [&](DWord address) {
        if (address < kBase) return -1;
        const DWord offset = address - kBase;
        return offset < image.size() ? static_cast<int>(offset) : -1;
    };

    const InstructionDecoder decoder = [](DWord address, DecodedInstruction &decoded) {
        constexpr DWord base = 0x00401000u;
        constexpr DWord branchTarget = base + 0x09u;
        constexpr DWord targetA = base + 0x10u;
        constexpr DWord targetB = base + 0x20u;
        decoded = {};
        switch (address) {
            case base:
                decoded = {5, "call", "call TargetA", true, false, false, false, targetA};
                return true;
            case base + 5:
                decoded = {2, "jz", "jz BranchTarget", false, true, true, false, branchTarget};
                return true;
            case base + 7:
                decoded = {1, "nop", "nop", false, false, false, false, 0};
                return true;
            case base + 8:
                decoded = {1, "ret", "ret", false, false, false, true, 0};
                return true;
            case branchTarget:
                decoded = {1, "ret", "ret", false, false, false, true, 0};
                return true;
            case targetA:
                decoded = {5, "call", "call TargetB", true, false, false, false, targetB};
                return true;
            case targetA + 5:
                decoded = {5, "call", "call TargetB", true, false, false, false, targetB};
                return true;
            case targetA + 10:
                decoded = {1, "ret", "ret", false, false, false, true, 0};
                return true;
            case targetB:
                decoded = {1, "ret", "ret", false, false, false, true, 0};
                return true;
            default:
                return false;
        }
    };

    ControlFlowResult result;
    if (!AnalyzeBoundedControlFlow(kBase, analysis, decoder, addressToOffset, {8, 8, 8}, result)) {
        std::cerr << "control-flow probe failed: " << result.error << '\n';
        return false;
    }

    if (result.entryBlocks.size() != 3 ||
        result.entryTrace.size() != 5 ||
        result.candidates.size() != 2 ||
        result.discoveredCandidateCount != 2 ||
        result.edges.size() != 5 ||
        result.callXrefs.size() != 3 ||
        result.procedures.size() != 3) {
        std::cerr << "control-flow probe produced unexpected established graph shape\n";
        return false;
    }

    const auto &entry = result.procedures[0];
    const auto &targetA = result.procedures[1];
    const auto &targetB = result.procedures[2];
    if (entry.address != kBase || entry.blockCount != 3 || entry.instructionCount != 5 ||
        entry.callEdgeCount != 1 || entry.branchTakenEdgeCount != 1 || entry.fallThroughEdgeCount != 1 ||
        entry.incomingCallCount != 0 || entry.observedStart != kBase ||
        entry.observedEndExclusive != kBase + 10 || entry.observedSpan != 10 ||
        targetA.address != kTargetA || targetA.blockCount != 1 || targetA.instructionCount != 3 ||
        targetA.callEdgeCount != 2 || targetA.branchTakenEdgeCount != 0 || targetA.fallThroughEdgeCount != 0 ||
        targetA.incomingCallCount != 1 || targetA.observedStart != kTargetA ||
        targetA.observedEndExclusive != kTargetA + 11 || targetA.observedSpan != 11 ||
        targetB.address != kTargetB || targetB.blockCount != 1 || targetB.instructionCount != 1 ||
        targetB.callEdgeCount != 0 || targetB.branchTakenEdgeCount != 0 || targetB.fallThroughEdgeCount != 0 ||
        targetB.incomingCallCount != 2 || targetB.observedStart != kTargetB ||
        targetB.observedEndExclusive != kTargetB + 1 || targetB.observedSpan != 1) {
        std::cerr << "control-flow probe produced unexpected established summaries\n";
        return false;
    }

    ProcedureAnalysisInput entryInput;
    ProcedureAnalysisInput targetAInput;
    ProcedureAnalysisInput targetBInput;
    ProcedureAnalysisInput missingInput;
    if (!BuildProcedureAnalysisInput(result, kBase, entryInput) ||
        !BuildProcedureAnalysisInput(result, kTargetA, targetAInput) ||
        !BuildProcedureAnalysisInput(result, kTargetB, targetBInput) ||
        BuildProcedureAnalysisInput(result, kBase + 0x30u, missingInput)) {
        std::cerr << "procedure analysis input construction failed\n";
        return false;
    }

    if (entryInput.summary.address != kBase || entryInput.instructions.size() != 5 ||
        entryInput.blocks.size() != 3 || entryInput.edges.size() != 3 ||
        !entryInput.incomingCalls.empty() || entryInput.outgoingCalls.size() != 1 ||
        entryInput.outgoingCalls[0].callee != kTargetA ||
        targetAInput.summary.address != kTargetA || targetAInput.instructions.size() != 3 ||
        targetAInput.blocks.size() != 1 || targetAInput.edges.size() != 2 ||
        targetAInput.incomingCalls.size() != 1 || targetAInput.incomingCalls[0].caller != kBase ||
        targetAInput.outgoingCalls.size() != 2 ||
        targetAInput.outgoingCalls[0].callee != kTargetB || targetAInput.outgoingCalls[1].callee != kTargetB ||
        targetBInput.summary.address != kTargetB || targetBInput.instructions.size() != 1 ||
        targetBInput.blocks.size() != 1 || !targetBInput.edges.empty() ||
        targetBInput.incomingCalls.size() != 2 || !targetBInput.outgoingCalls.empty()) {
        std::cerr << "procedure analysis input produced unexpected established payload\n";
        return false;
    }

    const auto branchOffset = static_cast<std::size_t>(addressToOffset(kBranchTarget));
    const auto entryOffset = static_cast<std::size_t>(addressToOffset(kBase));
    const auto aOffset = static_cast<std::size_t>(addressToOffset(kTargetA));
    const auto bOffset = static_cast<std::size_t>(addressToOffset(kTargetB));
    const auto requiredTargetFlags = CodeFlags::Loc | CodeFlags::Instruction | CodeFlags::Code;
    const auto requiredProcedureFlags = requiredTargetFlags | CodeFlags::ProcStart;
    if (!analysis.IsFlagSet(CodeFlags::Call, entryOffset) ||
        !analysis.IsFlagSet(CodeFlags::ProcStart, entryOffset) ||
        (analysis.Flags()[branchOffset] & requiredTargetFlags) != requiredTargetFlags ||
        analysis.IsFlagSet(CodeFlags::ProcStart, branchOffset) ||
        (analysis.Flags()[aOffset] & requiredProcedureFlags) != requiredProcedureFlags ||
        (analysis.Flags()[bOffset] & requiredProcedureFlags) != requiredProcedureFlags) {
        std::cerr << "control-flow probe produced unexpected established flags\n";
        return false;
    }

    std::cout << "neutral-control-flow=ok\n";
    std::cout << "entry-block-count=" << result.entryBlocks.size() << '\n';
    std::cout << "candidate-count=" << result.candidates.size() << '\n';
    std::cout << "procedure-summary-count=" << result.procedures.size() << '\n';
    std::cout << "call-xref-count=" << result.callXrefs.size() << '\n';
    std::cout << "edge-count=" << result.edges.size() << '\n';
    std::cout << "procedure-analysis-input=ok\n";
    return true;
}

bool RunRichGraphFixture() {
    constexpr DWord kBase = 0x00402000u;
    constexpr DWord kPathA = kBase + 0x10u;
    constexpr DWord kJoin = kBase + 0x20u;
    constexpr DWord kLoopBody = kBase + 0x30u;
    constexpr DWord kInvalidTarget = kBase + 0x80u;

    std::vector<Byte> image(0x40, 0);
    AnalysisState analysis(image.size());

    const AddressMapper addressToOffset = [&](DWord address) {
        if (address < kBase) return -1;
        const DWord offset = address - kBase;
        return offset < image.size() ? static_cast<int>(offset) : -1;
    };

    const InstructionDecoder decoder = [](DWord address, DecodedInstruction &decoded) {
        constexpr DWord base = 0x00402000u;
        constexpr DWord pathA = base + 0x10u;
        constexpr DWord join = base + 0x20u;
        constexpr DWord loopBody = base + 0x30u;
        constexpr DWord invalidTarget = base + 0x80u;
        decoded = {};
        switch (address) {
            case base:
                decoded = {2, "jz", "jz PathA", false, true, true, false, pathA};
                return true;
            case base + 2:
                decoded = {2, "jmp", "jmp Join", false, true, false, false, join};
                return true;
            case pathA:
                decoded = {2, "jmp", "jmp Join", false, true, false, false, join};
                return true;
            case join:
                decoded = {2, "jnz", "jnz LoopBody", false, true, true, false, loopBody};
                return true;
            case join + 2:
                decoded = {5, "call", "call InvalidTarget", true, false, false, false, invalidTarget};
                return true;
            case join + 7:
                decoded = {2, "jmp", "jmp InvalidTarget", false, true, false, false, invalidTarget};
                return true;
            case loopBody:
                decoded = {2, "jmp", "jmp Join", false, true, false, false, join};
                return true;
            default:
                return false;
        }
    };

    ControlFlowResult result;
    if (!AnalyzeBoundedControlFlow(kBase, analysis, decoder, addressToOffset, {8, 8, 8}, result)) {
        std::cerr << "rich control-flow probe failed: " << result.error << '\n';
        return false;
    }

    std::size_t callEdges = 0;
    std::size_t branchTakenEdges = 0;
    std::size_t fallThroughEdges = 0;
    std::size_t joinIncomingEdges = 0;
    std::size_t backEdges = 0;
    std::size_t invalidEdges = 0;
    for (const auto &edge : result.edges) {
        switch (edge.kind) {
            case ControlFlowEdgeKind::Call: ++callEdges; break;
            case ControlFlowEdgeKind::BranchTaken: ++branchTakenEdges; break;
            case ControlFlowEdgeKind::FallThrough: ++fallThroughEdges; break;
        }
        if (edge.to == kJoin) ++joinIncomingEdges;
        if (edge.from == kLoopBody && edge.to == kJoin && edge.kind == ControlFlowEdgeKind::BranchTaken) ++backEdges;
        if (edge.to == kInvalidTarget) ++invalidEdges;
    }

    if (result.entryBlocks.size() != 6 || result.entryTrace.size() != 7 ||
        !result.candidates.empty() || result.discoveredCandidateCount != 0 ||
        result.edges.size() != 7 || callEdges != 0 || branchTakenEdges != 5 ||
        fallThroughEdges != 2 || joinIncomingEdges != 3 || backEdges != 1 || invalidEdges != 0 ||
        !result.callXrefs.empty() || result.procedures.size() != 1) {
        std::cerr << "rich control-flow probe produced unexpected graph shape\n";
        return false;
    }

    const auto &summary = result.procedures[0];
    if (summary.address != kBase || summary.blockCount != 6 || summary.instructionCount != 7 ||
        summary.callEdgeCount != 0 || summary.branchTakenEdgeCount != 5 || summary.fallThroughEdgeCount != 2 ||
        summary.incomingCallCount != 0 || summary.observedStart != kBase ||
        summary.observedEndExclusive != kLoopBody + 2 || summary.observedSpan != 0x32u) {
        std::cerr << "rich control-flow probe produced unexpected procedure summary\n";
        return false;
    }

    ProcedureAnalysisInput input;
    if (!BuildProcedureAnalysisInput(result, kBase, input) ||
        input.summary.address != kBase || input.instructions.size() != 7 || input.blocks.size() != 6 ||
        input.edges.size() != 7 || !input.incomingCalls.empty() || !input.outgoingCalls.empty()) {
        std::cerr << "rich procedure analysis input produced unexpected payload\n";
        return false;
    }

    const auto baseOffset = static_cast<std::size_t>(addressToOffset(kBase));
    const auto pathAOffset = static_cast<std::size_t>(addressToOffset(kPathA));
    const auto joinOffset = static_cast<std::size_t>(addressToOffset(kJoin));
    const auto loopOffset = static_cast<std::size_t>(addressToOffset(kLoopBody));
    const auto invalidCallOffset = static_cast<std::size_t>(addressToOffset(kJoin + 2));
    if (!analysis.IsFlagSet(CodeFlags::ProcStart, baseOffset) ||
        analysis.IsFlagSet(CodeFlags::ProcStart, pathAOffset) ||
        analysis.IsFlagSet(CodeFlags::ProcStart, joinOffset) ||
        analysis.IsFlagSet(CodeFlags::ProcStart, loopOffset) ||
        !analysis.IsFlagSet(CodeFlags::Loc, pathAOffset) ||
        !analysis.IsFlagSet(CodeFlags::Loc, joinOffset) ||
        !analysis.IsFlagSet(CodeFlags::Loc, loopOffset) ||
        !analysis.IsFlagSet(CodeFlags::Call, invalidCallOffset)) {
        std::cerr << "rich control-flow probe produced unexpected flags\n";
        return false;
    }

    std::cout << "rich-control-flow=ok\n";
    std::cout << "rich-block-count=" << result.entryBlocks.size() << '\n';
    std::cout << "rich-edge-count=" << result.edges.size() << '\n';
    std::cout << "rich-join-incoming-edge-count=" << joinIncomingEdges << '\n';
    std::cout << "rich-back-edge-count=" << backEdges << '\n';
    std::cout << "rich-procedure-analysis-input=ok\n";
    return true;
}

bool RunPrototypeMetadataFixture() {
    constexpr Byte kProcedureKind = 1;
    constexpr Byte kFunctionKind = 2;

    ProcedurePrototypeMetadata procedure;
    procedure.kind = kProcedureKind;
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) return false;

    ProcedurePrototypeMetadata function;
    function.kind = kFunctionKind;
    if (IsProcedurePrototypeComplete(function, kFunctionKind)) return false;
    function.returnType = "Integer";
    if (!IsProcedurePrototypeComplete(function, kFunctionKind)) return false;

    ProcedureArgumentMetadata argument;
    argument.tag = 0x21;
    argument.inRegister = true;
    argument.index = 0;
    argument.size = 4;
    argument.name = "Value";
    procedure.arguments.push_back(argument);
    if (IsProcedurePrototypeComplete(procedure, kFunctionKind)) return false;

    procedure.arguments[0].type = "Integer";
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) return false;

    ProcedureArgumentMetadata secondArgument;
    secondArgument.tag = 0x22;
    secondArgument.inRegister = false;
    secondArgument.index = 8;
    secondArgument.size = 4;
    secondArgument.name = "Other";
    secondArgument.type = "Pointer";
    procedure.arguments.push_back(secondArgument);
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) return false;

    procedure.arguments[1].type.clear();
    if (IsProcedurePrototypeComplete(procedure, kFunctionKind)) return false;

    std::cout << "procedure-prototype-metadata=ok\n";
    std::cout << "procedure-argument-count=" << procedure.arguments.size() << '\n';
    return true;
}

} // namespace

int main() {
    if (!RunEstablishedFixture()) return 1;
    if (!RunRichGraphFixture()) return 2;
    if (!RunPrototypeMetadataFixture()) return 3;
    return 0;
}
