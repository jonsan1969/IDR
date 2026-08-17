#include "IdrAnalysis.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace idr::core {
namespace {
std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
} // namespace

std::string Hex(DWord value, unsigned minDigits) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0');
    if (minDigits != 0) out << std::setw(static_cast<int>(minDigits));
    out << value;
    return out.str();
}

std::string DefaultProcName(DWord address) {
    return "sub_" + Hex(address, 8);
}

std::string GlobalVarName(DWord address) {
    return "gvar_" + Hex(address, 8);
}

bool CanReplaceTypeName(const std::string &fromName, const std::string &toName) {
    if (toName.empty()) return false;
    if (fromName.empty()) return true;

    const auto lowered = Lower(fromName);
    return lowered == "byte" || lowered == "word" || lowered == "dword";
}

} // namespace idr::core
