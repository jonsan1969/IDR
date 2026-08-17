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
    SetImageSegments({image.data(), image.size(), kBase}, {{kBase, static_cast<DWord>(image.size()), 0}});
    AnalysisState analysis(image.size());

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
    if (!AnalyzeBoundedControlFlow(kBase, analysis, decoder, {8, 8, 8}, result)) {
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

    const auto branchOffset = static_cast<std::size_t>(AddressToOffset(kBranchTarget));
    const auto aOffset = static_cast<std::size_t>(AddressToOffset(kTargetA));
    const auto bOffset = static_cast<std::size_t>(AddressToOffset(kTargetB));
    const auto entryOffset = static_cast<std::size_t>(AddressToOffset(kBase));
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
        return 3;
    }

    std::cout << "neutral-control-flow=ok\n";
    std::cout << "entry-block-count=" << result.entryBlocks.size() << '\n';
    std::cout << "candidate-count=" << result.candidates.size() << '\n';
    std::cout << "procedure-start-count=" << (result.candidates.size() + 1) << '\n';
    std::cout << "edge-count=" << result.edges.size() << '\n';
    std::cout << "call-edge-count=" << callEdges << '\n';
    std::cout << "branch-taken-edge-count=" << branchTakenEdges << '\n';
    std::cout << "fallthrough-edge-count=" << fallThroughEdges << '\n';
    std::cout << "entry-owned-edge-count=" << entryOwnedEdges << '\n';
    std::cout << "target-a-owned-edge-count=" << targetAOwnedEdges << '\n';
    std::cout << "target-b-owned-edge-count=" << targetBOwnedEdges << '\n';
    return 0;
}
