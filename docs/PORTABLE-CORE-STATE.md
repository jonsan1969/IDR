# Portable Core Project State

Last updated: 2026-08-17

## Goal

Make it possible to build a useful headless IDR core/CLI on free GitHub-hosted Windows Actions runners, without requiring Embarcadero C++Builder, while keeping the original VCL GUI path intact.

This work started from the `sarog/IDR` fork, which modernized IDR for current Embarcadero C++Builder versions and added CMake/Ninja support. The current experimental work lives in `jonsan1969/IDR`.

## Working branch

`agent/portable-core-smoke`

Do not treat `main` as containing the portability work yet. `main` is intentionally left untouched while the approach is validated.

## CI workflow

`.github/workflows/portable-core-smoke.yml`

Runner:

- `windows-latest`
- Currently resolves to Windows Server 2025 / VS2026 hosted runner
- MSVC x86 is initialized with `vswhere` + `vcvars32.bat`

Actions policy:

- Uses `actions/checkout@v6`
- Avoid Node.js 20 based Actions
- No `ilammy/msvc-dev-cmd@v1`; MSVC setup is done directly to avoid its Node 20 runtime
- `concurrency` with `cancel-in-progress: true` is enabled to avoid queues of stale smoke runs

## What is proven to compile with MSVC on GitHub-hosted Actions

The following have been successfully compiled on the hosted runner:

- `Disasm.h`
- `KnowledgeBase.h`
- `Infos.h`
- a generated portable copy of `Decompiler.h`
- real `Disasm.cpp` implementation, compiled as x86 after a small generated portability transformation
- real slices of `Decompiler.cpp`, progressively expanded from helper routines through significant decompiler state and environment code

### Successful decompiler implementation coverage so far

The tested slice has reached through code covering:

- `GetString`
- `GetFieldName`
- `GetArgName`
- `GetGvarName`
- direct/inverted condition helpers
- `InitItem`
- `AssignItem`
- `TNamer`
- `TForInfo`
- `TWhileInfo`
- `TLoopInfo`
- `TDecompileEnv` construction/destruction
- local variable naming
- context save/restore
- `TDecompiler` construction/destruction
- decompiler flag handling
- register item handling
- stack handling is the next active boundary

Run #17 was fully green after adding STL-backed replacements for the minimal `TList`/`TStringList` behavior needed by this slice.

Run #19 was triggered to extend the slice through register and stack handling and introduce a minimal `Exception` shim.

## Current portability strategy

Do not rewrite the original source tree wholesale yet.

The current method is deliberately conservative:

1. Leave original IDR source files unchanged.
2. Generate portable test copies/slices under `tests/generated` during CI.
3. Replace only compiler/runtime-specific constructs required for the tested slice.
4. Compile the resulting code using MSVC x86.
5. Expand the slice until a genuine architectural blocker is found.

This separates proof of portability from invasive source refactoring.

## Compatibility layer

`tests/portable_core_compat.h`

Current concepts supplied by the test compatibility layer include:

- `Byte`, `Word`, `DWord` using `<cstdint>`
- `String` currently mapped to `std::string`
- `__fastcall` compatibility macro
- minimal STL-backed `TList`
- minimal STL-backed `TStringList`
- minimal `Exception` replacement using the standard C++ exception model

These are intentionally incomplete. Add only semantics actually required by IDR code as the smoke-test boundary expands.

## Important architectural findings

### `Main.h`

`Main.h` mixes core data structures with VCL GUI definitions. A future clean port will probably split reusable analysis structures into a compiler-neutral header such as `IdrTypes.h` or `CoreTypes.h`.

### `TypeInfo2`

`TypeInfo2.h/.cpp` mixes RTTI logic with a VCL `TForm`. For a portable core, RTTI functionality such as `GetRTTI`, `GetCppTypeInfo` and `Guid2String` should eventually be separated from the form class.

### `InputDlg`

`InputDlg` is pure UI and should not be part of the headless core. Any decompiler path that requests interactive user input should eventually use a callback/interface or be disabled in CLI mode.

### `Resources`

`Resources.*` is strongly tied to VCL/forms/DFM handling and is not part of the initial headless portability target.

## Warnings seen under MSVC

These do not currently stop compilation:

- old CRT calls such as `sprintf`, `strcat`, `strcpy`
- unused variables in legacy code
- x86 inline assembly touching `ebp` in `Disasm.cpp`

Do not modernize these while proving portability unless necessary. Functional porting and cleanup/security modernization should remain separate changes to reduce regression risk.

## Why x86 first

IDR is historically a Win32 Delphi reverse-engineering application, and `Disasm.cpp` contains Borland-style inline x86 assembly. MSVC supports inline assembly for x86 but not x64, so the first portable build target is intentionally Win32/x86.

A future x64 port would require rewriting/replacing those inline-assembly sections.

## Immediate next steps

1. Observe run #19 and fix only the first genuine portability failure.
2. Continue expanding the real `Decompiler.cpp` implementation slice.
3. Map how much of `Infos.cpp`, `KnowledgeBase.cpp`, `Misc.cpp`, `Analyze1.cpp`, `Analyze2.cpp`, and `AnalyzeArguments.cpp` can compile with the same compatibility layer.
4. Once enough implementation is proven portable, introduce a real `idr-core` target rather than generated smoke slices.
5. Add a minimal `idr-cli` executable only after core linkage is practical.
6. Leave the original VCL GUI build path intact during this work.

## Success criterion

The useful milestone is not merely compiling individual headers. The target is:

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid tooling, or a self-hosted runner.
