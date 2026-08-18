#include "IdrControlFlow.h"

#include <iostream>
#include <vector>

int main() {
    using namespace idr::core;

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
        return 1;
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
        return 2;
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
        return 3;
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
        return 4;
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
        return 5;
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
    return 0;
}
