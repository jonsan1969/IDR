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

Current isolated slices require selected definitions copied into the compatibility layer instead of importing VCL-heavy `Main.h`:

```text
cfImport
cfPass
cfLoc
cfSkip
ikFloat
ikLString
ikRecord
ikFunc
```

This remains evidence for future `CoreTypes.h` / `IdrTypes.h` extraction.

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

A third independent slice has been added for complete `TDecompiler::Decompile()` only. It begins at:

```cpp
DWord __fastcall TDecompiler::Decompile(...)
```

and stops immediately before:

```cpp
DWord __fastcall TDecompiler::DecompileCaseEnum(...)
```

Generator: `tests/prepare_portable_decompiler_engine_slice.ps1`.

The CI workflow now compiles `tests/generated/Decompiler.engine.slice.cpp` as a separate object after the primary and BJL slices. This split is deliberate: the main engine is expected to expose a much larger helper/global dependency surface, and failures there should not obscure the already-proven BJL portability.

Triggering workflow commit: `c79fc41c60f64ebfbbb143355641fafc854a3ff5`.

## Mixed-responsibility headers

### `Main.h`

Contains both core structs/constants and VCL GUI state. Future split should move reusable definitions to a neutral header consumed by both core and GUI.

### `Misc.h`

Contains pure analysis helpers alongside `TForm`, `TCanvas`, clipboard and dialog helpers. The #30 -> #31 transition proved analysis dependencies can be exposed independently.

### `TypeInfo2.*`

Contains useful RTTI logic mixed with a VCL form. Later extraction candidates include `Guid2String`, `GetRTTI`, and `GetCppTypeInfo`.

### UI-only / initially excluded

- `InputDlg.*`
- `Resources.*`
- most `.dfm` presentation code
- direct `FMain_11011981` output plumbing

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
