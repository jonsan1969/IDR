# Portable Core Project State

Last updated: 2026-08-17

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active product branch: `agent/portable-cli`

Frozen porting/integration reference: `agent/portable-core-integration`

Frozen compiler-smoke reference: `agent/portable-core-smoke`

`main` remains untouched at the modern Embarcadero baseline.

`agent/portable-cli` was rebuilt cleanly from `main` with one squash baseline commit containing the verified portable-core/session state, then the verified first CLI step was carried forward. The old integration history remains intact on `agent/portable-core-integration` for archaeology and evidence.

## CI

Workflow: `.github/workflows/portable-core-integration.yml`

On the active branch it is configured for `agent/portable-cli` and uses:

- GitHub-hosted Windows Server 2025 / VS2026
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`
- `actions/checkout@v6`
- fail-fast compile commands (`cl ... || exit /b 1`)
- concurrency cancellation
- `docs/**` ignored for push-triggered CI

x86 remains intentional because the real legacy `Disasm.cpp` contains x86 inline assembly and the shipped `dis.dll` is x86.

## Verified milestone entering the CLI branch

The old integration branch ended its useful active phase with run **#57**, commit:

`a0d72d9916b9c458ba3098f7a7d17354d536fba0` — `Introduce first portable IDR CLI`

Run #57 completed successfully. It built the full portable/legacy analysis chain, linked `idr-cli.exe`, built a real MSVC x86 PE32 fixture and executed:

`idr-cli.exe idr-cli-fixture.exe`

successfully.

The active branch starts from a clean portable-core baseline whose parent is directly `main`:

`52788d54...` — original `main`

`7f3f3fe6...` — `Establish portable MSVC x86 core baseline`

The CLI commit is layered on top of that clean baseline rather than carrying the entire experimental integration commit history.

## Current verified architecture

The portable target now contains and has runtime evidence for:

- real generated legacy `Disasm.cpp`
- complete generated legacy `KnowledgeBase.cpp`
- complete generated legacy `Infos.cpp`
- complete generated legacy `Misc.cpp`
- complete generated legacy `Decompiler.cpp` minus three GUI-owned presentation/orchestration methods
- neutral core types and services
- segment-aware `IdrImageContext`
- `AnalysisState`
- instruction navigation through the real decoder/dis.dll path
- neutral PE32/x86 loader
- legacy session bridge
- first real `idr-cli.exe` host

The verified end-to-end host path is now:

`PE32 target -> IdrPeLoader -> packed analysis bytes + segments -> legacy session activation -> linked real analysis core -> idr-cli.exe metadata output`

## Important integration milestones retained for reference

### #23: real decoder runtime

Generated real `Disasm.cpp` linked and executed against the repository's shipped x86 `dis.dll`.

### #24: segment-aware image mapping

`IdrImageContext` gained address/offset mapping compatible with legacy segment semantics, including unbacked `0x80000` segments.

### #31: whole real Decompiler TU compile

The complete real `Decompiler.cpp` compiled under MSVC x86, excluding only:

- `TDecompileEnv::OutputSourceCodeLine()`
- `TDecompileEnv::OutputSourceCode()`
- `TDecompileEnv::DecompileProc()`

### #32-48: linker-driven integration

The unresolved frontier moved:

`103 -> 89 -> 73 -> 46 -> 37 -> 29 -> 4`

while real `Misc`, `KnowledgeBase`, `Infos` and session/global state were integrated.

### #49-50: zero unresolved and runnable linked core

The last four explicit headless seams were connected through portable services or real legacy behavior. #50 linked and executed the integrated core probe with zero unresolved externals.

### #51-53: PE32 ingestion

A neutral PE32/x86 loader was extracted from the analysis-facing semantics of legacy `Main.cpp`. #53 completed successfully after the Windows `max` macro collision was removed.

The loader preserves:

- `ImageBase`, `SizeOfImage` and absolute entry point
- section-order/span validation
- next-section RVA span behavior
- raw-less/resource/relocation sections as unbacked
- legacy `0x80000` unbacked marker
- packed backed-analysis bytes
- first-section `CodeBase`
- packed-analysis `CodeSize`

### #54: one authoritative loaded session

`LoadedPeImage` was bound to both the neutral image context and the legacy analysis-facing state:

- `EP`
- `ImageBase`
- `ImageSize`
- `TotalSize`
- `CodeBase`
- `CodeSize`
- `Code`
- `AnalysisState -> Flags`
- `Infos[]`

Session activation and reset were runtime-tested.

### #57: first real CLI host

`idr-cli.exe <target.exe>` was introduced. It loads a PE32 target, activates the same session used by the legacy analysis core, verifies the neutral and legacy views agree, then reports deterministic image and segment metadata.

CI builds a real MSVC x86 executable fixture and runs the CLI against it.

## Current CLI behavior

`idr-cli.exe <target.exe>` currently reports:

- file path
- image base
- image size
- entry point
- code base
- code size
- packed analysis byte count
- segment count
- segment start/size/flags and backed/unbacked state
- confirmation that the legacy session is bound

It is a real host boundary, but not yet a useful decompiler frontend.

## Known semantic risks

Successful build/link/runtime probes do not establish complete behavioral equivalence.

Important open risks include:

- Borland 1-based direct `String[index]` semantics versus `std::string` 0-based indexing
- incomplete `WideString`, `Variant`, `Currency`, `Comp` and formatting fidelity
- exact legacy `SetFlags` / `ClearFlags` end-boundary behavior
- legacy warnings that have not yet reached meaningful runtime paths
- transitional register/string/container ABI compatibility
- analysis metadata ownership/lifetime as deeper paths begin creating `InfoRec` structures
- imports/exports/resources and Delphi-specific discovery are not yet driven by the CLI

## Immediate next phase

The linker chase is over and file/session loading is established. Work on `agent/portable-cli` should now be driven by real host behavior.

Preferred sequence:

1. add stable CLI options/output structure without over-designing a frontend;
2. initialize the real decoder from the loaded CLI session and prove entry-point decoding on a real PE32 target;
3. identify and invoke the first safe legacy analysis initialization path that populates flags/metadata;
4. audit reached Borland `String` indexing and RTL semantics as runtime paths become real;
5. add controlled Delphi Win32 fixtures and compare results against original IDR;
6. progressively expose procedures/types/metadata through CLI output;
7. later publish `idr-cli.exe` as a GitHub Actions artifact.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- Keep architecture-scale changes coherent.
- Do not disturb the frozen smoke branch.
- Treat `agent/portable-core-integration` as frozen reference after the #57 milestone.
- Do not push over an active `agent/portable-cli` workflow run.
- Preserve original legacy source unless a deliberate structural port warrants changing it.
- No upstream PR or merge to `main` yet.
- Keep these docs current at meaningful milestones.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> headless idr-cli.exe -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling, or a self-hosted runner.
