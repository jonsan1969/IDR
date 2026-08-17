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

### Proven String mismatch

Run #33 established the first concrete Embarcadero String incompatibility:

```cpp
String(m)
String(m + 1)
```

Embarcadero converts integers to decimal text; `std::string` has no such one-argument constructor. The generated BJL smoke copy therefore translates only these observed forms to:

```cpp
std::to_string(m)
std::to_string(m + 1)
```

Run #34 proved that narrow transform is sufficient for the complete `CreateBJLSequence()` span.

Do not introduce a large custom String wrapper prematurely. Remaining semantics to map include:

- 1-based indexing
- `Pos()`
- `SubString()`
- case-insensitive helpers
- Unicode/ANSI behavior
- additional numeric constructors/conversions

### `TList`

Current STL-backed shim supports:

- `Count`
- `Items[index]`
- `Add(void *)`
- `Clear()`
- `Delete(index)`

`Clear()` / `Delete()` are non-owning; IDR code deletes pointed-to objects explicitly where needed.

### `TStringList`

Current minimal shim supports:

- `Sorted`
- `Count`
- `Strings`
- `Add()`
- `IndexOf()`

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

Do not modernize these while mapping portability unless required.

x64 is a separate later project because MSVC does not support inline asm in x64 mode.

## `Decompiler.cpp` mapping

The file is large and is intentionally mapped using multiple real implementation slices rather than one giant conversion.

### Primary slice

Starts at `GetString()` and runs through complete `TDecompiler::Init()`.

Run #27: fully green.

This span includes:

- naming / condition helpers
- `ITEM` manipulation
- namer and loop/environment objects
- saved context
- decompiler construction/destruction
- flags
- register state
- normal stack
- FPU stack
- prototype checking
- calling convention argument setup
- return-value setup

### GUI boundary

Immediately after `TDecompiler::Init()`:

- `OutputSourceCodeLine()`
- `OutputSourceCode()`
- `DecompileProc()`

The first writes directly to `FMain_11011981->lbSourceCode`; output logic also uses Embarcadero-specific String behavior. This block is intentionally skipped, not treated as a core blocker.

### BJL / branch-analysis slice

Starts at `TDecompileEnv::GetBJLRange()`.

Dependencies exposed explicitly instead of importing mixed GUI headers:

- global `Disasm`
- global `Code`
- `Adr2Pos()`
- `IsFlagSet()`
- `BranchGetPrevInstructionType()`
- `GetDirectCondition()`
- `GetInvertCondition()`
- `cfSkip`

Milestones:

- #30 red: missing declaration for `BranchGetPrevInstructionType()`.
- #31 green: complete `GetBJLRange()` compiles.
- #33 red: only numeric `String(int)` construction fails inside `CreateBJLSequence()`.
- #34 green: complete `CreateBJLSequence()` compiles after narrow generated `std::to_string()` conversion.

### Active BJL expansion

Current branch-slice end marker has moved to immediately before:

```cpp
bool __fastcall TDecompileEnv::BJLGetIdx(...)
```

Therefore the active compile now additionally includes:

- complete `UpdateBJLList()`
- complete `BJLAnalyze()`

Triggering commit: `cef522d6e7b4aec18002b2a9e7a298a9203ab8ad`.

This is the next meaningful test of expression merging/pattern analysis and container mutation without crossing into GUI code.

## Mixed-responsibility headers

### `Main.h`

Contains both core structs/constants and VCL GUI state. Future split should move reusable definitions to a neutral header consumed by both core and GUI.

### `Misc.h`

Contains pure analysis helpers such as `BranchGetPrevInstructionType()` alongside `TForm`, `TCanvas`, clipboard and dialog helpers. The #30 -> #31 transition proved the analysis dependency can be exposed independently.

### `TypeInfo2.*`

Contains useful RTTI logic mixed with a VCL form. Later candidate extraction:

- `Guid2String`
- `GetRTTI`
- `GetCppTypeInfo`

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
