#include "IdrControlFlow.h"

#include <iostream>
#include <vector>

int main() {
    using namespace idr::core;

    constexpr DWord kBase = 0x00401000u;
    constexpr DWord kTargetA = kBase + 0x10u;
    constexpr DWord kTargetB = kBase + 0x20u;

    std::vector<Byte> image(0x40, 0);
    SetImageSegments({image.data(), image.size(), kBase}, {{kBase, static_cast<DWord>(image.size()), 0}});
    AnalysisState analysis(image.size());

    const InstructionDecoder decoder = [](DWord address, DecodedInstruction &decoded) {
        constexpr DWord base = 0x00401000u;
        constexpr DWord targetA = base + 0x10u;
        constexpr DWord targetB = base + 0x20u;
        decoded = {};
        switch (address) {
            case base:
                decoded = {5, "call", "call TargetA", true, false, false, false, targetA};
                return true;
            case base + 5:
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
    if (!AnalyzeBoundedControlFlow(kBase, analysis, decoder, {8, 8}, result)) {
        std::cerr << "control-flow probe failed: " << result.error << '\n';
        return 1;
    }

    if (result.entryTrace.size() != 2 ||
        result.candidates.size() != 2 ||
        result.discoveredCandidateCount != 2 ||
        result.edges.size() != 3) {
        std::cerr << "control-flow probe produced unexpected graph shape\n";
        return 2;
    }

    const auto aOffset = static_cast<std::size_t>(AddressToOffset(kTargetA));
    const auto bOffset = static_cast<std::size_t>(AddressToOffset(kTargetB));
    const auto entryOffset = static_cast<std::size_t>(AddressToOffset(kBase));
    const auto requiredTargetFlags = CodeFlags::Loc | CodeFlags::Instruction | CodeFlags::Code;

    if (!analysis.IsFlagSet(CodeFlags::Call, entryOffset) ||
        !analysis.IsFlagSet(CodeFlags::Call, aOffset) ||
        (analysis.Flags()[aOffset] & requiredTargetFlags) != requiredTargetFlags ||
        (analysis.Flags()[bOffset] & requiredTargetFlags) != requiredTargetFlags) {
        std::cerr << "control-flow probe produced unexpected analysis flags\n";
        return 3;
    }

    std::cout << "neutral-control-flow=ok\n";
    std::cout << "candidate-count=" << result.candidates.size() << '\n';
    std::cout << "edge-count=" << result.edges.size() << '\n';
    return 0;
}
