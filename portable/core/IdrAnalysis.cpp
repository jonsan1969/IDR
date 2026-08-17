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

std::string ExtractClassName(const std::string &name) {
    if (name.empty()) return {};
    const auto pos = name.find('.');
    return pos == std::string::npos ? std::string{} : name.substr(0, pos);
}

std::string ExtractProcName(const std::string &name) {
    if (name.empty()) return {};
    const auto pos = name.find('.');
    return pos == std::string::npos ? name : name.substr(pos + 1);
}

std::string ExtractName(const std::string &name) {
    if (name.empty()) return {};
    const auto pos = name.find(':');
    return pos == std::string::npos ? name : name.substr(0, pos);
}

std::string ExtractType(const std::string &name) {
    if (name.empty()) return {};
    const auto pos = name.find(':');
    return pos == std::string::npos ? std::string{} : name.substr(pos + 1);
}

std::string TrimTypeName(const std::string &typeName) {
    if (typeName.empty()) return typeName;

    const auto pos = typeName.find('.');
    if (pos == std::string::npos || pos == 0) return typeName;
    if (pos + 1 >= typeName.size() || typeName[pos + 1] == '.') return typeName;

    for (std::size_t i = 0; i < pos; ++i) {
        const unsigned char ch = static_cast<unsigned char>(typeName[i]);
        if (ch < static_cast<unsigned char>('0') || ch == static_cast<unsigned char>('<')) {
            return typeName;
        }
    }

    return ExtractProcName(typeName);
}

} // namespace idr::core
