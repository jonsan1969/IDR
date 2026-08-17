#pragma once

#include "IdrCoreTypes.h"

#include <cstddef>
#include <vector>

namespace idr::core {

class AnalysisState {
public:
    explicit AnalysisState(std::size_t size = 0);

    void Resize(std::size_t size);
    std::size_t Size() const;

    bool IsFlagSet(DWord flag, std::size_t pos) const;
    bool SetFlag(DWord flag, std::size_t pos);
    bool SetFlags(DWord flag, std::size_t pos, std::size_t count);
    bool ClearFlag(DWord flag, std::size_t pos);
    bool ClearFlags(DWord flag, std::size_t pos, std::size_t count);

    const std::vector<DWord> &Flags() const;

private:
    bool IsRangeValid(std::size_t pos, std::size_t count) const;

    std::vector<DWord> flags_;
};

} // namespace idr::core
