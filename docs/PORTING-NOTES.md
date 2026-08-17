# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical journal for the experimental MSVC/GitHub-hosted portability work.

## Repository / branch

- Original: `crypto2011/IDR`
- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Experimental branch: `agent/portable-core-smoke`

The experiment asks whether the analysis/decompiler core can become compiler-neutral enough for stock GitHub-hosted MSVC while the existing VCL application remains intact.

## CI

Workflow: `.github/workflows/portable-core-smoke.yml`

Hosted environment observed: Windows Server 2025 / VS2026, MSVC x86 via `vswhere.exe` + `vcvars32.bat`.

Use `actions/checkout@v6`; `docs/**` does not trigger push CI; stale runs are cancelled by concurrency.

For a failing run, fetch the job log once and analyze that result. Successful runs are verified from run/job/step metadata without fetching logs.

## Harness / regression assets

Important files:

- `tests/portable_core_compat.h`
- `tests/prepare_portable_decompiler*.ps1`
- `tests/audit_portable_decompiler_coverage.ps1`
- `tests/prepare_portable_disasm.ps1`

Generated transformed files live under `tests/generated` during CI. Original IDR source remains unchanged.

The extended push/pop translation unit became the aggregate proof target for late Decompiler helpers, avoiding a proliferation of nearly identical workflow steps.

## Decompiler method coverage milestone

The automated audit compares `TDecompiler::...` implementations in `Decompiler.cpp` against the generated translation units actually compiled by CI.

Run **#103** is fully green and establishes:

- source `TDecompiler` methods found: **52**
- methods represented in compiled slices: **52**
- coverage audit: **COMPLETE**
- every compile step in the workflow green

This ends the method-by-method compile-smoke expansion phase. The existing slices should now be retained as regression coverage rather than extended further.

## Final eight methods found by the audit

#97 first reported 44/52 represented and identified exactly eight gaps:

- `GetArrayFieldOffset()`
- `GetCycleFrom()`
- `GetCycleIdx()`
- `GetCycleTo()`
- `GetFloatItemFromStack()`
- `GetInt64ItemFromStack()`
- `GetMemItem()`
- `GetStringArgument()`

They are all included and compile-green by #103.

## Late-run chronology

### Float / format / case / try / conditions

- #85 green: complete `SimulatePush()` + `SimulatePop()`.
- #86: generator-only float intermediate.
- #87 red: first integrated `SimulateFloatInstruction()` compile exposed BCB `True` and missing `GetGvarName` declaration.
- #88 green: complete `SimulateFloatInstruction()`.
- #89 green: complete `SimulateFormatCall()`.
- #90 green: `MarkCaseEnum()` + `MarkGeneralCase()`.
- #91 red: complete `DecompileGeneralCase()` exposed numeric `String(_N)`, `String(_N1)`, `String(_N1-_N2)`, `String(_N1-1)` semantics.
- #92 green: complete `DecompileGeneralCase()`.
- #93 green: explicit `DecompileTry()` special-slice coverage.
- #94 red: `AnalyzeConditions()` needed `GetInvertCondition(char)` declaration.
- #95 green: complete `AnalyzeConditions()`.

`DecompileTry()` still emits MSVC C4715 (not all control paths return a value). Preserve this as a known source-level semantic warning; do not silently alter original behavior during smoke mapping.

### Coverage and final helpers

- #97 red: audit added; 44/52 represented; eight missing helpers named.
- #98 red: prepare/audit reached **52/52 COMPLETE**, then final-helper compilation failed.
- #100 red: 52/52 remained complete while compatibility failures narrowed.
- #101 red: `GetClassSize` and earlier float/helper dependencies were removed from the failure set.
- #102 red: `Currency` was resolved; remaining errors were BCB numeric `String(int)` constructions.
- #103 green: deterministic exact numeric mapping closed the final compile failures.

## #98 extraction bug

`GetArrayFieldOffset()` uses this formatting:

```cpp
int __fastcall
TDecompiler::GetArrayFieldOffset(...)
```

The first special extractor assumed the qualified method name followed `int __fastcall ` on the same physical line. `LastIndexOf(...)` therefore selected an earlier boundary and accidentally appended a large source range. False duplicate-body diagnostics followed for already-covered methods such as `DecompileTry()`, `MarkCaseEnum()` and `GetCycle*()`.

This was a harness defect, not an IDR portability failure. The extractor was corrected to tolerate the split declaration.

General rule: find the qualified method name first and derive a robust declaration boundary; never assume BCB formatting keeps return type, calling convention and method name on one line.

## Final numeric String mapping lesson

The late helper code contains BCB constructions such as:

```cpp
String(DisInfo.Immediate)
String(_offset)
String(-_offset)
String(_offset + 1)
String(_offset - _foffset)
String(_k)
String(-_k)
```

A global `String(...)` rewrite is unsafe because legitimate pointer/string constructors also exist. Earlier regex-based narrow mapping became hard to reason about for exact expressions.

#103 switched the known numeric-expression table to deterministic literal operations of the form:

```powershell
$body = $body.Replace("String($arg)", "std::to_string($arg)")
```

with a small explicit set of arithmetic expressions handled separately. This is intentionally compile-smoke-specific and avoids touching legitimate `String(char*)` forms.

## Compatibility layer status

Temporary aliases/representations include `Byte`, `Word`, `DWord`, `String = std::string`, `AnsiString = std::string`, minimal `WideString`, and targeted smoke types for reached Delphi/BCB constructs.

### Compiler-verified String / RTL differences

- numeric `String(int)` needs explicit decimal conversion
- numeric zero assigned to `String` needs explicit textual representation
- `.Length()` differs from `std::string::size()`
- `Pos()` and `SubString()` require preservation of Delphi/BCB 1-based semantics
- `SetLength()` is mapped to `resize()` only for observed compile-smoke use
- `AnsiReplaceText`, `IntToStr`, `IntToHex`, `QuotedStr` are explicit dependencies
- direct integer/String concatenation must be made explicit

Direct 1-based `String[index]` remains a major runtime-semantic risk despite compile success. Examples in original code include checks such as `_name[1]` and `_retType[1]`.

### Legacy float / Variant surface

Late helper mapping reaches:

- `Comp`
- `Currency`
- `FT_SINGLE`, `FT_REAL`, `FT_DOUBLE`, `FT_COMP`, `FT_CURRENCY`, `FT_EXTENDED`
- `FloatToStr(...)`
- an implicit String/Variant boundary

These are represented only far enough for compile mapping. Runtime representation and formatting equivalence remain future work.

### Core kinds / helper declarations

Late mapping also reaches `ikInterface` (`0x0F` in original `Main.h`), `GetArrayIndexes(...)`, and `GetClassSize(...)`. These belong to the neutral analysis surface rather than VCL UI.

## Established compile milestones

- #27: `TDecompiler::Init()`
- #38: BJL path through `PrintBJL()`
- #49: complete `TDecompiler::Decompile()`
- #51: `DecompileCaseEnum()`
- #58: syscall path
- #62: call simulation
- #64: `GetCmpInfo()`
- #66: `SimulateInstr1()`
- #69/#71/#74/#76/#78/#80: complete two-operand family and dispatcher
- #83: `SimulateInstr3()`
- #85: `SimulatePush()` + `SimulatePop()`
- #88: `SimulateFloatInstruction()`
- #89: `SimulateFormatCall()`
- #90: case markers
- #92: `DecompileGeneralCase()`
- #93: explicit `DecompileTry()` coverage
- #95: `AnalyzeConditions()`
- #103: **complete 52/52 Decompiler compile-smoke matrix green**

## GUI / policy seams discovered

Do not fake VCL to keep the compiler happy. The smoke harness exposes these as boundaries instead:

- embedded-procedure confirmation -> future core policy/callback
- form-owned virtual-method lookup (`FMain_11011981->GetMethodInfo`) -> future analysis service/context
- `ManualInput(...)` -> future resolver callback or deterministic CLI policy

Presentation functions `OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` remain intentionally outside the headless core.

## Mixed-responsibility headers

### `Main.h`

Core records/constants coexist with VCL form state. Extract neutral core definitions into `IdrTypes.h` / `CoreTypes.h`.

### `Misc.h`

Pure analysis APIs coexist with canvas/forms/dialog helpers. Split neutral analysis declarations away from the UI half.

## Next technical phase: integration, not more slices

The next work should establish a real portable core target:

1. extract neutral core declarations/types from `Main.h` and `Misc.h`;
2. build real core translation units rather than generated method aggregates;
3. link those objects and let linker failures expose the true dependency/service boundary;
4. replace compile-only String/RTL assumptions with runtime-correct semantics;
5. expose headless binary loading / analysis entry points;
6. produce first linked `idr-cli.exe`;
7. run against known Delphi Win32 binaries and compare useful output with the original IDR behavior.

The 52/52 slice matrix remains CI regression coverage while this structural work proceeds.

## Working rules

- Preserve original source unless a structural portable-core change is intentional and documented.
- Let compiler, linker and runtime evidence drive compatibility changes.
- Keep harness defects distinct from source portability defects.
- Avoid broad String/container rewrites without semantic evidence.
- Stay x86 first.
- No merge to `main` or upstream PR yet.

## First useful deliverable

```text
idr-cli.exe <target.exe>
```

built on stock GitHub-hosted Windows from real IDR core implementation code, capable of loading/analyzing a Delphi Win32 executable and emitting useful textual or machine-readable output.
