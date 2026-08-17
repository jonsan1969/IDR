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

Observed hosted environment: Windows Server 2025 / `windows-2025-vs2026`, VS 2026, MSVC x86 through `vswhere.exe` + `vcvars32.bat`.

Use `actions/checkout@v6`; avoid Node 20 actions. `docs/**` does not trigger push CI. Stale runs are cancelled through concurrency.

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

Fundamental temporary aliases:

```cpp
using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;
using AnsiString = std::string;
```

### Proven String / RTL mismatches

- #33: numeric `String(int)` differs; generated smoke code uses `std::to_string(...)` for observed cases.
- #36: `String::Length()` maps to `.size()` in generated smoke code.
- #38: `AnsiReplaceText(...)` is exposed with a narrow compile-time signature.
- #42: engine reaches `String::Pos()`, `IntToStr`, `IntToHex`, `AnsiString`, and additional numeric `String(...)` calls.

For `Pos`, the generated engine smoke copy uses a helper that preserves Embarcadero semantics: 1-based result, zero when not found. Do not replace it blindly with raw `std::string::find()` semantics.

Do not introduce a full custom String wrapper until more runtime semantics are known.

### Core constants trapped in `Main.h`

The compatibility header only grows as tested source demands. By #42 it includes the engine-observed kinds for integer, char, enumeration, float, class, Ansi/Wide/Unicode string families, variant, array, record, Int64, VMT, constructor/destructor, procedure and function, plus the previously mapped analysis flags.

### `Exception`

Run #40 exposed Borland `Exception::Message`; the shim stores that public string alongside `std::runtime_error`.

## `Disasm.cpp`

Real implementation compiles with MSVC x86 after generated portability transforms. Legacy CRT and inline-x86 warnings remain deliberately separate from portability work.

## `Decompiler.cpp` mapping

### Primary slice

`GetString()` through complete `TDecompiler::Init()` — green at #27.

### Presentation GUI boundary

Immediately after `Init()`, `OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly use the VCL form and are intentionally excluded from core mapping.

### BJL / branch-analysis slice

`GetBJLRange()` through complete `PrintBJL()` — stable green at #38.

### Main engine slice

The third slice covers complete `TDecompiler::Decompile()` only, ending before `DecompileCaseEnum()`.

#### Run #40

Hit MSVC's 100-error cap almost entirely on missing core declarations/constants inherited through `Main.h`/`Misc.h`. First batch exposed address/flag helpers, proc/info helpers, decompiler helpers, analysis helpers, Int64 helpers, `Disasm`, `Code`, and `Exception::Message` without importing VCL-heavy headers.

#### Run #42

The first batch worked: compilation progressed much deeper into `Decompile()`. The next failure set still contains many pure analysis/type dependencies, including inline-div/mod/length detection, general-case/Int64 helpers, type/record/array functions, procedure metadata helpers and additional type-kind constants.

New RTL surface in #42:

```text
IntToStr
IntToHex
AnsiString
String::Pos
additional String(int/DWord) construction
```

Most importantly, #42 exposed the first direct UI/interactivity inside the main engine itself.

##### Embedded procedure policy

Original engine code temporarily changes `FMain_11011981->lbCode->ItemIndex` and calls `Application->MessageBox()` asking whether an embedded procedure should be decompiled.

The generated smoke copy does **not** fake VCL classes. It neutralizes only list-box selection state and maps the confirmation decision to:

```cpp
bool PortableConfirmEmbeddedProcedure(const String &address);
```

This is intentionally shaped like the future core boundary. A real headless API should support a deterministic policy such as always/never/callback.

##### Manual input policy

`ManualInput(...)` is also reached by the engine when return-byte counts or function types cannot be inferred. It remains an explicit compile dependency for now. In a real core it should become a resolver callback or deterministic CLI error/policy, not a dialog dependency.

This changes the architecture conclusion slightly: `TDecompiler::Decompile()` is overwhelmingly core analysis, but contains small embedded interaction decisions that must be injected/extracted.

## Mixed-responsibility headers

### `Main.h`

Core flags/kinds and GUI form state coexist. The growing engine constant set is strong evidence for neutral `CoreTypes.h` / `IdrTypes.h`.

### `Misc.h`

Many pure analysis APIs used by the engine coexist with forms/canvas/dialog helpers. A separate neutral analysis API header is increasingly justified.

### `TypeInfo2.*`

Useful RTTI logic remains mixed with a VCL form; later extraction candidate.

## Working rules

- Preserve original source during dependency mapping.
- Let concrete compiler errors drive shims/transforms.
- Avoid wholesale String/container rewrites until semantics are mapped.
- Do not fake VCL merely to make compile smoke green; identify policy/callback boundaries instead.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- No merge to `main` or upstream PR yet.

## First useful deliverable

```text
idr-cli.exe <target.exe>
```

built on stock GitHub-hosted Windows from real IDR core implementation code, capable of loading/analyzing a Delphi Win32 executable and emitting useful textual or machine-readable output.
