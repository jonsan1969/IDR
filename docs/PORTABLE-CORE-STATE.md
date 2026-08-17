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
- complete independent main-engine slice containing all of `TDecompiler::Decompile()`

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

## Main Decompiler engine milestone

The third independent implementation slice covers complete `TDecompiler::Decompile()` and stops immediately before `TDecompiler::DecompileCaseEnum()`.

- #40: first full engine compile hit MSVC's 100-error cap, overwhelmingly on hidden core declarations/constants from `Main.h`/`Misc.h`.
- #42: progressed much deeper and exposed additional RTL/String semantics plus the first direct GUI/policy coupling inside `Decompile()`.
- #44: progressed beyond that UI boundary. It exposed another form-owned analysis call (`FMain_11011981->GetMethodInfo`) and also caught an over-broad smoke transformation that accidentally produced `GetImmstd::to_string`; the generator was corrected instead of changing original source.
- #46: major convergence milestone. The engine no longer hit the 100-error cap. The remaining errors were a bounded set dominated by numeric `String(...)` conversions, `TStringList::Objects` / `IndexOfName`, `Variant`, record/name/try helpers, and a handful of missing flags/kinds.
- #48: dependency mapping had converged completely. Only nine concrete String/numeric compatibility errors remained: four numeric-zero assignments to `String`, one direct integer concatenation, and four `String(_imm)` constructions in IMUL simulation. No new core or GUI dependencies appeared.
- #49: **complete `TDecompiler::Decompile()` compiles successfully with MSVC x86**. All workflow steps were green, including the already-stable header, primary, BJL and `Disasm.cpp` smoke tests.

This is the strongest portability result so far: the main decompiler engine is compiler-portable at object-compilation level once its hidden core dependencies, limited UI-policy seams and observed Embarcadero RTL semantics are made explicit. This is not yet runtime semantic equivalence and not yet a linked CLI.

## Current active test: case-enum slice

A fourth independent implementation slice now isolates complete `TDecompiler::DecompileCaseEnum()` between `Decompile()` and `GetSysCallAlias()`.

Known numeric case-label construction (`String(n + N)` / `String(m + N)`) is translated narrowly to `std::to_string(...)` in the generated smoke copy. The original source remains unchanged.

Keeping this slice independent preserves #49 as a stable main-engine milestone while mapping the next algorithmic block.

### Engine GUI/interactivity boundary

Embedded-procedure handling directly reads/writes `FMain_11011981->lbCode->ItemIndex` and calls `Application->MessageBox()`. The generated smoke copy replaces that plumbing with `PortableConfirmEmbeddedProcedure(...)` rather than fake VCL classes.

Virtual-method lookup is analysis logic but is currently owned by the form through `FMain_11011981->GetMethodInfo`; the smoke copy exposes it as `PortableGetMethodInfo(...)`.

`ManualInput(...)` remains an explicit dependency for unresolved return-byte/type cases and should eventually become a resolver callback or deterministic CLI policy.

## Compatibility layer status

`tests/portable_core_compat.h` supplies temporary compiler-neutral aliases/containers plus only constants reached by tested code.

Compiler-verified String/RTL differences currently mapped or exposed in generated smoke code:

- numeric `String(int)` -> controlled `std::to_string(...)` transforms
- numeric values assigned/concatenated directly to Embarcadero `String` -> explicit decimal conversion in generated smoke code where observed
- `.Length()` -> `.size()`
- `String::Pos()` -> helper preserving 1-based/zero-not-found semantics
- `SubString()` -> helper preserving 1-based start semantics
- `SetLength()` -> `resize()` for observed smoke use
- `AnsiReplaceText()`, `IntToStr`, `IntToHex`, `QuotedStr` exposed as dependencies
- `AnsiString` temporarily mapped to `std::string`
- `WideString` has a minimal smoke wrapper
- `Variant` has a temporary integer-compatible smoke alias

`String = std::string` is increasingly strained. Do not treat compile success as runtime semantic equivalence, especially for direct 1-based indexing. A thin compatibility type may become justified after the remaining indexing behavior is mapped.

`TStringList` now includes `Strings`, aligned `Objects`, sorted insertion, `IndexOf`, and `IndexOfName`. This is still a targeted compatibility shim, not a full VCL implementation.

## Architectural boundaries found

### `Main.h`

Mixes core records/constants with VCL GUI state. Runs through #49 strongly support extracting reusable core definitions into a neutral header (`CoreTypes.h` / `IdrTypes.h`).

### `Misc.h`

Mixes pure analysis helpers with forms/canvas/dialog helpers. The main engine uses many pure helpers without needing the UI half, strengthening the case for a separate core-analysis API header.

### GUI boundary after `TDecompiler::Init()`

`OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly couple to form/presentation behavior and remain intentionally skipped by the headless core mapping.

### GUI/interactivity inside `TDecompiler::Decompile()`

The engine is overwhelmingly analysis code but contains small policy/UI decisions around embedded procedures, virtual-method lookup ownership, and unresolved-call input. These need injected interfaces/policies in a real headless core.

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
