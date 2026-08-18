#pragma once

#include "IdrCoreTypes.h"
#include "IdrProcedureSizePolicy.h"

namespace idr::core {

struct LegacyDecompilerPreflightResult {
    int procedureSize = 0;
    ProcedureSizeSource procedureSizeSource = ProcedureSizeSource::None;
    DWord stackSize = 0;
    bool bpBased = false;
    bool initialized = false;
};

bool PreflightActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerPreflightResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

} // namespace idr::core
