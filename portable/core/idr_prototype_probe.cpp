#include "IdrProcedureAnalysis.h"

#include <iostream>

int main() {
    using namespace idr::core;

    constexpr Byte kProcedureKind = 1;
    constexpr Byte kFunctionKind = 2;

    ProcedurePrototypeMetadata procedure;
    procedure.kind = kProcedureKind;
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) {
        std::cerr << "procedure without return type should be complete\n";
        return 1;
    }

    ProcedurePrototypeMetadata function;
    function.kind = kFunctionKind;
    if (IsProcedurePrototypeComplete(function, kFunctionKind)) {
        std::cerr << "function without return type should be incomplete\n";
        return 2;
    }
    function.returnType = "Integer";
    if (!IsProcedurePrototypeComplete(function, kFunctionKind)) {
        std::cerr << "function with return type should be complete\n";
        return 3;
    }

    ProcedureArgumentMetadata argument;
    argument.tag = 0x21;
    argument.inRegister = true;
    argument.index = 0;
    argument.size = 4;
    argument.name = "Value";
    procedure.arguments.push_back(argument);
    if (IsProcedurePrototypeComplete(procedure, kFunctionKind)) {
        std::cerr << "argument without type should make prototype incomplete\n";
        return 4;
    }
    procedure.arguments[0].type = "Integer";
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) {
        std::cerr << "typed argument should make prototype complete\n";
        return 5;
    }

    ProcedureArgumentMetadata secondArgument;
    secondArgument.tag = 0x22;
    secondArgument.inRegister = false;
    secondArgument.index = 8;
    secondArgument.size = 4;
    secondArgument.name = "Other";
    secondArgument.type = "Pointer";
    procedure.arguments.push_back(secondArgument);
    if (!IsProcedurePrototypeComplete(procedure, kFunctionKind)) {
        std::cerr << "all typed arguments should keep prototype complete\n";
        return 6;
    }

    procedure.arguments[1].type.clear();
    if (IsProcedurePrototypeComplete(procedure, kFunctionKind)) {
        std::cerr << "any untyped argument should make prototype incomplete\n";
        return 7;
    }

    std::cout << "procedure-prototype-metadata=ok\n";
    std::cout << "procedure-argument-count=" << procedure.arguments.size() << '\n';
    return 0;
}
