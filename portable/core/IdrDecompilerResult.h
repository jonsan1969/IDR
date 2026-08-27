#pragma once

#include "IdrCoreTypes.h"
#include "IdrProcedureSizePolicy.h"

#include <string>
#include <vector>

namespace idr::core {

// Neutral result boundary for one completed decompiler execution. Legacy
// engine/container types stay behind the adapter; callers receive portable
// scalar state and ordinary source lines only.
struct ProcedureDecompileResult {
    DWord procedureAddress = 0;
    int procedureSize = 0;
    ProcedureSizeSource procedureSizeSource = ProcedureSizeSource::None;
    DWord endAddress = 0;
    bool wasRet = false;
    bool completed = false;
    std::vector<std::string> body;
};

} // namespace idr::core
