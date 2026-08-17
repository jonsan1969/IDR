# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen porting/integration reference: `agent/portable-core-integration`
- Frozen smoke/reference branch: `agent/portable-core-smoke`

`agent/portable-cli` is intentionally a clean branch from `main`. Its first commit after `main` is a squash baseline containing the verified portable core/session state. The full experimental history remains available on `agent/portable-core-integration`.

## Smoke lessons carried forward

- avoid giant Borland `String` wrappers without semantic evidence;
- preserve x86 ABI requirements where legacy code depends on them;
- Borland 1-based `String` semantics are a real runtime hazard;
- GUI/policy seams should be abstracted, not satisfied by fake VCL;
- compile representation is evidence, not runtime equivalence;
- compiler/linker/runtime frontiers should drive the work.

## Neutral core foundation

The current portable layer contains:

- `IdrCoreTypes`
- `IdrCoreServices`
- `IdrImageContext`
- `IdrPeLoader`
- `IdrAnalysis`
- `IdrAnalysisState`
- `IdrInstructionNav`
- `IdrDecompilerModel`
- transitional `IdrLegacyCompat`
- `IdrLegacyBridge`

The legacy analysis engine is linked from generated versions of the real translation units rather than reimplemented wholesale.

## Whole-legacy integration history

### #23-31: decoder and Decompiler representation

#23 proved real generated `Disasm.cpp` executes against the shipped x86 `dis.dll` on a hosted MSVC x86 runner.

#24 established segment-aware address/offset translation, including the legacy `0x80000` unbacked marker.

By #31 the complete real `Decompiler.cpp` compiled as one generated MSVC x86 TU, excluding only three GUI/presentation-owned methods.

### #32-48: linker-driven subsystem integration

The first whole-Decompiler link attempt exposed 103 unresolved externals. Rather than adding arbitrary stubs, the linker frontier was used as the dependency map.

The progression was:

`103 -> 89 -> 73 -> 46 -> 37 -> 29 -> 4`

During that phase:

- complete real `Misc.cpp` was integrated;
- complete real `KnowledgeBase.cpp` was integrated;
- complete real `Infos.cpp` was integrated;
- analysis-facing session/global ownership was extracted from legacy `Main.cpp` into `IdrLegacyBridge`;
- duplicate generated string helpers were localized rather than generalized.

### #49-50: portable services and zero unresolved

The last four seams were connected to real portable/legacy behavior:

- `ManualInput` -> service callback
- method lookup -> service callback
- enumeration formatting -> real legacy helper
- procedure-size estimation -> metadata/flag/image state

Embedded-procedure confirmation was also routed through the service boundary.

#50 completed successfully with zero unresolved externals and a runnable integrated probe.

## PE32 ingestion

### Legacy loader semantics retained

`IdrPeLoader` follows the analysis-facing behavior extracted from `Main.cpp`, not a generic memory-mapped PE abstraction.

Important retained behavior:

- PE32 / x86 validation;
- absolute entry point from `ImageBase + AddressOfEntryPoint`;
- section starts from `ImageBase + VirtualAddress`;
- next-section RVA defines analysis span except for the last section;
- last section uses `VirtualSize`;
- zero-raw-data sections are unbacked;
- resource and relocation sections are represented as unbacked;
- unbacked segments carry the legacy `0x80000` flag;
- backed analysis spans are packed contiguously and zero-filled where raw data is shorter than the analysis span;
- `CodeBase` is based on the first section;
- `CodeSize` equals packed backed-analysis size.

### #51-53: loader compile/runtime

#51 and #52 both exposed the Windows SDK `max` macro collision in `IdrPeLoader.cpp`. The workflow had already been hardened so every `cl` invocation fails the step immediately.

#53 fixed the macro collision and completed successfully. The loader probe then verified packed/unbacked section behavior and address/offset translation at runtime.

## Authoritative loaded session

### #54

A loaded PE now drives both neutral and legacy analysis-facing state through one activation API.

`ActivateLegacyLoadedPeSession()` binds:

- neutral `ImageContext`
- `EP`
- `ImageBase`
- `ImageSize`
- `TotalSize`
- `CodeBase`
- `CodeSize`
- legacy `Code` pointer
- neutral `AnalysisState` and legacy `Flags` view
- legacy `Infos` pointer storage

`ResetLegacyLoadedPeSession()` tears that state down again.

The session probe verifies activation, shared byte storage, flags visibility, infos initialization and reset behavior.

## First portable CLI

### #57 on the integration branch

Commit:

`a0d72d9916b9c458ba3098f7a7d17354d536fba0` — `Introduce first portable IDR CLI`

Run #57 completed successfully.

The host accepts:

`idr-cli.exe <target.exe>`

It:

1. loads a PE32 target through `IdrPeLoader`;
2. activates the authoritative legacy/neutral session;
3. verifies both views agree on the loaded image;
4. prints deterministic PE/session/segment metadata;
5. resets the session.

CI builds a real MSVC x86 PE32 executable fixture and runs the CLI against that file. This is deliberately different from only testing a synthetic in-memory PE byte array.

## Clean branch transition

After #57 the active work moved away from the long-lived integration branch.

`agent/portable-cli` was created from `main` with:

- parent: `52788d54bedab3f088356e3284bf3105ad57b30b`
- baseline commit: `7f3f3fe68db55c95a7928bbc86f3bf2580519113` — `Establish portable MSVC x86 core baseline`

The baseline tree is the verified #54 portable core/session tree including these documentation files.

The verified #57 CLI change is then carried as the next logical product commit, while the workflow is retargeted to `agent/portable-cli`.

This keeps the active product history compact while preserving the full #8-#57 evidence trail on the frozen integration branch.

## Current compatibility surface

`portable/core/IdrLegacyCompat.h` and the generator transforms remain transitional bridges, not a complete Borland RTL implementation.

Reached compatibility includes:

- `String = std::string`
- explicit reached numeric `String(expr)` conversions
- 1-based helper behavior for reached `Pos` / `SubString` calls
- generated `.Length()` / `.SetLength()` / `.IsEmpty()` transforms
- reached string/formatting helpers
- narrow `Currency`, `Variant` and enumeration paths
- `TList` / `TStringList` compatibility used by metadata code
- critical-section compatibility used by `Infos.cpp`
- configurable headless service callbacks.

Do not generalize this surface without runtime evidence.

## Important semantic hazards still open

### Direct String indexing

Legacy direct expressions such as `name[1]` can still differ because Borland `String` is 1-based while `std::string` is 0-based. Audit only reached runtime paths rather than applying blind source-wide transforms.

### Variant / WideString / Currency / Comp

Only reached cases are represented. Full binary-layout and formatting compatibility are not claimed.

### AnalysisState range fidelity

The exact legacy end-boundary asymmetry between `SetFlags` and `ClearFlags` remains an explicit fidelity question.

### Legacy warnings

Signedness, old CRT calls, suspicious shifts and not-all-paths-return warnings remain visible. Classify by runtime relevance instead of globally modernizing them.

### Metadata/session lifetime

The loaded session now owns flag and infos slot storage, but deeper analysis paths will begin allocating real `InfoRec`/procedure/type metadata. Lifetime and reset behavior must remain authoritative as those paths are enabled.

## Next technical frontier

The project no longer needs more linker-stub chasing. The next work should make the CLI drive progressively deeper real analysis.

Preferred order:

1. prove decoder initialization from the CLI-loaded session;
2. decode the target entry point through the real `MDisasm`/`dis.dll` chain;
3. locate the first safe non-GUI legacy analysis initialization path and invoke it from the host;
4. inspect resulting `Flags`/`Infos` state;
5. add controlled Delphi Win32 fixtures and compare outputs with original IDR;
6. audit runtime-reached Borland RTL semantics;
7. expose procedures/types/metadata through deterministic CLI output;
8. publish `idr-cli.exe` as a workflow artifact once it is useful enough to consume outside CI.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent commit per logical change where practical.
- Do not disturb frozen reference branches.
- Do not push over an active `agent/portable-cli` run.
- Preserve original legacy source unless a deliberate structural port warrants changing it.
- No upstream PR or merge to `main` yet.
