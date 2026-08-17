# Portable Core Project State

Last updated: 2026-08-17

## Goal

Build a useful headless IDR core/CLI on free GitHub-hosted Windows Actions runners with MSVC, without requiring Embarcadero C++Builder, while leaving the original VCL GUI path intact.

Starting point: `sarog/IDR`. Experimental fork: `jonsan1969/IDR`.

## Working branch

`agent/portable-core-smoke`

`main` remains untouched. Do not merge or open an upstream PR until the experiment has produced a coherent portable target.

## CI baseline

Workflow: `.github/workflows/portable-core-smoke.yml`

- GitHub-hosted Windows Server 2025 / VS2026
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`
- `actions/checkout@v6`
- stale runs cancelled by concurrency
- `docs/**` ignored for push-triggered CI

x86 is intentional because legacy `Disasm.cpp` contains inline x86 assembly that MSVC cannot compile in x64 mode.

## Current milestone: Decompiler compile-smoke COMPLETE

Run **#103** (`Make Decompiler numeric String mapping deterministic`, commit `b165aa480be68184863996d6f179cb905e13b25f`) is fully green.

The automated method audit reports:

- `TDecompiler` source methods found: **52**
- methods represented in compiled smoke slices: **52**
- coverage result: **COMPLETE**

All workflow prepare/compile steps are green in #103, including the final aggregated Decompiler helper slice and the portable `Disasm.cpp` smoke test.

This closes the method-by-method Decompiler compile-mapping phase. It does **not** prove runtime semantic equivalence.

## Proven portable at object-compilation level

Successfully compiled with hosted MSVC x86:

- `Disasm.h`, `KnowledgeBase.h`, `Infos.h`
- generated portable `Decompiler.h`
- real `Disasm.cpp` after generated compatibility transforms
- primary `Decompiler.cpp` implementation slice through `TDecompiler::Init()`
- complete BJL/branch-analysis path through `PrintBJL()`
- complete `TDecompiler::Decompile()` main engine
- complete `DecompileCaseEnum()` and `DecompileGeneralCase()`
- complete `DecompileTry()`
- complete syscall path (`GetSysCallAlias()` + `SimulateSysCall()`)
- complete call simulation (`SimulateInherited()` + `SimulateCall()`)
- complete comparison reconstruction (`GetCmpInfo()`)
- complete instruction simulation families: one-, two- and three-operand
- complete `SimulatePush()` / `SimulatePop()`
- complete `SimulateFloatInstruction()` / `SimulateFormatCall()` coverage
- case/general-case markers and `AnalyzeConditions()`
- final helper group identified by the 52/52 audit

Original IDR source remains unchanged; portability adaptations are generated under `tests/generated` or expressed as smoke-only compatibility declarations.

## Final helper group

The coverage audit initially found eight unrepresented methods:

- `GetArrayFieldOffset()`
- `GetCycleFrom()`
- `GetCycleIdx()`
- `GetCycleTo()`
- `GetFloatItemFromStack()`
- `GetInt64ItemFromStack()`
- `GetMemItem()`
- `GetStringArgument()`

They are now included in the aggregated special/push-pop translation unit and compile green in #103.

## Late-run history (#87-#103)

- #87 red: first integrated float compile exposed `True` and missing `GetGvarName`.
- #88 green: complete float instruction slice.
- #89 green: `SimulateFormatCall()`.
- #90 green: `MarkCaseEnum()` + `MarkGeneralCase()`.
- #91 red: `DecompileGeneralCase()` exposed numeric `String(_N...)` forms.
- #92 green: `DecompileGeneralCase()`.
- #93 green: explicit `DecompileTry()` special coverage.
- #94 red: `AnalyzeConditions()` needed `GetInvertCondition()` declaration.
- #95 green: `AnalyzeConditions()`.
- #97 red: new automated coverage audit found 44/52 represented and named the eight missing helpers.
- #98 red: audit reached **52/52**, but compile exposed both a harness extraction bug and genuine late compatibility dependencies.
- #100 red: 52/52 remained complete; remaining failures narrowed to helper declarations, `Currency`, and numeric `String(...)` forms.
- #101 red: narrowed further after `GetClassSize` and float fixes.
- #102 red: `Currency` issue was gone; remaining failures were numeric BCB `String(int)` conversions in the final helpers.
- #103 green: deterministic exact numeric String mapping made the complete 52/52 Decompiler smoke matrix compile green.

## Important #98 harness lesson

`GetArrayFieldOffset()` is formatted with a physical line break after `__fastcall`:

```cpp
int __fastcall
TDecompiler::GetArrayFieldOffset(...)
```

The old extractor assumed a one-line signature and started too early, producing false duplicate definitions of already-proven methods. The extractor was corrected; this was not treated as an IDR portability defect.

Method extraction must locate the qualified method name first and tolerate BCB formatting across physical lines.

## Compatibility layer status

`tests/portable_core_compat.h` remains a targeted compile-smoke layer, not a finished runtime compatibility library.

Compiler-verified differences currently mapped include:

- numeric `String(int)` -> controlled `std::to_string(...)`
- numeric zero assigned to `String` -> textual `"0"` where observed
- `.Length()` -> `.size()`
- `Pos()` and `SubString()` through helpers preserving 1-based semantics
- `SetLength()` -> `resize()` for observed smoke use
- `AnsiReplaceText`, `IntToStr`, `IntToHex`, `QuotedStr`
- temporary `AnsiString = std::string`
- minimal `WideString` wrapper
- targeted `Variant` boundary handling
- legacy float surface: `Comp`, `Currency`, `FT_*`, `FloatToStr`
- reached neutral kind `ikInterface`

The final numeric mapping deliberately uses exact `.Replace("String(<known numeric expression>)", ...)` operations instead of a broad global `String(...)` rewrite.

`String = std::string` is still only a compile approximation. Direct 1-based `String[index]` remains a known runtime-semantic hazard.

## Known warning / semantic risks

- `TDecompiler::DecompileTry()` produces MSVC C4715: not all control paths return a value. Do not silently change original behavior; address intentionally during real core integration/runtime validation.
- direct BCB 1-based String indexing can compile while behaving differently with `std::string`.
- legacy `Comp`, `Currency`, `Variant`, `WideString`, and container shims are not yet proven semantically equivalent.
- GUI/policy seams remain around embedded-procedure confirmation, form-owned method lookup, and `ManualInput(...)`.

## Architectural boundaries found

### `Main.h`

Core records/constants coexist with VCL GUI state. Extract neutral reusable definitions into a core header (`IdrTypes.h` / `CoreTypes.h`).

### `Misc.h`

Pure analysis helpers coexist with forms/canvas/dialog helpers. Separate the neutral analysis API from the UI half.

### Presentation functions

`OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly depend on VCL/form presentation and remain outside the headless core mapping.

### Small UI/policy seams inside analysis

Generated smoke code currently exposes neutral placeholders for embedded-procedure confirmation and form-owned virtual-method lookup. These should become explicit injected services/policies in the real core.

## Next phase: real core integration

**Stop adding method slices.** The Decompiler method inventory and compile matrix are complete.

Next work should move from synthetic compile slices to a real linkable portable core:

1. extract neutral core declarations/types from `Main.h` and `Misc.h`;
2. establish real core translation units instead of generated method aggregation;
3. compile/link those objects together and resolve the true extern/service boundary;
4. replace compiler-smoke-only String/RTL assumptions with runtime-correct behavior, especially 1-based indexing;
5. expose headless binary loading and analysis entry points;
6. link the first `idr-cli.exe`;
7. run it against known Delphi Win32 binaries and compare useful output with IDR behavior.

Keep the current slice matrix and 52/52 audit as regression coverage while integration proceeds.

## Working rules

- Preserve original source until a structural change has a clear portable-core purpose.
- Let concrete compiler/linker/runtime evidence drive compatibility changes.
- Keep harness defects separate from source portability defects.
- Stay x86 first.
- Do not fake VCL merely to satisfy the compiler.
- No merge to `main` or upstream PR yet.
- Fetch failed Actions logs once per run; successful runs need metadata verification only.
- Keep docs updated at meaningful milestones.

## Success criterion

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid CI tooling, or a self-hosted runner.
