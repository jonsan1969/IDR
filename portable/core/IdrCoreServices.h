#pragma once

#include "IdrCoreTypes.h"

#include <functional>
#include <optional>
#include <string>

namespace idr::core {

struct MethodInfo {
    DWord address = 0;
    std::string name;
};

struct Services {
    std::function<bool(DWord procedureAddress)> confirmEmbeddedProcedure;
    std::function<std::optional<MethodInfo>(DWord vmtAddress, int methodIndex)> lookupMethod;
    std::function<std::optional<std::string>(DWord procedureAddress,
                                             DWord currentAddress,
                                             const std::string &caption,
                                             const std::string &label)> manualInput;
};

Services MakeHeadlessServices();

} // namespace idr::core
