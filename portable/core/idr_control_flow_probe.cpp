#include "IdrControlFlow.h"

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
                decoded = {2, "jz", "jz branchTarget", false, true, true, false, branchTarget};
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

    std::size_t callEdges = 0;
    std::size_t branchTakenEdges = 0;
    std::size_t fallThroughEdges = 0;
    std::size_t entryOwnedEdges = 0;
    std::size_t targetAOwnedEdges = 0;
    std::size_t targetBOwnedEdges = 0;
    std::size_t unknownOwnedEdges = 0;
    for (const auto &edge : result.edges) {
        switch (edge.kind) {
            case ControlFlowEdgeKind::Call: ++callEdges; break;
            case ControlFlowEdgeKind::BranchTaken: ++branchTakenEdges; break;
            case ControlFlowEdgeKind::FallThrough: ++fallThroughEdges; break;
        }
        if (edge.procedure == kBase)
            ++entryOwnedEdges;
        else if (edge.procedure == kTargetA)
            ++targetAOwnedEdges;
        else if (edge.procedure == kTargetB)
            ++targetBOwnedEdges;
        else
            ++unknownOwnedEdges;
    }

    if (result.entryBlocks.size() != 3 ||
        result.entryTrace.size() != 5 ||
        result.candidates.size() != 2 ||
        result.discoveredCandidateCount != 2 ||
        result.edges.size() != 5 ||
        callEdges != 3 || branchTakenEdges != 1 || fallThroughEdges != 1 ||
        entryOwnedEdges != 3 || targetAOwnedEdges != 2 || targetBOwnedEdges != 0 || unknownOwnedEdges != 0) {
        std::cerr << "control-flow probe produced unexpected graph shape\n";
        return false;
    }

    if (result.procedures.size() != 3 ||
        result.procedures[0].address != kBase ||
        result.procedures[0].blockCount != 3 ||
        result.procedures[0].instructionCount != 5 ||
        result.procedures[0].callEdgeCount != 1 ||
        result.procedures[0].branchTakenEdgeCount != 1 ||
        result.procedures[0].fallThroughEdgeCount != 1 ||
        result.procedures[0].incomingCallCount != 0 ||
        result.procedures[1].address != kTargetA ||
        result.procedures[1].blockCount != 1 ||
        result.procedures[1].instructionCount != 3 ||
        result.procedures[1].callEdgeCount != 2 ||
        result.procedures[1].branchTakenEdgeCount != 0 ||
        result.procedures[1].fallThroughEdgeCount != 0 ||
        result.procedures[1].incomingCallCount != 1 ||
        result.procedures[2].address != kTargetB ||
        result.procedures[2].blockCount != 1 ||
        result.procedures[2].instructionCount != 1 ||
        result.procedures[2].callEdgeCount != 0 ||
        result.procedures[2].branchTakenEdgeCount != 0 ||
        result.procedures[2].fallThroughEdgeCount != 0 ||
        result.procedures[2].incomingCallCount != 2) {
        std::cerr << "control-flow probe produced unexpected procedure summaries\n";
        return false;
    }

    if (result.callXrefs.size() != 3 ||
        result.callXrefs[0].caller != kBase ||
        result.callXrefs[0].callSite != kBase ||
        result.callXrefs[0].callee != kTargetA ||
        result.callXrefs[1].caller != kTargetA ||
        result.callXrefs[1].callSite != kTargetA ||
        result.callXrefs[1].callee != kTargetB ||
        result.callXrefs[2].caller != kTargetA ||
        result.callXrefs[2].callSite != kTargetA + 5 ||
        result.callXrefs[2].callee != kTargetB) {
        std::cerr << "control-flow probe produced unexpected call xrefs\n";
        return false;
    }

    const auto *entrySummary = FindProcedureSummary(result, kBase);
    const auto *targetASummary = FindProcedureSummary(result, kTargetA);
    const auto *targetBSummary = FindProcedureSummary(result, kTargetB);
    const auto *missingSummary = FindProcedureSummary(result, kBase + 0x30u);
    const auto targetBIncoming = FindIncomingCallXrefs(result, kTargetB);
    const auto targetAOutgoing = FindOutgoingCallXrefs(result, kTargetA);
    const auto entryEdges = FindProcedureEdges(result, kBase);
    const auto missingIncoming = FindIncomingCallXrefs(result, kBase + 0x30u);
    const auto missingOutgoing = FindOutgoingCallXrefs(result, kBase + 0x30u);
    const auto missingEdges = FindProcedureEdges(result, kBase + 0x30u);

    if (!entrySummary || entrySummary->address != kBase ||
        !targetASummary || targetASummary->address != kTargetA ||
        !targetBSummary || targetBSummary->address != kTargetB ||
        missingSummary != nullptr ||
        targetBIncoming.size() != 2 ||
        targetBIncoming[0].callSite != kTargetA ||
        targetBIncoming[1].callSite != kTargetA + 5 ||
        targetAOutgoing.size() != 2 ||
        targetAOutgoing[0].callee != kTargetB ||
        targetAOutgoing[1].callee != kTargetB ||
        entryEdges.size() != 3 ||
        !missingIncoming.empty() || !missingOutgoing.empty() || !missingEdges.empty()) {
        std::cerr << "control-flow probe produced unexpected query results\n";
        return false;
    }

    const auto branchOffset = static_cast<std::size_t>(addressToOffset(kBranchTarget));
    const auto aOffset = static_cast<std::size_t>(addressToOffset(kTargetA));
    const auto bOffset = static_cast<std::size_t>(addressToOffset(kTargetB));
    const auto entryOffset = static_cast<std::size_t>(addressToOffset(kBase));
    const auto requiredTargetFlags = CodeFlags::Loc | CodeFlags::Instruction | CodeFlags::Code;
    const auto requiredProcedureFlags = requiredTargetFlags | CodeFlags::ProcStart;

    if (!analysis.IsFlagSet(CodeFlags::Call, entryOffset) ||
        !analysis.IsFlagSet(CodeFlags::Call, aOffset) ||
        !analysis.IsFlagSet(CodeFlags::ProcStart, entryOffset) ||
        (analysis.Flags()[branchOffset] & requiredTargetFlags) != requiredTargetFlags ||
        analysis.IsFlagSet(CodeFlags::ProcStart, branchOffset) ||
        (analysis.Flags()[aOffset] & requiredProcedureFlags) != requiredProcedureFlags ||
        (analysis.Flags()[bOffset] & requiredProcedureFlags) != requiredProcedureFlags) {
        std::cerr << "control-flow probe produced unexpected analysis flags\n";
        return false;
    }

    std::cout << "neutral-control-flow=ok\n";
    std::cout << "entry-block-count=" << result.entryBlocks.size() << '\n';
    std::cout << "candidate-count=" << result.candidates.size() << '\n';
    std::cout << "procedure-start-count=" << (result.candidates.size() + 1) << '\n';
    std::cout << "procedure-summary-count=" << result.procedures.size() << '\n';
    std::cout << "call-xref-count=" << result.callXrefs.size() << '\n';
    std::cout << "entry-incoming-call-count=" << result.procedures[0].incomingCallCount << '\n';
    std::cout << "target-a-incoming-call-count=" << result.procedures[1].incomingCallCount << '\n';
    std::cout << "target-b-incoming-call-count=" << result.procedures[2].incomingCallCount << '\n';
    std::cout << "edge-count=" << result.edges.size() << '\n';
    std::cout << "call-edge-count=" << callEdges << '\n';
    std::cout << "branch-taken-edge-count=" << branchTakenEdges << '\n';
    std::cout << "fallthrough-edge-count=" << fallThroughEdges << '\n';
    std::cout << "entry-owned-edge-count=" << entryOwnedEdges << '\n';
    std::cout << "target-a-owned-edge-count=" << targetAOwnedEdges << '\n';
    std::cout << "target-b-owned-edge-count=" << targetBOwnedEdges << '\n';
    std::cout << "target-b-query-incoming-count=" << targetBIncoming.size() << '\n';
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
        if (edge.from == kLoopBody && edge.to == kJoin && edge.kind == ControlFlowEdgeKind::BranchTaken)
            ++backEdges;
        if (edge.to == kInvalidTarget) ++invalidEdges;
    }

    if (result.entryBlocks.size() != 6 ||
        result.entryTrace.size() != 7 ||
        result.candidates.size() != 0 ||
        result.discoveredCandidateCount != 0 ||
        result.edges.size() != 7 ||
        callEdges != 0 || branchTakenEdges != 5 || fallThroughEdges != 2 ||
        joinIncomingEdges != 3 || backEdges != 1 || invalidEdges != 0) {
        std::cerr << "rich control-flow probe produced unexpected graph shape\n";
        return false;
    }

    if (result.callXrefs.size() != 0 ||
        result.procedures.size() != 1 ||
        result.procedures[0].address != kBase ||
        result.procedures[0].blockCount != 6 ||
        result.procedures[0].instructionCount != 7 ||
        result.procedures[0].callEdgeCount != 0 ||
        result.procedures[0].branchTakenEdgeCount != 5 ||
        result.procedures[0].fallThroughEdgeCount != 2 ||
        result.procedures[0].incomingCallCount != 0) {
        std::cerr << "rich control-flow probe produced unexpected procedure summary\n";
        return false;
    }

    const auto *entrySummary = FindProcedureSummary(result, kBase);
    const auto entryEdges = FindProcedureEdges(result, kBase);
    if (!entrySummary || entrySummary->blockCount != 6 || entryEdges.size() != 7 ||
        !FindIncomingCallXrefs(result, kBase).empty() ||
        !FindOutgoingCallXrefs(result, kBase).empty()) {
        std::cerr << "rich control-flow probe produced unexpected query results\n";
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
        std::cerr << "rich control-flow probe produced unexpected analysis flags\n";
        return false;
    }

    std::cout << "rich-control-flow=ok\n";
    std::cout << "rich-block-count=" << result.entryBlocks.size() << '\n';
    std::cout << "rich-edge-count=" << result.edges.size() << '\n';
    std::cout << "rich-join-incoming-edge-count=" << joinIncomingEdges << '\n';
    std::cout << "rich-back-edge-count=" << backEdges << '\n';
    std::cout << "rich-invalid-edge-count=" << invalidEdges << '\n';
    return true;
}

} // namespace

int main() {
    if (!RunEstablishedFixture()) return 1;
    if (!RunRichGraphFixture()) return 2;
    return 0;
}
