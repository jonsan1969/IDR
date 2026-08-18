#pragma once

#include "IdrCoreTypes.h"

namespace idr::core {

struct LegacyDecompilerPreflightResult {
    int procedureSize = 0;
    DWord stackSize = 0;
    bool bpBased = false;
    bool initialized = false;
};

bool PreflightActiveLegacyProcedure(DWord address,
                                    LegacyDecompilerPreflightResult &result);

} // namespace idr::core
