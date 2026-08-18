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

struct LegacyDecompilerRunResult {
    int procedureSize = 0;
    ProcedureSizeSource procedureSizeSource = ProcedureSizeSource::None;
    DWord endAddress = 0;
    bool wasRet = false;
    bool decompiled = false;
};

bool PreflightActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerPreflightResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

bool DecompileActiveLegacyProcedure(
    DWord address,
    LegacyDecompilerRunResult &result,
    const HeadlessProcedureSizeResolver &sizeResolver = {});

} // namespace idr::core
