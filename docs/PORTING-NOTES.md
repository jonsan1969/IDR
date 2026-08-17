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

Temporary aliases include `Byte`, `Word`, `DWord`, `String = std::string`, `AnsiString = std::string`, a minimal `WideString` wrapper, and a temporary integer-compatible `Variant` alias for compile mapping.

### Proven String / RTL mismatches

- #33: numeric `String(int)` differs; generated smoke code uses controlled `std::to_string(...)` transformations.
- #36: `String::Length()` maps to `.size()` for observed cases.
- #38: `AnsiReplaceText(...)` exposed with a narrow compile-time signature.
- #42: engine reaches `String::Pos()`, `IntToStr`, `IntToHex`, `AnsiString`, and further numeric `String(...)` calls.
- Later engine mapping also reaches `SubString()`, `SetLength()`, `QuotedStr`, and `WideString` behavior.

`Pos` and `SubString` helpers preserve Embarcadero's 1-based semantics in generated smoke code. Do not blindly substitute raw `std::string` indexing/find semantics.

Direct 1-based `String[index]` usage remains a known runtime-semantic risk even when it compiles. A thin compatibility String type may eventually be cleaner than an indefinitely growing transform set, but defer that decision until the observed indexing surface is mapped.

### Containers

`TList` remains a non-owning vector-backed smoke shim.

`TStringList` now supports:

- `Strings`
- aligned `Objects`
- sorted insertion that keeps `Strings`/`Objects` aligned
- `IndexOf`
- `IndexOfName` using `=` as the current name/value separator

This is targeted compatibility, not a claim of full VCL semantics.

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

Hit MSVC's 100-error cap almost entirely on missing core declarations/constants inherited through `Main.h`/`Misc.h`.

#### Run #42

Progressed much deeper. Exposed more pure analysis/type dependencies and the first direct engine UI policy: embedded-procedure confirmation through `FMain_11011981` / `Application->MessageBox`. The generated smoke copy maps this to `PortableConfirmEmbeddedProcedure(...)` rather than inventing VCL stubs.

`ManualInput(...)` is also reached by unresolved-call paths and is a future resolver/policy boundary.

#### Run #44

Progressed further and found another form-owned analysis API, `FMain_11011981->GetMethodInfo`. Smoke code maps this to neutral `PortableGetMethodInfo(...)`.

#44 also caught a bug in our own generator: an over-broad textual replacement transformed the tail of `GetImmString(...)` into `GetImmstd::to_string`. The replacement rules were tightened with explicit argument lists / guarded matching. This is a harness bug, not an IDR portability problem.

#### Run #46

Major convergence milestone: no 100-error cap. The remaining compile errors were a finite set dominated by:

- more numeric `String(...)` forms
- `TStringList::Objects` / `IndexOfName`
- `Variant`
- record/name helpers (`GetRecordFields`, `ExtractName`, `ExtractType`, `MakeGvar`, etc.)
- try-analysis helpers (`IsTryBegin`, `IsTryBegin0`)
- remaining core flags/kinds (`cfCode`, `cfETable`, `cfFinally`, `cfExcept`, `ikMethod`)

Those categories are now batched into the smoke compatibility layer/generator. Numeric String transforms remain deliberately controlled so legitimate `String(char*)` constructors are not corrupted.

The direction is encouraging: later engine failures are shrinking from broad dependency exposure toward concrete RTL/container semantics rather than discovering large new VCL regions.

## Mixed-responsibility headers

### `Main.h`

Core flags/kinds and GUI form state coexist. The growing engine constant set is strong evidence for neutral `CoreTypes.h` / `IdrTypes.h`.

### `Misc.h`

Many pure analysis APIs used by the engine coexist with forms/canvas/dialog helpers. A separate neutral analysis API header is increasingly justified.

### Form-owned analysis helpers

At least `GetMethodInfo` is analysis logic currently owned by `TFMain_11011981`. It should move behind a core service/context rather than stay on a form class.

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
