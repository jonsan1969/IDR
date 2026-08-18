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

// Materialize every procedure already discovered by the portable CFG into the
// active legacy Infos[] session. Procedure addresses come only from the CFG;
// prototype/kind data is supplied explicitly by the caller. All procedures are
// preflighted before publication. If a later write fails, records installed by
// this batch are removed again. procSize is still never inferred from observedSpan.
inline bool ApplyDiscoveredProceduresToActiveLegacySession(
    const ControlFlowResult &flow,
    Byte functionKind,
    const LegacyProcedurePrototypeProvider &provider,
    std::vector<DWord> *installedAddresses = nullptr) {
    if (installedAddresses) installedAddresses->clear();
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
    std::vector<std::size_t> seenPositions;
    seenPositions.reserve(flow.procedures.size());

    for (const auto &summary : flow.procedures) {
        ProcedurePrototypeMetadata metadata;
        if (!provider(summary, metadata)) return false;

        LegacyProcedureMetadataSeed seed;
        if (!BuildLegacyProcedureMetadataSeed(metadata, functionKind, seed)) return false;

        const int offset = AddressToOffset(summary.address);
        if (offset < 0) return false;
        const auto pos = static_cast<std::size_t>(offset);
        if (pos >= session.analysisSize || pos >= static_cast<std::size_t>(session.totalSize)) return false;
        if (Infos[pos]) return false;
        for (const auto seen : seenPositions)
            if (seen == pos) return false;

        seenPositions.push_back(pos);
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
    return true;
}

} // namespace idr::core
