#include "IdrLegacyBridge.h"

#include "IdrAnalysis.h"
#include "IdrImageContext.h"
#include "IdrInstructionNav.h"
#include "IdrLegacyCompat.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace {
idr::core::AnalysisState fallbackState;
idr::core::AnalysisState *activeState = &fallbackState;

void EnsureFallbackSize() {
    if (activeState != &fallbackState) return;
    const auto size = idr::core::GetImageView().size;
    if (fallbackState.Size() != size) fallbackState.Resize(size);
}

char LowerAscii(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

String LowerAsciiCopy(const String &value) {
    String result = value;
    std::transform(result.begin(), result.end(), result.begin(), LowerAscii);
    return result;
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

// Transitional legacy session globals. These are deliberately kept in the
// bridge while the real loader/session object is being extracted from Main.cpp.
int DelphiVersion = 0;
MDisasm Disasm;
DWord CurProcAdr = 0;
TStringList *BSSInfos = nullptr;
int cVmtSelfPtr = 0;

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

String __fastcall ExtractClassName(const String &name) {
    return idr::core::ExtractClassName(name);
}

String __fastcall ExtractProcName(const String &name) {
    return idr::core::ExtractProcName(name);
}

String __fastcall ExtractName(const String &name) {
    return idr::core::ExtractName(name);
}

String __fastcall ExtractType(const String &name) {
    return idr::core::ExtractType(name);
}

String __fastcall TrimTypeName(const String &name) {
    return idr::core::TrimTypeName(name);
}

String __fastcall GetDefaultProcName(DWord address) {
    return idr::core::DefaultProcName(address);
}

String __fastcall MakeGvarName(DWord address) {
    return idr::core::GlobalVarName(address);
}

// Small BCB/System RTL compatibility surface reached by the real Decompiler.
String __fastcall IntToStr(__int64 value) {
    return std::to_string(value);
}

String __fastcall IntToHex(__int64 value, int digits) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0');
    if (digits > 0) out << std::setw(digits);
    out << static_cast<unsigned long long>(value);
    return out.str();
}

String __fastcall QuotedStr(const String &value) {
    String result;
    result.reserve(value.size() + 2);
    result.push_back('\'');
    for (char ch : value) {
        result.push_back(ch);
        if (ch == '\'') result.push_back('\'');
    }
    result.push_back('\'');
    return result;
}

String __fastcall AnsiReplaceText(const String &text, const String &from, const String &to) {
    if (from.empty()) return text;

    String result;
    String lowerText = LowerAsciiCopy(text);
    const String lowerFrom = LowerAsciiCopy(from);
    std::size_t cursor = 0;

    while (cursor < text.size()) {
        const auto found = lowerText.find(lowerFrom, cursor);
        if (found == String::npos) {
            result.append(text, cursor, String::npos);
            break;
        }
        result.append(text, cursor, found - cursor);
        result += to;
        cursor = found + from.size();
    }
    return result;
}

bool __fastcall SameText(const String &left, const String &right) {
    return LowerAsciiCopy(left) == LowerAsciiCopy(right);
}

template <typename T>
String FloatToStr(T value) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
    return out.str();
}

template String FloatToStr<float>(float);
template String FloatToStr<double>(double);
template String FloatToStr<__int64>(__int64);
template String FloatToStr<long double>(long double);

// The headless default policy never recursively decompiles an embedded
// procedure without an explicit caller decision. This mirrors the conservative
// default used by MakeHeadlessServices().
bool PortableConfirmEmbeddedProcedure(const String &) {
    return false;
}

String PortableCurrencyToString(const Currency &value) {
    const bool negative = value.Val < 0;
    const auto magnitude = negative
        ? static_cast<unsigned long long>(-(value.Val + 1)) + 1ULL
        : static_cast<unsigned long long>(value.Val);
    const auto whole = magnitude / 10000ULL;
    auto fraction = magnitude % 10000ULL;

    std::ostringstream out;
    if (negative) out << '-';
    out << whole;
    if (fraction != 0) {
        out << '.' << std::setw(4) << std::setfill('0') << fraction;
        String formatted = out.str();
        while (!formatted.empty() && formatted.back() == '0') formatted.pop_back();
        return formatted;
    }
    return out.str();
}
