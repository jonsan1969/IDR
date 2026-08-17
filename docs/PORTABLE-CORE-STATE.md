# Portable Core Project State

Last updated: 2026-08-17

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active integration branch: `agent/portable-core-integration`

Frozen regression/reference branch: `agent/portable-core-smoke`

`main` remains untouched. Do not merge or open an upstream PR until the portable target is coherent and runtime-tested.

## CI

Integration workflow: `.github/workflows/portable-core-integration.yml`

- GitHub-hosted Windows Server 2025 / VS2026
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`
- `actions/checkout@v6`
- concurrency cancellation enabled
- `docs/**` ignored for push-triggered CI

x86 remains intentional because legacy `Disasm.cpp` contains x86 inline assembly and the shipped `dis.dll` is x86.

## Smoke phase: complete and frozen

`agent/portable-core-smoke` ended with the complete 52/52 `TDecompiler` compile-smoke matrix green. Keep that branch unchanged as regression/reference coverage.

Important smoke conclusion: compile coverage alone did not establish runtime equivalence, especially for Borland 1-based `String` semantics.

## Integration milestones

### Clean integration baseline

The integration branch was rebuilt directly from `main`, not from the smoke branch, so the real integration work has a clean baseline.

### Neutral core layers now present

- `IdrCoreTypes`
- `IdrCoreServices`
- `IdrImageContext`
- `IdrAnalysis`
- `IdrAnalysisState`
- `IdrInstructionNav`
- `IdrDecompilerModel`
- transitional `IdrLegacyCompat`

### Real `Disasm.cpp` / `dis.dll`

Run #23 established compile + link + runtime through the real generated `Disasm.cpp` translation unit and the shipped x86 `dis.dll`.

Verified path:

`image bytes -> ImageContext -> address translation -> MDisasm -> dis.dll -> DISINFO -> instruction navigation`

This established that the legacy decoder backend can be used from the MSVC x86 portable core without VCL.

### Segment-aware image mapping

Run #24 established a neutral segment-aware image model matching the original `SegmentList` / `Adr2Pos` / `Pos2Adr` behavior, including unbacked segments using the legacy `0x80000` flag.

Flat-image mode remains available as a compatibility/test special case.

### Neutral decompiler model and primitives

Runs #25-#26 established neutral decompiler data structures plus runtime-tested equivalents of the first legacy primitives (`InitItem`, `AssignItem`, direct/invert conditions). Legacy behavior that `AssignItem` does not copy `Offset` is intentionally preserved and tested.

## Current milestone: full legacy `Decompiler.cpp` compiles as one MSVC x86 TU

Run **#31** is green at commit:

`7addf0cd6116de1b21cc7e224792992428f62106` — `Handle final numeric String construction`

This is the first integration milestone where the generated portable build compiles **the whole legacy `Decompiler.cpp` as one translation unit**, rather than method slices.

The workflow now generates:

- `tests/generated/Decompiler.portable.h`
- `tests/generated/Misc.portable.h`
- `tests/generated/Decompiler.portable.cpp`

and produces:

- `Decompiler.portable.obj`

The object is compile-green but is **not yet linked into the integration probe**. That is the next phase.

## What was required to make the full TU compile

The full-TU generator intentionally transforms only the integration copy; original legacy source remains unchanged.

Current compatibility seams include:

- `String = std::string` as a transitional representation
- explicit numeric `String(...) -> std::to_string(...)` mapping for known numeric expressions
- `.Length()` -> `.size()`
- 1-based `Pos()` and `SubString()` helpers
- `SetLength()` / `IsEmpty()` transformations where reached
- `True` / `False` normalization
- narrow `Currency` and enumeration/Variant seams
- embedded-procedure confirmation callback seam
- form-owned virtual-method lookup seam

The presentation-owned definitions are intentionally excluded from the portable Decompiler TU:

- `TDecompileEnv::OutputSourceCodeLine()`
- `TDecompileEnv::OutputSourceCode()`
- `TDecompileEnv::DecompileProc()`

Do not fake VCL to pull them into the core.

## Known risks / warnings

Compile-green does not mean semantic equivalence.

Important remaining risks:

- direct Borland 1-based `String[index]` usage can still differ from `std::string` 0-based indexing
- `WideString`, `Variant`, `Currency`, `Comp`, container shims and formatting behavior remain transitional
- legacy compiler warnings (including old `sprintf` / `sscanf` usage) are currently non-blocking
- several legacy shift expressions trigger MSVC warnings and need intentional semantic review later
- `TDecompiler::DecompileTry()` historically has a non-returning control path warning; do not silently modernize behavior
- `ManualInput(...)` still needs an explicit deterministic headless service/policy

## Immediate next phase: linker-driven Decompiler integration

Do not return to method-slice expansion.

Next steps:

1. add `Decompiler.portable.obj` to the integration link target;
2. capture the unresolved external set from the linker;
3. classify each unresolved symbol as neutral core state, legacy subsystem dependency, or UI/policy seam;
4. implement/bridge those dependencies in coherent groups rather than adding arbitrary stubs;
5. keep real `dis.dll` runtime coverage green while resolving the linker frontier;
6. once the full Decompiler object links, begin runtime construction/execution of a minimal `TDecompileEnv` / `TDecompiler` path;
7. then expose PE loading and a minimal `idr-cli.exe <target.exe>` entry point.

## Working rules

- Preserve original source unless a structural portable-core change is intentional and documented.
- Let compiler, linker and runtime evidence drive changes.
- Distinguish harness defects from product portability defects.
- Stay x86 first.
- Avoid broad Borland emulation when a narrow neutral API is possible.
- Keep the smoke branch frozen.
- Do not merge to `main` or open an upstream PR yet.
- Keep docs updated at meaningful milestones.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> headless idr-cli.exe -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling, or a self-hosted runner.
