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

Do not fetch successful run logs. For a failing run, fetch the log once, analyze it locally/from the returned result, and fetch again only for a new run.

## Harness files

- `tests/portable_core_compat.h`
- `tests/portable_disasm_header_smoke.cpp`
- `tests/portable_kb_header_smoke.cpp`
- `tests/portable_infos_header_smoke.cpp`
- `tests/portable_decompiler_header_smoke.cpp`
- `tests/prepare_portable_disasm.ps1`
- `tests/prepare_portable_decompiler.ps1`
- `tests/prepare_portable_decompiler_slice.ps1`
- `tests/prepare_portable_decompiler_branch_slice.ps1`

Generated transformed files live under `tests/generated` during CI. Original IDR source remains unchanged.

## Compatibility layer

### Fundamental aliases

```cpp
using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;
```

`String = std::string` is temporary and only compile-oriented.

### Proven String mismatches

Run #33 established numeric construction:

```cpp
String(m)
String(m + 1)
```

Embarcadero converts integers to decimal text; the generated smoke copy maps observed forms to `std::to_string(...)`. Run #34 proved that sufficient for complete `CreateBJLSequence()`.

Run #36 established the second mismatch: Embarcadero `String::Length()` has no `std::string` member equivalent. The generated smoke copy maps observed `.Length()` calls to `.size()`. Run #37 proved that sufficient for the complete non-printing BJL helper span through `ExprMerge()`.

Current `PrintBJL()` mapping adds numeric `String(k)` -> `std::to_string(k)`.

Do not introduce a large custom String wrapper prematurely. Remaining semantics to map include:

- 1-based indexing
- `Pos()`
- `SubString()`
- case-insensitive helpers / `AnsiReplaceText`
- Unicode/ANSI behavior
- additional numeric constructors/conversions

### `AnsiReplaceText`

`PrintBJL()` uses `AnsiReplaceText()` from `System.StrUtils.hpp`. For the current compile-only slice, expose only a compatible String signature in the generated prefix rather than importing VCL/RTL headers or implementing replacement semantics prematurely. Runtime/link semantics must be supplied before a real portable executable/library target links this path.

### `TList`

Current STL-backed shim supports:

- `Count`
- `Items[index]`
- `Add(void *)`
- `Clear()`
- `Delete(index)`

`Clear()` / `Delete()` are non-owning; IDR code deletes pointed-to objects explicitly where needed.

### `TStringList`

Current minimal shim supports `Sorted`, `Count`, `Strings`, `Add()`, and `IndexOf()`.

Still watch duplicate handling, case sensitivity, ownership/Objects, sorted insertion and encoding semantics.

### `Exception`

Borland `Exception` is represented by a small `std::runtime_error` wrapper for the current smoke code.

### Core constants trapped in `Main.h`

Current isolated slices require selected definitions copied into the compatibility layer rather than importing VCL-heavy `Main.h`:

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

This is evidence for a future neutral `CoreTypes.h` / `IdrTypes.h`.

## `Disasm.cpp`

Real implementation compiles with MSVC x86 after generated portability transforms.

Known non-blocking warnings/issues:

- old CRT calls (`sprintf`, `strcat`, `strcpy`)
- unused locals
- inline x86 asm modifies `ebp`

Do not modernize these while mapping portability unless required. x64 is a separate later project because MSVC does not support inline asm in x64 mode.

## `Decompiler.cpp` mapping

### Primary slice

Starts at `GetString()` and runs through complete `TDecompiler::Init()`.

Run #27: fully green.

This span includes naming/condition helpers, `ITEM` manipulation, namer/loop/environment objects, saved context, flags, register state, normal/FPU stacks, prototype checking, calling-convention argument setup and return-value setup.

### GUI boundary

Immediately after `TDecompiler::Init()` are `OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()`. Direct `FMain_11011981` output and Embarcadero String behavior make this a presentation/orchestration boundary, so it is intentionally skipped.

### BJL / branch-analysis slice

Starts at `TDecompileEnv::GetBJLRange()`.

Dependencies exposed explicitly instead of importing mixed GUI headers include global `Disasm`, global `Code`, `Adr2Pos()`, `IsFlagSet()`, `BranchGetPrevInstructionType()`, `GetDirectCondition()`, `GetInvertCondition()`, and `cfSkip`.

Milestones:

- #30 red: missing declaration for `BranchGetPrevInstructionType()`.
- #31 green: complete `GetBJLRange()`.
- #33 red: numeric `String(int)` mismatch inside `CreateBJLSequence()`.
- #34 green: complete `CreateBJLSequence()` after narrow `std::to_string()` transform.
- #35 green: complete `UpdateBJLList()` and `BJLAnalyze()` with no new shim.
- #36 red: `.Length()` mismatch in the expanded helper block.
- #37 green: complete helper span through `ExprMerge()` after `.Length()` -> `.size()` transform.

### Active BJL expansion: `PrintBJL()`

The branch-slice end marker is now immediately before `TDecompiler::Decompile()`, so the active compile additionally includes complete `TDecompileEnv::PrintBJL()`.

Observed dependencies:

```cpp
String(k)
AnsiReplaceText(...)
```

The generated smoke copy maps `String(k)` to `std::to_string(k)` and declares only the String signature of `AnsiReplaceText()` for compile-only validation. Triggering commit: `5170af65ceb3fdf2edf3e728b85ec007d903d1be`.

## Mixed-responsibility headers

### `Main.h`

Contains both core structs/constants and VCL GUI state. Future split should move reusable definitions to a neutral header consumed by both core and GUI.

### `Misc.h`

Contains pure analysis helpers such as `BranchGetPrevInstructionType()` alongside `TForm`, `TCanvas`, clipboard and dialog helpers. The #30 -> #31 transition proved the analysis dependency can be exposed independently.

### `TypeInfo2.*`

Contains useful RTTI logic mixed with a VCL form. Later candidate extraction: `Guid2String`, `GetRTTI`, `GetCppTypeInfo`.

### UI-only / initially excluded

- `InputDlg.*`
- `Resources.*`
- most `.dfm` presentation code
- direct `FMain_11011981` output plumbing

## Run milestone summary

- #13: headers + portable Decompiler header + real Disasm implementation green.
- #15: first real Decompiler implementation slice green.
- #17: expanded Decompiler slice green with initial STL containers.
- #19: red only on missing `cfPass` / `cfLoc`.
- #23: green through register and normal-stack handling.
- #24: red only on missing `ikFunc`.
- #25: green through FPU stack + `CheckPrototype()`.
- #26: red only on `cfImport`, `ikFloat`, `ikLString`, `ikRecord`.
- #27: green through complete `TDecompiler::Init()`.
- #30: red only on missing core-helper declaration.
- #31: green through complete `GetBJLRange()`.
- #33: red only on `String(int)` semantics.
- #34: green through complete `CreateBJLSequence()`.
- #35: green through `UpdateBJLList()` + `BJLAnalyze()`.
- #36: red only on `.Length()` semantics.
- #37: green through all non-printing BJL helpers up to `ExprMerge()`.

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
