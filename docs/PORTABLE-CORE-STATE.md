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

Run #40 was the first compile of the full engine slice. MSVC reached its 100-error cap, but the observed failures were overwhelmingly missing core declarations/constants normally made visible transitively through `Main.h` and `Misc.h`, not VCL calls or a compiler-language blocker.

Observed dependency groups in #40 included:

- address/flag helpers (`Adr2Pos`, `Pos2Adr`, `IsFlagSet`, `SetFlag`, `SetFlags`, `ClearFlag`)
- procedure/info helpers (`GetProcSize`, `GetInfoRec`, `GetNearestUpInstruction`)
- decompiler helpers (`GetDecompilerRegisterName`, `InitItem`, `GetDirectCondition`, `BranchGetPrevInstructionType`)
- analysis helpers (`IsIntOver`, `IsExit`, `IsValidCodeAdr`, Int64 comparison helpers)
- string/helper APIs (`Val2Str8`, `SameText`, `ExtractProcName`, inheritance-name check)
- globals `Disasm` and `Code`
- additional core flags/kinds from `Main.h`
- Borland `Exception::Message` semantics in catch/rethrow paths

The first batch fix exposes these exact dependencies without including the VCL-heavy headers. No original algorithm source is changed.

## Compatibility layer status

`tests/portable_core_compat.h` supplies temporary compiler-neutral aliases/containers plus selected core constants. After #40 it also includes the engine-required flags/kinds (`cfFrame`, `cfSwitch`, `cfDSkip`, `cfTry`, `cfLoop`, `cfFinallyExit`, `ikUnknown`, `ikConstructor`, `ikDestructor`) and an `Exception::Message` member compatible with the observed Borland exception usage.

Compiler-verified String differences currently mapped in generated smoke code:

- numeric `String(int)` -> `std::to_string(...)`
- `.Length()` -> `.size()`
- `AnsiReplaceText()` exposed by a narrow String-only declaration for compile smoke

Still to map as encountered: 1-based indexing, `Pos()`, `SubString()`, case-insensitive behavior, Unicode/ANSI semantics and other RTL helpers.

## Architectural boundaries found

### `Main.h`

Mixes core records/constants with VCL GUI state. Runs through #40 increasingly support extracting reusable core definitions into a neutral header (`CoreTypes.h` / `IdrTypes.h`).

### `Misc.h`

Mixes pure analysis helpers with `TForm`, `TCanvas`, clipboard and dialog helpers. The engine dependency wall reinforces that a neutral core-analysis API header would remove many transitive dependencies without pulling in VCL.

### GUI boundary after `TDecompiler::Init()`

`OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly couple to form/presentation behavior and are intentionally skipped by the headless core mapping.

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
