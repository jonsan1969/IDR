#pragma once

#include "IdrCoreTypes.h"

#include <string>

namespace idr::core {

// Neutral analysis/name helpers extracted from the non-UI portion of Misc.cpp.
// These preserve the observable legacy IDR naming/hex-format semantics while
// avoiding any VCL dependency.
std::string Hex(DWord value, unsigned minDigits = 0);
std::string DefaultProcName(DWord address);
std::string GlobalVarName(DWord address);
bool CanReplaceTypeName(const std::string &fromName, const std::string &toName);

} // namespace idr::core
