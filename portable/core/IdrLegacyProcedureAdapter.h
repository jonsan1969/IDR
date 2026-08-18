#pragma once

#include "IdrProcedureAnalysis.h"
#include "IdrLegacyCompat.h"
#include "IdrLegacyBridge.h"
#include "IdrImageContext.h"

#include <functional>
#include <utility>
#include <vector>

extern PInfoRec *Infos;

namespace idr::core {

// First controlled mutating bridge into the real legacy procedure metadata.
// The target must already be a procedure-style InfoRec with an empty procInfo
// argument/local surface. procSize is intentionally preserved untouched.
inline bool ApplyLegacyProcedureMetadataSeed(InfoRec &record,
                                             const LegacyProcedureMetadataSeed &seed) {
    if (seed.kind < ikRefine || seed.kind > ikFunc) return false;
    if (record.kind < ikRefine || record.kind > ikFunc || !record.procInfo) return false;
    if (record.procInfo->args || record.procInfo->locals) return false;

    record.kind = seed.kind;
    record.type = seed.returnType;
    record.procInfo->flags = seed.flags;
    record.procInfo->bpBase = seed.bpBase;
    record.procInfo->retBytes = seed.retBytes;
    record.procInfo->stackSize = seed.stackSize;

    for (const auto &argument : seed.arguments) {
        PARGINFO legacyArgument = record.procInfo->AddArg(argument.tag,
                                                          argument.ndx,
                                                          argument.size,
                                                          argument.name,
                                                          argument.typeDef);
        if (!legacyArgument) return false;
        legacyArgument->Register = argument.registerArgument;
    }

    for (const auto &local : seed.locals) {
        if (!record.procInfo->AddLocal(local.ofs, local.size, local.name, local.typeDef)) return false;
    }

    return true;
}

// Read the prototype/stack surface already known by one real legacy procedure
// record into the neutral metadata model. This is deliberately read-only and
// excludes legacy procSize: stored procedure extent has a separate lifecycle
// from prototype metadata and must not be conflated with observed CFG span.
inline bool CaptureLegacyProcedurePrototypeMetadata(const InfoRec &record,
                                                    ProcedurePrototypeMetadata &metadata) {
    metadata = {};
    if (record.kind < ikRefine || record.kind > ikFunc || !record.procInfo) return false;

    metadata.kind = record.kind;
    metadata.returnType = record.type;
    metadata.flags = record.procInfo->flags;
    metadata.bpBase = record.procInfo->bpBase;
    metadata.retBytes = record.procInfo->retBytes;
    metadata.stackSize = record.procInfo->stackSize;

    if (record.procInfo->args) {
        metadata.arguments.reserve(static_cast<std::size_t>(record.procInfo->args->Count));
        for (int i = 0; i < record.procInfo->args->Count; ++i) {
            PARGINFO argument = static_cast<PARGINFO>(record.procInfo->args->Items[static_cast<std::size_t>(i)]);
            if (!argument) {
                metadata = {};
                return false;
            }
            metadata.arguments.push_back({argument->Tag,
                                          argument->Register,
                                          argument->Ndx,
                                          argument->Size,
                                          argument->Name,
                                          argument->TypeDef});
        }
    }

    if (record.procInfo->locals) {
        metadata.locals.reserve(static_cast<std::size_t>(record.procInfo->locals->Count));
        for (int i = 0; i < record.procInfo->locals->Count; ++i) {
            PLOCALINFO local = static_cast<PLOCALINFO>(record.procInfo->locals->Items[static_cast<std::size_t>(i)]);
            if (!local) {
                metadata = {};
                return false;
            }
            metadata.locals.push_back({local->Ofs,
                                       local->Size,
                                       local->Name,
                                       local->TypeDef});
        }
    }

    return true;
}

// Install one fully prepared procedure record into the currently active loaded
// PE session. Build and populate the record detached first, then publish it to
// Infos[] only after success so a failed adaptation cannot leave a partial slot.
// This step deliberately does not set cfProcStart and still does not invent
// legacy procSize from the portable observed span.
inline bool ApplyLegacyProcedureMetadataSeedToActiveSession(DWord address,
                                                            const LegacyProcedureMetadataSeed &seed) {
    if (seed.kind < ikRefine || seed.kind > ikFunc) return false;

    const auto session = GetLegacyImageSessionView();
    if (!Infos || !session.infos ||
        session.infos != reinterpret_cast<void *const *>(Infos))
        return false;

    const int offset = AddressToOffset(address);
    if (offset < 0) return false;
    const auto pos = static_cast<std::size_t>(offset);
    if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;
    if (Infos[pos]) return false;

    InfoRec *record = new InfoRec(-1, seed.kind);
    if (!ApplyLegacyProcedureMetadataSeed(*record, seed)) {
        delete record;
        return false;
    }

    Infos[pos] = record;
    return true;
}

using LegacyProcedurePrototypeProvider =
    std::function<bool(const ProcedureSummary &, ProcedurePrototypeMetadata &)>;

// Reconcile every procedure already discovered by the portable CFG with the
// active legacy Infos[] session. Existing procedure records are captured and
// preserved untouched; only empty slots ask the caller for neutral prototype
// metadata and materialize a new record. All slots are preflighted before any
// new publication. A later write failure rolls back only records created by
// this batch. procSize is never inferred from observedSpan.
inline bool ApplyDiscoveredProceduresToActiveLegacySession(
    const ControlFlowResult &flow,
    Byte functionKind,
    const LegacyProcedurePrototypeProvider &provider,
    std::vector<DWord> *installedAddresses = nullptr,
    std::vector<DWord> *reusedAddresses = nullptr) {
    if (installedAddresses) installedAddresses->clear();
    if (reusedAddresses) reusedAddresses->clear();
    if (!provider || flow.procedures.empty()) return false;

    const auto session = GetLegacyImageSessionView();
    if (!Infos || !session.infos ||
        session.infos != reinterpret_cast<void *const *>(Infos))
        return false;

    struct PreparedProcedure {
        DWord address = 0;
        std::size_t pos = 0;
        LegacyProcedureMetadataSeed seed;
    };

    std::vector<PreparedProcedure> prepared;
    prepared.reserve(flow.procedures.size());
    std::vector<DWord> reused;
    reused.reserve(flow.procedures.size());
    std::vector<std::size_t> seenPositions;
    seenPositions.reserve(flow.procedures.size());

    for (const auto &summary : flow.procedures) {
        const int offset = AddressToOffset(summary.address);
        if (offset < 0) return false;
        const auto pos = static_cast<std::size_t>(offset);
        if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;
        for (const auto seen : seenPositions)
            if (seen == pos) return false;
        seenPositions.push_back(pos);

        if (Infos[pos]) {
            ProcedurePrototypeMetadata existing;
            if (!CaptureLegacyProcedurePrototypeMetadata(*Infos[pos], existing)) return false;
            reused.push_back(summary.address);
            continue;
        }

        ProcedurePrototypeMetadata metadata;
        if (!provider(summary, metadata)) return false;

        LegacyProcedureMetadataSeed seed;
        if (!BuildLegacyProcedureMetadataSeed(metadata, functionKind, seed)) return false;
        prepared.push_back({summary.address, pos, std::move(seed)});
    }

    std::vector<DWord> installed;
    installed.reserve(prepared.size());
    for (const auto &procedure : prepared) {
        if (!ApplyLegacyProcedureMetadataSeedToActiveSession(procedure.address, procedure.seed)) {
            for (const auto address : installed) {
                const int rollbackOffset = AddressToOffset(address);
                if (rollbackOffset < 0) continue;
                const auto rollbackPos = static_cast<std::size_t>(rollbackOffset);
                if (rollbackPos >= session.analysisSize) continue;
                delete Infos[rollbackPos];
                Infos[rollbackPos] = nullptr;
            }
            return false;
        }
        installed.push_back(procedure.address);
    }

    if (installedAddresses) *installedAddresses = std::move(installed);
    if (reusedAddresses) *reusedAddresses = std::move(reused);
    return true;
}

} // namespace idr::core
