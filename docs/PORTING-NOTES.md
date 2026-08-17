# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical journal for the MSVC/GitHub-hosted portable-core work.

## Repository / branches

- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Active integration branch: `agent/portable-core-integration`
- Frozen smoke/reference branch: `agent/portable-core-smoke`

The integration branch was reconstructed directly from clean `main`; the smoke branch remains a frozen regression/reference laboratory.

## Smoke lessons carried forward

- avoid giant Borland `String` wrappers without semantic evidence;
- preserve `__fastcall` ABI on x86;
- Borland 1-based `String` semantics are a real runtime hazard;
- GUI/policy seams should be abstracted, not satisfied by fake VCL;
- compile representation is useful evidence but is not runtime equivalence.

## Neutral core foundation

The integration branch now contains neutral layers for core types, services, image/segment mapping, analysis helpers, analysis state, instruction navigation and the first decompiler model primitives.

The current `AnalysisState` still has a deliberate open fidelity question: neutral range operations allow an exact-end range symmetrically, while original `SetFlags` and `ClearFlags` differ at that boundary.

## Real decoder runtime

Run #23 proved generated real `Disasm.cpp` links and executes against the shipped x86 `dis.dll` on a hosted MSVC x86 runner.

The portable generated transform also resolves the decoder function pointers before invoking `PdisNew(1)`. Original `Disasm.cpp` remains unchanged.

Run #24 added segment-aware image translation compatible with legacy `SegmentList` semantics, including unbacked `0x80000` segments.

## Whole-Decompiler transition

After #26 the integration strategy changed from extracted method slices to complete generated legacy translation units.

`tests/prepare_portable_decompiler_full.ps1` builds one portable TU from real `Decompiler.cpp`, removing only the GUI-owned definitions:

- `TDecompileEnv::OutputSourceCodeLine()`
- `TDecompileEnv::OutputSourceCode()`
- `TDecompileEnv::DecompileProc()`

Run #31 was the first compile-clean complete Decompiler TU.

## Linker-driven subsystem integration

### #32: first full Decompiler link attempt

Adding `Decompiler.portable.obj` to the integration probe exposed **103 unresolved externals**. This became the dependency map rather than a reason to add arbitrary stubs.

### #33-40: bridges plus complete real Misc

Small RTL/session bridges reduced the linker frontier while `tests/prepare_portable_misc_full.ps1` was pushed through its compatibility walls.

By #40:

- complete real `Misc.cpp` compiled under MSVC x86;
- complete real `Decompiler.cpp` compiled in the same build;
- the linker frontier was down to 46 unresolved externals plus two duplicate generated string helpers.

The duplicate `PortableStringPos` / `PortableSubString` definitions were localized to their generated TU rather than creating a broader runtime abstraction.

### #42-44: complete real KnowledgeBase

`tests/prepare_portable_knowledgebase_full.ps1` integrates real `KnowledgeBase.cpp`.

Its first complete compile attempt reached only two missing declarations (`MatchCode`, `SameText`). A later attempt exposed `GetTypeIdxByUID`, an empty legacy function explicitly marked unused; the generated portable TU drops that dead definition instead of inventing a return value.

KnowledgeBase then compiled as part of the integrated build and materially reduced the linker dependency set.

### #46-47: complete real Infos

`tests/prepare_portable_infos_full.ps1` integrates real `Infos.cpp`.

The first pass exposed a narrow declaration/compatibility wall consisting largely of real `Misc` helpers plus one remaining GUI workdir reference and one numeric Borland `String(...)` construction. After those were mapped, #47 compiled all four large legacy units together:

`KnowledgeBase + Infos + Misc + Decompiler`

The linker then reported only 29 unresolved externals, mostly legacy session/global state.

### #48: session/state ownership

`IdrLegacyBridge.cpp` now supplies the real analysis-facing subset formerly owned by `Main.cpp`, including:

- `KnowledgeBase`
- `Infos`
- `Flags`
- image/code size globals
- segment/type/VMT lists
- VMT layout globals
- string buffer state
- mutable legacy register-name tables
- class address lookup/cache
- working directory

`Flags` is a view over the neutral `AnalysisState` instead of an independent fake allocation.

This reduced the unresolved set from 29 to four explicit headless seams.

### #49-50: portable services and linked execution

The four remaining seams were connected to real portable or legacy behavior:

- `ManualInput` -> `IdrCoreServices.manualInput`
- `PortableGetMethodInfo` -> `IdrCoreServices.lookupMethod`
- `PortableGetEnumerationString` -> real legacy `GetEnumerationString`
- `PortableEstimateProcSize` -> metadata first, then neutral flag/image state

Embedded-procedure confirmation was also routed through the same service boundary, and `IdrLegacyBridge` gained a configurable service set with conservative headless defaults.

#49 contained one local bridge defect: `AddressToOffset()` returns an `int` with negative failure codes, but the new code treated it as an optional value. Because the workflow's multi-command `cmd` compile step did not fail immediately, the missing `IdrLegacyBridge.obj` appeared later as `LNK1181`.

#50 corrected the offset contract.

Run **#50** at commit:

`242b4a841baeb93abe3eeb0802df262483c10c43` — `Fix portable proc-size offset handling`

completed successfully, including the linked probe execution.

Current verified chain:

`portable core + generated Disasm + KnowledgeBase + Infos + Misc + Decompiler -> MSVC x86 linker -> executable -> runtime probe`

There are now **zero unresolved externals in the integrated core probe**.

## Transitional compatibility surface

`portable/core/IdrLegacyCompat.h` and generated transforms remain transitional bridges, not a claim of a complete Borland RTL implementation.

Current reached compatibility includes:

- `String = std::string`
- explicit known numeric `String(expr)` conversions
- 1-based helper implementations for reached `Pos` / `SubString` calls
- `.Length()` / `.SetLength()` / `.IsEmpty()` generated transforms
- `SameText`, `AnsiReplaceText`, numeric/string formatting helpers
- narrow `Currency`, `Variant` and enumeration paths
- `TList` / `TStringList` compatibility needed by real metadata code
- critical-section compatibility needed by real `Infos.cpp`
- configurable headless service callbacks.

Do not generalize this surface without runtime evidence.

## Important semantic hazards still open

### Borland direct String indexing

Direct legacy expressions such as `name[1]` still have Borland 1-based versus `std::string` 0-based semantics. These must be audited as actual runtime paths are exercised.

### Variant / WideString / Currency / Comp

Only reached cases are represented. Binary layout and full conversion/formatting compatibility are not claimed.

### Legacy warnings

Signedness warnings, old CRT calls, suspicious shift-count expressions and not-all-paths-return warnings remain visible. They should be classified by runtime relevance, not globally modernized.

### Workflow fail-fast weakness

#49 demonstrated that the multi-command `cmd` compile step can continue after one `cl` failure and report the failure indirectly at link time. The workflow should be hardened so every compile command fails the step immediately.

## Next architectural frontier: executable ingestion

The linker dependency chase is finished for the current probe. The next useful work is to provide the core with a real target image.

Preferred sequence:

1. isolate PE32 loading semantics from legacy VCL `Main.cpp`;
2. introduce a neutral loader that owns file bytes and PE section metadata;
3. populate `ImageView` / `ImageSegments` and legacy image/session views from one authoritative loaded-image object;
4. test RVA/address/offset mapping against controlled PE32 input;
5. harden CI compile steps to fail immediately while touching the workflow for the next integration target;
6. add a minimal headless executable host that opens a file and reports deterministic PE/core metadata;
7. grow that host into `idr-cli.exe <target.exe>` and then exercise deeper real analysis paths.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent commit per logical change where practical.
- Do not disturb the frozen smoke branch.
- Do not push over an active integration run because concurrency cancels the previous run.
- Preserve original legacy files until a deliberate structural port warrants changing them.
- No upstream PR or merge to `main` yet.
