#include "IdrAnalysisState.h"

namespace idr::core {

AnalysisState::AnalysisState(std::size_t size) : flags_(size, 0) {}

void AnalysisState::Resize(std::size_t size) {
    flags_.resize(size, 0);
}

std::size_t AnalysisState::Size() const {
    return flags_.size();
}

bool AnalysisState::IsFlagSet(DWord flag, std::size_t pos) const {
    return pos < flags_.size() && (flags_[pos] & flag) != 0;
}

bool AnalysisState::SetFlag(DWord flag, std::size_t pos) {
    if (pos >= flags_.size()) return false;
    flags_[pos] |= flag;
    return true;
}

bool AnalysisState::SetFlags(DWord flag, std::size_t pos, std::size_t count) {
    if (!IsRangeValid(pos, count)) return false;
    for (std::size_t i = pos; i < pos + count; ++i) flags_[i] |= flag;
    return true;
}

bool AnalysisState::ClearFlag(DWord flag, std::size_t pos) {
    if (pos >= flags_.size()) return false;
    flags_[pos] &= ~flag;
    return true;
}

bool AnalysisState::ClearFlags(DWord flag, std::size_t pos, std::size_t count) {
    if (!IsRangeValid(pos, count)) return false;
    for (std::size_t i = pos; i < pos + count; ++i) flags_[i] &= ~flag;
    return true;
}

const std::vector<DWord> &AnalysisState::Flags() const {
    return flags_;
}

bool AnalysisState::IsRangeValid(std::size_t pos, std::size_t count) const {
    if (pos > flags_.size()) return false;
    return count <= flags_.size() - pos;
}

} // namespace idr::core
