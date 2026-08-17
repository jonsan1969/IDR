# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical scratchpad for the experimental MSVC/GitHub-hosted portability work.

## Repository / branch

- Original: `crypto2011/IDR`
- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Experimental branch: `agent/portable-core-smoke`

The experiment asks whether the analysis/decompiler core can become compiler-neutral enough for stock GitHub-hosted MSVC while the existing VCL application remains intact.

## CI

Workflow: `.github/workflows/portable-core-smoke.yml`

Observed hosted environment:

- Windows Server 2025
- `windows-2025-vs2026`
- VS 2026 Developer Command Prompt 18.8.2
- x86 environment via `vswhere.exe` + `vcvars32.bat`

Actions policy:

- `actions/checkout@v6`
- avoid Node.js 20 actions
- no `ilammy/msvc-dev-cmd@v1`
- `docs/**` ignored for push CI
- stale runs cancelled with workflow concurrency

Do not fetch successful run logs. For a failing run, fetch the log once, analyze it from the returned result, and fetch again only for a new run.

## Harness files

- `tests/portable_core_compat.h`
- `tests/prepare_portable_disasm.ps1`
- `tests/prepare_portable_decompiler.ps1`
- `tests/prepare_portable_decompiler_slice.ps1`
- `tests/prepare_portable_decompiler_branch_slice.ps1`
- `tests/prepare_portable_decompiler_engine_slice.ps1`

Generated transformed files live under `tests/generated` during CI. Original IDR source remains unchanged.

## Compatibility layer

### Fundamental aliases

```cpp
using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;
```

`String = std::string` is temporary and compile-oriented.

### Proven String mismatches

Run #33: numeric construction such as `String(m)` / `String(m + 1)` is not source-compatible with `std::string`; generated smoke code maps observed forms to `std::to_string(...)`.

Run #36: Embarcadero `String::Length()` is mapped to `std::string::size()` in generated smoke code.

Run #38: `PrintBJL()` additionally needs numeric `String(k)` and `AnsiReplaceText(...)`. Numeric construction is mapped to `std::to_string(k)`; compile-only validation exposes a narrow String signature for `AnsiReplaceText()` rather than importing `System.StrUtils.hpp` or prematurely implementing RTL semantics.

Do not introduce a large custom String wrapper until more semantics are mapped. Still watch 1-based indexing, `Pos()`, `SubString()`, case-insensitive helpers, ANSI/Unicode behavior and other RTL conversions.

### Containers

Current STL-backed `TList` shim supports `Count`, `Items[]`, `Add()`, `Clear()`, and `Delete(index)`. Current `TStringList` shim supports `Sorted`, `Count`, `Strings`, `Add()`, and `IndexOf()`.

### Core constants trapped in `Main.h`

The compatibility layer now contains only constants reached by tested slices, including the engine additions from run #40:

```text
cfImport, cfFrame, cfSwitch, cfDSkip, cfPass, cfLoc, cfTry, cfLoop,
cfFinallyExit, cfSkip,
ikUnknown, ikFloat, ikLString, ikRecord, ikConstructor, ikDestructor, ikFunc
```

This increasingly supports a future `CoreTypes.h` / `IdrTypes.h` extraction.

### `Exception`

The initial `std::runtime_error` wrapper was sufficient until the main engine. Run #40 exposed Borland-style catch/rethrow code that reads `Exception::Message`. The shim now stores a public `String Message` initialized alongside the standard exception base.

## `Disasm.cpp`

Real implementation compiles with MSVC x86 after generated portability transforms. Legacy CRT warnings and inline-x86 `ebp` warnings are non-blocking and intentionally not modernized during mapping.

## `Decompiler.cpp` mapping

### Primary slice

Starts at `GetString()` and runs through complete `TDecompiler::Init()`.

Run #27: fully green.

### GUI boundary

Immediately after `TDecompiler::Init()` are `OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()`. Direct form output and Embarcadero-specific String behavior make these presentation/orchestration code, so they are intentionally skipped.

### BJL / branch-analysis slice

Starts at `TDecompileEnv::GetBJLRange()` and now runs through complete `PrintBJL()`.

Milestones:

- #30 red: missing `BranchGetPrevInstructionType()` declaration.
- #31 green: complete `GetBJLRange()`.
- #33 red: numeric `String(int)` mismatch.
- #34 green: complete `CreateBJLSequence()`.
- #35 green: complete `UpdateBJLList()` + `BJLAnalyze()`.
- #36 red: `.Length()` mismatch.
- #37 green: all non-printing BJL helpers through `ExprMerge()`.
- #38 green: complete `PrintBJL()`.

This BJL slice is now treated as a stable independently green compile block.

### Main engine slice

The third slice covers complete `TDecompiler::Decompile()` only, ending immediately before `DecompileCaseEnum()`.

Run #40 hit the MSVC 100-error cap. The errors were highly repetitive and fell into a few dependency families rather than 100 separate portability problems. No VCL symbol appeared in the observed batch.

First batch exposed explicitly in the engine generator:

```text
Adr2Pos / Pos2Adr
Val2Str8
IsFlagSet / SetFlag / SetFlags / ClearFlag
GetProcSize / GetInfoRec / GetNearestUpInstruction
GetDecompilerRegisterName
InitItem
IsIntOver / IsExit / IsValidCodeAdr
SameText
IsInheritsByProcName / ExtractProcName
GetDirectCondition / BranchGetPrevInstructionType
IsInt64ComparisonViaStack1 / IsInt64ComparisonViaStack2
Disasm / Code
```

This is a compile-surface extraction only. Implementations remain in the original source tree and are not duplicated into the smoke harness.

The `SimulateInstr1()` diagnostic seen in #40 is not yet treated as a real signature bug because it occurred while `Disasm` itself was undeclared; original call sites use the declared two-argument form. Re-evaluate only if the diagnostic survives after dependency declarations are visible.

## Mixed-responsibility headers

### `Main.h`

Contains both core structs/constants and VCL GUI state. The main-engine dependency batch makes the case for a neutral core definitions header stronger.

### `Misc.h`

Contains pure analysis helpers alongside `TForm`, `TCanvas`, clipboard and dialog helpers. The engine uses many of those pure helpers without needing the UI half, strengthening the case for a separate core-analysis API header.

### `TypeInfo2.*`

Contains useful RTTI logic mixed with a VCL form. Later extraction candidates include `Guid2String`, `GetRTTI`, and `GetCppTypeInfo`.

## Working rules

- Preserve original source during dependency mapping.
- Let concrete compiler errors drive shims/transforms.
- Avoid wholesale container/String rewrites until semantics are mapped.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- Do not port the GUI merely to make CI green.
- No merge to `main` or upstream PR yet.

## First useful deliverable

Target:

```text
idr-cli.exe <target.exe>
```

built on stock GitHub-hosted Windows from real IDR core implementation code, capable of loading/analyzing a Delphi Win32 executable and emitting useful textual or machine-readable output.
