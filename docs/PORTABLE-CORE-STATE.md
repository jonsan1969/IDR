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
- primary real `Decompiler.cpp` slice from `GetString()` through the complete `TDecompiler::Init()`
- independent branch-analysis slice through all non-printing BJL helpers up to `ExprMerge()`

### Primary decompiler milestone

Run #27: complete `TDecompiler::Init()` compiles with MSVC x86 without importing VCL.

Coverage includes naming helpers, ITEM manipulation, loop/environment objects, saved context, register state, normal/FPU stacks, prototype checking, decompiler flags and calling-convention/return-value setup.

### Branch-analysis milestones

- #30 red: missing declaration of core helper `BranchGetPrevInstructionType()` from mixed VCL/core `Misc.h`.
- #31 green: complete `GetBJLRange()` compiles after exposing only that helper declaration.
- #33 red: first real Embarcadero `String` semantic mismatch at `String(m)` / `String(m + 1)`.
- #34 green: complete `CreateBJLSequence()` compiles after the generated smoke copy maps those numeric conversions to `std::to_string(...)`.
- #35 green: complete `UpdateBJLList()` and complete `BJLAnalyze()` compile with no additional portability shim.
- #36 red: expanded BJL helpers reached `String::Length()`, the second compiler-verified String semantic mismatch.
- #37 green: the complete non-printing BJL helper span through `ExprMerge()` compiles after the generated smoke copy maps `.Length()` to `.size()`.

Run #34 onward also confirms the current STL-backed `TList` operations used by BJL code compile cleanly: `Count`, `Items[]`, `Add()`, `Clear()`, and `Delete(index)`.

## Current active test

The branch-analysis slice is now extended through complete `TDecompileEnv::PrintBJL()` and stops immediately before `TDecompiler::Decompile()`.

`PrintBJL()` adds two observed Embarcadero dependencies:

- numeric `String(k)`, mapped to `std::to_string(k)` in the generated smoke copy
- `AnsiReplaceText(...)`; compile-only smoke exposes its pure String signature without importing `System.StrUtils.hpp` or implementing an RTL clone yet

Triggering commit: `5170af65ceb3fdf2edf3e728b85ec007d903d1be`.

Original IDR source remains unchanged; transformations exist only in the generated portability smoke copy.

If the active run fails, fetch that new failing run log once only, fix the complete batch of errors from that result, and never refetch the same failed log.

## Compatibility layer status

`tests/portable_core_compat.h` currently supplies:

- `Byte`, `Word`, `DWord` via `<cstdint>`
- temporary `String = std::string`
- neutralized `__fastcall`
- minimal STL-backed `TList`
- minimal STL-backed `TStringList`
- standard-C++ `Exception` shim
- selected core flags/kinds trapped in `Main.h`: `cfImport`, `cfPass`, `cfLoc`, `cfSkip`, `ikFloat`, `ikLString`, `ikRecord`, `ikFunc`

Important: `String = std::string` is only a compile shim, not a semantic replacement. Compiler-verified differences now include numeric `String(int)` construction (#33) and `.Length()` (#36). Other Embarcadero semantics still to map include 1-based indexing, `Pos()`, `SubString()`, case-insensitive replacement/search and Unicode behavior.

## Architectural boundaries found

### `Main.h`

Mixes core records/constants with VCL GUI state. Future clean port should move reusable core definitions into a neutral header (`CoreTypes.h` / `IdrTypes.h`) consumed by both core and VCL code.

### `Misc.h`

Also mixes responsibilities. Pure analysis APIs such as `BranchGetPrevInstructionType()` sit beside `TForm`, `TCanvas`, clipboard and dialog helpers. #30 -> #31 proved the analysis API can be separated from the UI half.

### GUI boundary after `TDecompiler::Init()`

Immediately afterward, `OutputSourceCodeLine()` writes directly to `FMain_11011981->lbSourceCode`; `OutputSourceCode()` also depends on Embarcadero string behavior. This block is intentionally skipped in the headless core experiment.

### Other UI-heavy areas

- `InputDlg.*`: pure UI
- `Resources.*`: VCL/DFM-heavy
- `TypeInfo2.*`: useful RTTI logic mixed with a VCL form; later extract RTTI core helpers

## Working rules

- Preserve original source; generated transformed copies/harnesses only during mapping.
- Add compatibility semantics only when actual code requires them.
- Do not mix CRT/security modernization with functional portability work.
- Do not attempt x64 yet.
- Do not pull GUI code into core just to keep source slices contiguous.
- Fetch a failed Actions log once per run; successful runs need status/job verification only, not logs.
- Keep these docs updated at meaningful milestones.

## Success criterion

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
