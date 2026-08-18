#include "IdrProcedureAnalysis.h"
#include "IdrHeadlessPrototypePolicy.h"

#include <iostream>

int main() {
    using namespace idr::core;

    constexpr Byte kProcedureKind = 1;
    constexpr Byte kFunctionKind = 2;

    ProcedurePrototypeMetadata procedure;
    procedure.kind = kProcedureKind;
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) return 1;

    ProcedurePrototypeMetadata function;
    function.kind = kFunctionKind;
    if (IsProcedurePrototypeComplete(function, kFunctionKind)) return 2;
    function.returnType = "Integer";
    if (!IsProcedurePrototypeComplete(function, kFunctionKind)) return 3;

    procedure.flags = 0x12345678u;
    procedure.bpBase = 12;
    procedure.retBytes = 8;
    procedure.stackSize = 0x200;

    ProcedureArgumentMetadata regArg;
    regArg.tag = 0x21;
    regArg.inRegister = true;
    regArg.index = 1;
    regArg.size = 4;
    regArg.name = "Value";
    regArg.type = "Integer";
    procedure.arguments.push_back(regArg);

    ProcedureArgumentMetadata stackArg;
    stackArg.tag = 0x22;
    stackArg.inRegister = false;
    stackArg.index = 12;
    stackArg.size = 4;
    stackArg.name = "Other";
    stackArg.type = "Pointer";
    procedure.arguments.push_back(stackArg);

    ProcedureLocalMetadata local;
    local.offset = -8;
    local.size = 4;
    local.name = "Temp";
    local.type = "Integer";
    procedure.locals.push_back(local);

    LegacyProcedureMetadataSeed seed;
    if (!BuildLegacyProcedureMetadataSeed(procedure, kFunctionKind, seed)) return 4;
    if (seed.kind != kProcedureKind || !seed.returnType.empty() ||
        seed.flags != procedure.flags || seed.bpBase != 12 || seed.retBytes != 8 ||
        seed.stackSize != 0x200 || seed.arguments.size() != 2 || seed.locals.size() != 1)
        return 5;

    if (seed.arguments[0].tag != 0x21 || !seed.arguments[0].registerArgument ||
        seed.arguments[0].ndx != 1 || seed.arguments[0].size != 4 ||
        seed.arguments[0].name != "Value" || seed.arguments[0].typeDef != "Integer")
        return 6;

    if (seed.arguments[1].tag != 0x22 || seed.arguments[1].registerArgument ||
        seed.arguments[1].ndx != 12 || seed.arguments[1].size != 4 ||
        seed.arguments[1].name != "Other" || seed.arguments[1].typeDef != "Pointer")
        return 7;

    if (seed.locals[0].ofs != -8 || seed.locals[0].size != 4 ||
        seed.locals[0].name != "Temp" || seed.locals[0].typeDef != "Integer")
        return 8;

    ProcedurePrototypeMetadata incompleteFunction;
    incompleteFunction.kind = kFunctionKind;
    LegacyProcedureMetadataSeed rejected;
    if (BuildLegacyProcedureMetadataSeed(incompleteFunction, kFunctionKind, rejected)) return 9;

    PrototypeResolutionRequest request;
    request.procedureAddress = 0x00402000u;
    request.callSite = 0x00401005u;
    request.current = incompleteFunction;

    ProcedurePrototypeMetadata resolved;
    if (ResolveProcedurePrototype(request, kFunctionKind, {}, resolved)) return 10;

    HeadlessPrototypeResolver unavailable = [](const PrototypeResolutionRequest &) {
        return PrototypeResolutionResult{PrototypeResolutionStatus::Unavailable, {}};
    };
    if (ResolveProcedurePrototype(request, kFunctionKind, unavailable, resolved)) return 11;

    HeadlessPrototypeResolver incomplete = [=](const PrototypeResolutionRequest &) {
        PrototypeResolutionResult result;
        result.status = PrototypeResolutionStatus::Resolved;
        result.prototype.kind = kFunctionKind;
        return result;
    };
    if (ResolveProcedurePrototype(request, kFunctionKind, incomplete, resolved)) return 12;

    HeadlessPrototypeResolver resolver = [=](const PrototypeResolutionRequest &seen) {
        PrototypeResolutionResult result;
        if (seen.procedureAddress != 0x00402000u || seen.callSite != 0x00401005u)
            return result;
        result.status = PrototypeResolutionStatus::Resolved;
        result.prototype.kind = kFunctionKind;
        result.prototype.returnType = "Integer";
        return result;
    };
    if (!ResolveProcedurePrototype(request, kFunctionKind, resolver, resolved)) return 13;
    if (resolved.kind != kFunctionKind || resolved.returnType != "Integer") return 14;

    bool resolverCalled = false;
    PrototypeResolutionRequest completeRequest;
    completeRequest.current = procedure;
    HeadlessPrototypeResolver mustNotRun = [&](const PrototypeResolutionRequest &) {
        resolverCalled = true;
        return PrototypeResolutionResult{};
    };
    if (!ResolveProcedurePrototype(completeRequest, kFunctionKind, mustNotRun, resolved)) return 15;
    if (resolverCalled || resolved.kind != kProcedureKind) return 16;

    std::cout << "procedure-prototype-metadata=ok\n";
    std::cout << "legacy-procedure-metadata-seed=ok\n";
    std::cout << "headless-prototype-policy=ok\n";
    std::cout << "seed-argument-count=" << seed.arguments.size() << '\n';
    std::cout << "seed-local-count=" << seed.locals.size() << '\n';
    return 0;
}
