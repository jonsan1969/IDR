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
- core compile commands are fail-fast (`cl ... || exit /b 1`)

x86 remains intentional because legacy `Disasm.cpp` contains x86 inline assembly and the shipped `dis.dll` is x86.

## Smoke phase: complete and frozen

`agent/portable-core-smoke` ended with complete 52/52 `TDecompiler` compile-smoke coverage. Keep that branch unchanged as regression/reference coverage.

Compile coverage alone does not establish runtime equivalence, especially for Borland 1-based `String` semantics.

## Current milestone: real PE32 ingestion reaches the portable core

Run **#53** succeeded at:

`a804f7f93e3e59bfddc02f8ef5f5cd25072401f5` — `Avoid Windows max macro in PE32 loader`

The integration workflow now has two runtime-tested paths:

1. the complete linked analysis-core probe containing generated real legacy `Disasm + KnowledgeBase + Infos + Misc + Decompiler` together with the neutral portable layers;
2. a neutral PE32 loader probe that constructs and loads a controlled Win32 PE image with `.text`, `.rsrc`, `.data` and `.bss` sections.

The loader preserves the analysis-facing semantics extracted from legacy `Main.cpp`:

- PE32 / x86 validation;
- `ImageBase`, `SizeOfImage` and absolute entry point extraction;
- section-address ordering and span validation;
- section span based on the next section RVA, with the last section using `VirtualSize`;
- raw-less sections treated as unbacked;
- resource and base-relocation sections treated as unbacked;
- legacy `0x80000` unbacked segment flag;
- only backed analysis spans stored in the packed byte image;
- `CodeBase` based on the first PE section;
- `CodeSize` equal to packed analysis size.

Verified loader chain:

`PE32 file -> neutral loader -> packed backed bytes + segment table -> ImageContext -> address/offset translation`

This moves the portable target beyond hand-constructed image bytes: it now has a real executable-file ingestion boundary.

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

### #51-53: neutral PE32 loader and runtime probe

The next phase extracted the executable-loading semantics needed by analysis from legacy `Main.cpp` into `IdrPeLoader`.

#51 and #52 both stopped at compile time because `Windows.h` defined the legacy `max` macro and collided with `std::numeric_limits<T>::max()`. The workflow's newly fail-fast compile commands correctly stopped at the real compiler error instead of deferring the symptom to the linker.

#53 made the Windows API include macro-safe and completed successfully. The PE32 loader probe now exercises the packed/unbacked section model at runtime.

## Current architecture boundary

The portable target now has:

- a real linked legacy analysis engine behind a headless policy/state boundary;
- a neutral x86 PE32 loader with runtime-tested segment packing;
- one neutral `ImageContext` address/offset model shared by analysis and loader tests.

The remaining gap is that activating a loaded PE currently populates `ImageContext`, while legacy session globals (`ImageSize`, `TotalSize`, `CodeBase`, `CodeSize`, entry point, `Flags`, `Infos`) are not yet initialized from the same authoritative loaded-image/session object.

That is the next boundary to remove before introducing the first useful CLI host.

## Known semantic risks

Loader/link/runtime success does not mean full behavioral equivalence.

Important open risks:

- direct Borland 1-based `String[index]` usage can differ from `std::string` 0-based indexing
- `WideString`, `Variant`, `Currency`, `Comp`, formatting and container behavior remain transitional
- `AnalysisState::SetFlags` / `ClearFlags` exact-end behavior still differs from the asymmetric legacy implementation and needs intentional resolution
- legacy warnings such as signedness, old CRT functions and suspicious shift counts remain non-blocking until their runtime paths are exercised
- `TDecompiler::DecompileTry()` and `GetStringArgument()` retain legacy not-all-paths-return warnings
- headless defaults for interactive services are intentionally conservative; a CLI host must supply policy where meaningful
- PE import/export/resource analysis beyond the image/segment ingestion boundary is not yet wired to the headless host.

## Immediate next phase: authoritative loaded session and first host

Do not return to method-slice work or linker-stub chasing.

Next steps:

1. bind one loaded PE object to both `ImageContext` and the legacy analysis session globals;
2. allocate/reset neutral analysis flags for the packed image and expose the legacy `Flags` view through that same state;
3. initialize the legacy `Infos` pointer array for the packed analysis size without inventing metadata;
4. runtime-test session activation/deactivation and address translation from a controlled PE32 input;
5. add a minimal headless executable host that opens a target and reports deterministic PE/core metadata;
6. evolve that host into `idr-cli.exe <target.exe>`;
7. then execute progressively deeper real analysis/decompiler paths and compare with original IDR on known Delphi Win32 binaries.

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
