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

`agent/portable-core-smoke` ended with complete 52/52 `TDecompiler` compile-smoke coverage. Keep that branch unchanged as regression/reference coverage.

Compile coverage alone does not establish runtime equivalence, especially for Borland 1-based `String` semantics.

## Current milestone: linked and runnable real analysis core

Run **#50** succeeded at:

`242b4a841baeb93abe3eeb0802df262483c10c43` — `Fix portable proc-size offset handling`

The integration workflow now compiles, links and runs one MSVC x86 probe containing the real generated legacy translation units:

- `Disasm.cpp`
- `KnowledgeBase.cpp`
- `Infos.cpp`
- `Misc.cpp`
- `Decompiler.cpp` minus the three GUI-owned presentation/orchestration methods

alongside the neutral portable layers:

- `IdrCoreTypes`
- `IdrCoreServices`
- `IdrImageContext`
- `IdrAnalysis`
- `IdrAnalysisState`
- `IdrInstructionNav`
- `IdrDecompilerModel`
- transitional `IdrLegacyCompat`
- `IdrLegacyBridge`

Verified build/runtime chain:

`generated real legacy TUs -> MSVC x86 objects -> linked console probe -> process execution`

This is the first milestone where the large real analysis dependency chain is not merely compile-represented: it is resolved through the linker and executable on a stock GitHub-hosted runner.

## Integration chronology

### #23: real decoder runtime

The generated real `Disasm.cpp` translation unit linked and executed against the repository's shipped x86 `dis.dll`.

Verified path:

`image bytes -> ImageContext -> address translation -> MDisasm -> dis.dll -> DISINFO -> instruction navigation`

### #24: segment-aware image mapping

`IdrImageContext` gained segment-aware mapping matching legacy `SegmentList` / `Adr2Pos` / `Pos2Adr` behavior, including unbacked segments using the legacy `0x80000` flag.

### #25-26: neutral decompiler model

Neutral decompiler structures and the first runtime primitives were established. The legacy behavior that `AssignItem` does not copy `Offset` remains intentionally preserved.

### #31: whole real Decompiler TU compile

The generated portable build first compiled the complete real `Decompiler.cpp` translation unit, excluding only:

- `TDecompileEnv::OutputSourceCodeLine()`
- `TDecompileEnv::OutputSourceCode()`
- `TDecompileEnv::DecompileProc()`

These remain outside the core because they belong to presentation/orchestration, not analysis.

### #32-40: linker-driven dependency discovery and real Misc integration

Linking the real Decompiler object exposed 103 unresolved externals. Coherent compatibility/session bridges reduced that set while the complete real `Misc.cpp` translation unit was integrated.

By #40 both full `Misc.cpp` and full `Decompiler.cpp` compiled together and the linker frontier had fallen to 46 unresolved externals plus two duplicate generated helpers.

### #42-44: real KnowledgeBase integration

The complete real `KnowledgeBase.cpp` translation unit was added. Its compile frontier narrowed to two missing analysis declarations and one dead legacy UID stub before becoming compile-clean.

With real KnowledgeBase linked, the unresolved set fell further.

### #46-47: real Infos integration

The complete real `Infos.cpp` translation unit was added. After one compatibility pass, all four major real legacy units compiled together:

`KnowledgeBase + Infos + Misc + Decompiler`

The remaining linker set fell to 29 and was dominated by legacy session/global state.

### #48: portable session/state bridge

The portable bridge took ownership of the analysis state actually needed from legacy `Main.cpp`, including:

- `KnowledgeBase`, `Infos`, `Flags`
- image/code sizes
- segment/type/VMT lists
- VMT offsets
- string buffer state
- register-name tables
- class-address cache
- working-directory seam

This reduced the linker frontier from 29 unresolved externals to only four explicit headless analysis seams.

### #49-50: headless service seams and zero unresolved externals

The remaining seams were connected rather than stubbed away:

- `ManualInput` -> portable service callback
- method lookup -> portable service callback
- enumeration formatting -> real `Misc.cpp::GetEnumerationString`
- procedure-size estimation -> portable image/session/flag state
- embedded-procedure confirmation -> portable service callback

#49 exposed a local bridge compile bug in procedure-size offset handling. #50 corrected the `AddressToOffset()` contract and the entire integrated probe compiled, linked and executed successfully.

## Current architecture boundary

The portable target now has a real linked legacy analysis engine behind a headless policy/state boundary. It does **not** yet have a real executable-file ingestion path or useful CLI orchestration.

The next architectural boundary is therefore file loading, not more linker chasing.

## Known semantic risks

Link/runtime success of the probe does not mean full behavioral equivalence.

Important open risks:

- direct Borland 1-based `String[index]` usage can differ from `std::string` 0-based indexing
- `WideString`, `Variant`, `Currency`, `Comp`, formatting and container behavior remain transitional
- `AnalysisState::SetFlags` / `ClearFlags` exact-end behavior still differs from the asymmetric legacy implementation and needs intentional resolution
- legacy warnings such as signedness, old CRT functions and suspicious shift counts remain non-blocking until their runtime paths are exercised
- `TDecompiler::DecompileTry()` and `GetStringArgument()` retain legacy not-all-paths-return warnings
- headless defaults for interactive services are intentionally conservative; a CLI host must supply policy where meaningful

## Immediate next phase: PE ingestion and a useful host boundary

Do not return to method-slice work or linker-stub chasing.

Next steps:

1. isolate the real executable-loading semantics needed by analysis from the VCL `Main.cpp` path;
2. add a neutral PE32 loader that owns file bytes and populates `ImageView` / `ImageSegments` correctly;
3. bridge legacy image/session globals from that neutral loaded-image state instead of ad-hoc probe setup;
4. add runtime tests for PE address/offset/segment mapping using a controlled Win32 PE sample;
5. introduce a minimal headless host/CLI entry point capable of opening a target and reporting deterministic metadata before attempting full decompilation;
6. then execute progressively deeper real analysis/decompiler paths and compare with original IDR on known Delphi Win32 binaries.

## Working rules

- Preserve original source unless a structural portable-core change is intentional and documented.
- Let compiler, linker and runtime evidence drive changes.
- Distinguish harness defects from product portability defects.
- Stay x86 first.
- Avoid broad Borland emulation when a narrow neutral API is possible.
- Keep the smoke branch frozen.
- Do not push over an active integration run because concurrency cancels it.
- Do not merge to `main` or open an upstream PR yet.
- Keep docs updated at meaningful milestones.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> headless idr-cli.exe -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling, or a self-hosted runner.
