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
- `docs/**` is ignored for push-triggered CI so documentation-only commits do not start compiler runs

## What is proven to compile with MSVC on GitHub-hosted Actions

The following have been successfully compiled on the hosted runner:

- `Disasm.h`
- `KnowledgeBase.h`
- `Infos.h`
- a generated portable copy of `Decompiler.h`
- real `Disasm.cpp` implementation, compiled as x86 after a small generated portability transformation
- real slices of `Decompiler.cpp`, progressively expanded through decompiler state, register handling, stack/FPU handling, prototype checking and full `TDecompiler::Init()`

### Successful decompiler implementation coverage so far

The tested primary slice has reached through code covering:

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
- decompiler flag handling, including `InitFlags`
- register item handling (`SetRegItem`, `GetRegItem`, `GetRegType`)
- stack handling (`Push`, `Pop`)
- FPU stack handling (`FGet`, `FSet`, `FXch`, `FPush`, `FPop`)
- `CheckPrototype`
- full `TDecompiler::Init()` including calling-convention argument setup and return-type handling

Run #17 was fully green after adding STL-backed replacements for the minimal `TList`/`TStringList` behavior needed by the slice.

Run #19 failed only because the isolated slice did not yet define the core flags `cfPass` and `cfLoc`. The failure was not a compiler, STL, stack, or VCL architectural blocker.

Run #23 was fully green after adding only the missing core flag definitions. This confirmed the expanded slice through `InitFlags`, register handling, and `Push`/`Pop`.

Run #24 failed only on missing `ikFunc`; the FPU code itself had compiled. Run #25 was fully green after adding that core kind constant.

Run #26 expanded through all of `TDecompiler::Init()` and failed only on four more core constants trapped in `Main.h`: `cfImport`, `ikFloat`, `ikLString`, and `ikRecord`.

Run #27 was fully green after adding those four constants. This is an important milestone: the complete `TDecompiler::Init()` implementation now compiles with MSVC x86 on a stock GitHub-hosted Windows runner without pulling in VCL.

## Current phase: branch/loop analysis slice

The primary contiguous slice now stops immediately before `OutputSourceCodeLine()`, where direct GUI coupling begins through `FMain_11011981`.

Rather than force GUI code into the portable core, a second independent implementation slice is being introduced beginning at:

`TDecompileEnv::GetBJLRange()`

This targets branch/jump/loop analysis while deliberately skipping `OutputSourceCode*()` and `DecompileProc()` presentation/orchestration code.

New harness file:

- `tests/prepare_portable_decompiler_branch_slice.ps1`

The first branch-analysis smoke test contains only `GetBJLRange()` so that dependency growth remains measurable. `CreateBJLSequence()` will be added after that boundary is proven.

## Current portability strategy

Do not rewrite the original source tree wholesale yet.

The current method is deliberately conservative:

1. Leave original IDR source files unchanged.
2. Generate portable test copies/slices under `tests/generated` during CI.
3. Replace only compiler/runtime-specific constructs required for the tested slice.
4. Compile the resulting code using MSVC x86.
5. Split around known UI blocks rather than dragging VCL into headless code.
6. Expand each core slice until a genuine architectural blocker is found.

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
- selected core flags/kinds normally trapped in `Main.h`, including `cfImport`, `cfPass`, `cfLoc`, `cfSkip`, `ikFloat`, `ikLString`, `ikRecord`, and `ikFunc`

These are intentionally incomplete. Add only semantics actually required by IDR code as the smoke-test boundary expands.

## Important architectural findings

### `Main.h`

`Main.h` mixes core data structures, flag constants, and VCL GUI definitions. A future clean port will probably split reusable analysis structures/constants into a compiler-neutral header such as `IdrTypes.h` or `CoreTypes.h`.

Runs #19 through #27 provide concrete evidence: several harmless decompiler flags/type-kind constants had to be duplicated in the isolated portability layer solely because their canonical definitions currently live inside VCL-heavy `Main.h`.

### GUI boundary after `TDecompiler::Init()`

Immediately after the now-portable `Init()` function, `OutputSourceCodeLine()` writes directly through `FMain_11011981->lbSourceCode`. `OutputSourceCode()` also relies on VCL/Embarcadero string behavior and the form.

This is now treated as a presentation boundary, not as evidence that the underlying decompiler/branch-analysis algorithms require VCL.

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
- signed/unsigned comparison warnings
- x86 inline assembly touching `ebp` in `Disasm.cpp`

Do not modernize these while proving portability unless necessary. Functional porting and cleanup/security modernization should remain separate changes to reduce regression risk.

## Why x86 first

IDR is historically a Win32 Delphi reverse-engineering application, and `Disasm.cpp` contains Borland-style inline x86 assembly. MSVC supports inline assembly for x86 but not x64, so the first portable build target is intentionally Win32/x86.

A future x64 port would require rewriting/replacing those inline-assembly sections.

## Immediate next steps

1. Prove `TDecompileEnv::GetBJLRange()` in the new branch-analysis slice.
2. Expand that slice into `CreateBJLSequence()`; this will likely require adding `Clear()` and `Delete()` semantics to the minimal `TList` shim.
3. Continue through the BJL expression/loop-analysis helpers while avoiding the known GUI output block.
4. Map how much of `Infos.cpp`, `KnowledgeBase.cpp`, `Misc.cpp`, `Analyze1.cpp`, `Analyze2.cpp`, and `AnalyzeArguments.cpp` can compile with the same compatibility layer.
5. Once enough implementation is proven portable, introduce a real `idr-core` target rather than generated smoke slices.
6. Add a minimal `idr-cli` executable only after core linkage is practical.
7. Leave the original VCL GUI build path intact during this work.

## Success criterion

The useful milestone is not merely compiling individual headers. The target is:

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid tooling, or a self-hosted runner.
