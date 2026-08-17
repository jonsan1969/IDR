#include "IdrLegacyBridge.h"

#include "IdrAnalysis.h"
#include "IdrImageContext.h"
#include "IdrInstructionNav.h"

#include <string>

using idr::core::DWord;

namespace {
idr::core::AnalysisState fallbackState;
idr::core::AnalysisState *activeState = &fallbackState;

void EnsureFallbackSize() {
    if (activeState != &fallbackState) return;
    const auto size = idr::core::GetImageView().size;
    if (fallbackState.Size() != size) fallbackState.Resize(size);
}
}

namespace idr::core {

void SetLegacyAnalysisState(AnalysisState *state) {
    activeState = state ? state : &fallbackState;
    EnsureFallbackSize();
}

AnalysisState &LegacyAnalysisState() {
    EnsureFallbackSize();
    return *activeState;
}

} // namespace idr::core

// Legacy global API adapters. Keep __fastcall so the x86 symbol decoration
// matches the declarations emitted from Misc.portable.h.
bool __fastcall IsFlagSet(DWord flag, int pos) {
    if (pos < 0) return false;
    return idr::core::LegacyAnalysisState().IsFlagSet(flag, static_cast<std::size_t>(pos));
}

void __fastcall SetFlag(DWord flag, int pos) {
    if (pos < 0) return;
    idr::core::LegacyAnalysisState().SetFlag(flag, static_cast<std::size_t>(pos));
}

void __fastcall SetFlags(DWord flag, int pos, int num) {
    if (pos < 0 || num < 0) return;
    idr::core::LegacyAnalysisState().SetFlags(flag,
                                              static_cast<std::size_t>(pos),
                                              static_cast<std::size_t>(num));
}

void __fastcall ClearFlag(DWord flag, int pos) {
    if (pos < 0) return;
    idr::core::LegacyAnalysisState().ClearFlag(flag, static_cast<std::size_t>(pos));
}

DWord __fastcall Pos2Adr(int pos) {
    if (pos < 0) return 0;
    const auto address = idr::core::OffsetToAddress(static_cast<std::size_t>(pos));
    return address.value_or(0);
}

int __fastcall GetNearestUpInstruction(int pos) {
    return idr::core::GetNearestUpInstruction(idr::core::LegacyAnalysisState(), pos);
}

int __fastcall GetNearestUpInstruction(int pos, int toPos) {
    return idr::core::GetNearestUpInstruction(idr::core::LegacyAnalysisState(), pos, toPos);
}

std::string __fastcall ExtractClassName(const std::string &name) {
    return idr::core::ExtractClassName(name);
}

std::string __fastcall ExtractProcName(const std::string &name) {
    return idr::core::ExtractProcName(name);
}

std::string __fastcall ExtractName(const std::string &name) {
    return idr::core::ExtractName(name);
}

std::string __fastcall ExtractType(const std::string &name) {
    return idr::core::ExtractType(name);
}

std::string __fastcall TrimTypeName(const std::string &name) {
    return idr::core::TrimTypeName(name);
}

std::string __fastcall GetDefaultProcName(DWord address) {
    return idr::core::DefaultProcName(address);
}

std::string __fastcall MakeGvarName(DWord address) {
    return idr::core::GlobalVarName(address);
}
