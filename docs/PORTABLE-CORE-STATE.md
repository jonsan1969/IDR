# Portable Core Project State

Last updated: 2026-08-17

## Goal

Build a useful headless IDR core/CLI on free GitHub-hosted Windows Actions runners with MSVC, without requiring Embarcadero C++Builder, while leaving the original VCL GUI path intact.

Starting point: `sarog/IDR` modern fork. Current experimental fork: `jonsan1969/IDR`.

## Working branch

`agent/portable-core-smoke`

`main` remains untouched. Do not merge or open an upstream PR until the experiment has produced a coherent portable target.

## CI baseline

Workflow: `.github/workflows/portable-core-smoke.yml`

- `windows-latest` (currently Windows Server 2025 / VS2026 image)
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`
- `actions/checkout@v6` (Node 24 generation)
- no Node 20 helper actions
- `concurrency.cancel-in-progress: true`
- `docs/**` ignored for push-triggered CI

x86 is intentional because legacy `Disasm.cpp` contains inline x86 assembly that MSVC cannot compile in x64 mode.

## Proven portable so far

Successfully compiled on GitHub-hosted MSVC x86:

- `Disasm.h`
- `KnowledgeBase.h`
- `Infos.h`
- generated portable `Decompiler.h`
- real `Disasm.cpp` after a small generated syntax/compatibility transform
- primary real `Decompiler.cpp` slice from `GetString()` through complete `TDecompiler::Init()`
- complete independent BJL/branch-analysis slice from `GetBJLRange()` through `PrintBJL()`

### Primary decompiler milestone

Run #27: complete `TDecompiler::Init()` compiles with MSVC x86 without importing VCL.

### Branch-analysis milestones

- #30 red: missing declaration of `BranchGetPrevInstructionType()` from mixed VCL/core `Misc.h`.
- #31 green: complete `GetBJLRange()`.
- #33 red: first Embarcadero `String` mismatch at numeric `String(int)` construction.
- #34 green: complete `CreateBJLSequence()` after narrow `std::to_string(...)` transformation in generated smoke copy.
- #35 green: complete `UpdateBJLList()` and `BJLAnalyze()` with no new shim.
- #36 red: second verified String mismatch at `.Length()`.
- #37 green: all non-printing BJL helpers through `ExprMerge()` after `.Length()` -> `.size()` transformation.
- #38 green: complete `PrintBJL()` compiles after mapping `String(k)` to `std::to_string(k)` and exposing only the compile-time String signature of `AnsiReplaceText()`.

The complete BJL slice is now a stable green block. Original source remains unchanged; all portability transforms are generated under `tests/generated`.

## Current active test: Decompiler engine slice

A third independent implementation slice covers complete `TDecompiler::Decompile()` and stops immediately before `TDecompiler::DecompileCaseEnum()`.

Run #40 was the first compile of the full engine slice. MSVC reached its 100-error cap, but the observed failures were overwhelmingly missing core declarations/constants normally made visible transitively through `Main.h` and `Misc.h`. The first dependency batch exposed those APIs explicitly without changing the original algorithm.

Run #42 progressed substantially beyond #40 and exposed three new categories:

1. A second large set of pure core-analysis helpers and type-kind constants from `Misc.h` / `Main.h`.
2. Additional Embarcadero RTL/String semantics: `IntToStr`, `IntToHex`, `AnsiString`, numeric `String(...)` construction and 1-based `String::Pos()`.
3. The first direct GUI/interactivity coupling found inside `TDecompiler::Decompile()` itself.

### Engine GUI/interactivity boundary found in #42

Embedded-procedure handling directly reads/writes `FMain_11011981->lbCode->ItemIndex` and calls `Application->MessageBox()` to ask whether an embedded procedure should be decompiled. Other uncertain-call paths use `ManualInput(...)`.

This is not being ported by inventing fake VCL form/application classes. The generated smoke copy replaces only the embedded-procedure GUI plumbing with a policy-shaped `PortableConfirmEmbeddedProcedure(...)` dependency while preserving the analysis path. `ManualInput(...)` remains an explicit dependency and is a candidate for a future headless resolver callback.

A real portable core should eventually expose policies/interfaces such as:

- embedded procedures: always / never / callback
- unknown return bytes or types: resolver callback or deterministic CLI failure

The original `Decompiler.cpp` remains unchanged.

## Compatibility layer status

`tests/portable_core_compat.h` supplies temporary compiler-neutral aliases/containers plus only constants reached by tested code. After #42 this includes additional observed kinds such as integer, char, enumeration, class, wide/Unicode/string-pointer kinds, arrays, Int64, VMT and procedure. `AnsiString` is temporarily mapped to `std::string` for compile smoke.

Compiler-verified String/RTL differences currently mapped or exposed in generated smoke code:

- numeric `String(int)` -> `std::to_string(...)`
- `.Length()` -> `.size()`
- `String::Pos()` -> helper preserving 1-based/zero-not-found semantics
- `AnsiReplaceText()` exposed by a narrow String-only declaration
- `IntToStr` / `IntToHex` exposed as RTL dependencies
- `AnsiString` temporarily mapped to `std::string`

Still to map as encountered: SubString, broader 1-based indexing, case-insensitive behavior and Unicode/ANSI semantics.

## Architectural boundaries found

### `Main.h`

Mixes core records/constants with VCL GUI state. Runs through #42 strongly support extracting reusable core definitions into a neutral header (`CoreTypes.h` / `IdrTypes.h`).

### `Misc.h`

Mixes pure analysis helpers with `TForm`, `TCanvas`, clipboard and dialog helpers. The main engine uses many of the pure helpers without needing the UI half, strengthening the case for a separate core-analysis API header.

### GUI boundary after `TDecompiler::Init()`

`OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly couple to form/presentation behavior and remain intentionally skipped by the headless core mapping.

### GUI/interactivity inside `TDecompiler::Decompile()`

Run #42 proved that the engine itself also contains limited interactive policy/UI code around embedded procedures and unresolved calls. These need callbacks/policies in a real headless core rather than VCL compatibility shims.

## Working rules

- Preserve original source; generated transformed copies/harnesses only during mapping.
- Add compatibility semantics only when actual code requires them.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- Do not pull GUI code into core just to keep source slices contiguous.
- Fetch a failed Actions log once per run; successful runs need status/job verification only.
- Keep these docs updated at meaningful milestones.

## Success criterion

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
