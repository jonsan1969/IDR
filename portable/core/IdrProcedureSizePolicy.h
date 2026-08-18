#pragma once

#include "IdrCoreTypes.h"

#include <functional>

namespace idr::core {

enum class ProcedureSizeResolutionStatus {
    Resolved,
    Unavailable,
    Rejected
};

enum class ProcedureSizeSource {
    None,
    LegacyMetadata,
    HeadlessResolver
};

struct ProcedureSizeResolutionRequest {
    DWord procedureAddress = 0;
    int storedSize = 0;
};

struct ProcedureSizeResolutionResult {
    ProcedureSizeResolutionStatus status = ProcedureSizeResolutionStatus::Unavailable;
    int size = 0;
};

struct ResolvedProcedureSize {
    int size = 0;
    ProcedureSizeSource source = ProcedureSizeSource::None;
};

using HeadlessProcedureSizeResolver =
    std::function<ProcedureSizeResolutionResult(const ProcedureSizeResolutionRequest &)>;

inline bool ResolveProcedureSize(const ProcedureSizeResolutionRequest &request,
                                 const HeadlessProcedureSizeResolver &resolver,
                                 ResolvedProcedureSize &resolved) {
    resolved = {};

    if (request.storedSize > 0) {
        resolved.size = request.storedSize;
        resolved.source = ProcedureSizeSource::LegacyMetadata;
        return true;
    }
    if (request.storedSize < 0 || !resolver) return false;

    const auto candidate = resolver(request);
    if (candidate.status != ProcedureSizeResolutionStatus::Resolved || candidate.size <= 0)
        return false;

    resolved.size = candidate.size;
    resolved.source = ProcedureSizeSource::HeadlessResolver;
    return true;
}

} // namespace idr::core
