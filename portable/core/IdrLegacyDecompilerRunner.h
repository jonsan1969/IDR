#pragma once

#include "IdrDecompilerResult.h"

namespace idr::core {

struct LegacyDecompilerPreflightResult {
    int procedureSize = 0;
    ProcedureSizeSource procedureSizeSource = ProcedureSizeSource::None;
    DWord stackSize = 0;
    bool bpBased = false;
    bool initialized = false;
};

// Compatibility name for the legacy adapter entry point. The payload itself
// is the neutral portable result boundary.
using LegacyDecompilerRunResult = ProcedureDecompileResult;

bool PreflightActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerPreflightResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

bool DecompileActiveLegacyProcedure(
    DWord address,
    ProcedureDecompileResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

bool DecompileActiveLegacyProcedureSource(
    DWord address,
    ProcedureSourceResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

} // namespace idr::core
