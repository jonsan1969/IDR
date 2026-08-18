#pragma once

#include "IdrProcedureAnalysis.h"
#include "IdrLegacyCompat.h"

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

} // namespace idr::core
