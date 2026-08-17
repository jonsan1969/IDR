#include "IdrDecompilerModel.h"

#include <array>

namespace idr::core {
namespace {
constexpr std::array<const char *, 19> kDirectConditions{{
    "", "", "<", ">=", "=", "<>", "<=", ">", "", "", "", "", "<", ">=", "<=", ">", "not in", "in", "is"
}};

constexpr std::array<const char *, 19> kInvertConditions{{
    "", "", ">=", "<", "<>", "=", ">", "<=", "", "", "", "", ">=", "<", ">", "<=", "in", "not in", "is not"
}};

std::string ConditionFromTable(const std::array<const char *, 19> &table, char condition) {
    if (condition < 'A') return "?";
    const auto index = static_cast<unsigned char>(condition - 'A');
    if (index >= table.size()) return "?";
    return table[index];
}
} // namespace

void InitDecompilerItem(DecompilerItem &item) {
    item.flags = 0;
    item.precedence = Precedence::Atom;
    item.size = 0;
    item.offset = 0;
    item.intValue = 0;
    item.value.clear();
    item.value1.clear();
    item.type.clear();
    item.name.clear();
}

void AssignDecompilerItem(DecompilerItem &destination, const DecompilerItem &source) {
    destination.flags = source.flags;
    destination.precedence = source.precedence;
    destination.size = source.size;
    destination.intValue = source.intValue;
    destination.value = source.value;
    destination.value1 = source.value1;
    destination.type = source.type;
    destination.name = source.name;
    // Legacy AssignItem deliberately does not copy Offset.
}

std::string DirectCondition(char condition) {
    return ConditionFromTable(kDirectConditions, condition);
}

std::string InvertCondition(char condition) {
    return ConditionFromTable(kInvertConditions, condition);
}

} // namespace idr::core
