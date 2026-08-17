#include "IdrCoreServices.h"

namespace idr::core {

Services MakeHeadlessServices() {
    Services services;
    services.confirmEmbeddedProcedure = [](DWord) { return false; };
    services.lookupMethod = [](DWord, int) -> std::optional<MethodInfo> { return std::nullopt; };
    services.manualInput = [](DWord, DWord, const std::string &, const std::string &) -> std::optional<std::string> {
        return std::nullopt;
    };
    return services;
}

} // namespace idr::core
