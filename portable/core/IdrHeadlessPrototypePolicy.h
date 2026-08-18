#pragma once

#include "IdrProcedureAnalysis.h"

#include <functional>

namespace idr::core {

enum class PrototypeResolutionStatus {
    Resolved,
    Unavailable,
    Rejected
};

struct PrototypeResolutionRequest {
    DWord procedureAddress = 0;
    DWord callSite = 0;
    ProcedurePrototypeMetadata current;
};

struct PrototypeResolutionResult {
    PrototypeResolutionStatus status = PrototypeResolutionStatus::Unavailable;
    ProcedurePrototypeMetadata prototype;
};

using HeadlessPrototypeResolver =
    std::function<PrototypeResolutionResult(const PrototypeResolutionRequest &)>;

inline bool ResolveProcedurePrototype(const PrototypeResolutionRequest &request,
                                      Byte functionKind,
                                      const HeadlessPrototypeResolver &resolver,
                                      ProcedurePrototypeMetadata &resolved) {
    resolved = {};

    if (IsProcedurePrototypeComplete(request.current, functionKind)) {
        resolved = request.current;
        return true;
    }
    if (!resolver) return false;

    PrototypeResolutionResult result = resolver(request);
    if (result.status != PrototypeResolutionStatus::Resolved) return false;
    if (!IsProcedurePrototypeComplete(result.prototype, functionKind)) return false;

    resolved = std::move(result.prototype);
    return true;
}

} // namespace idr::core
