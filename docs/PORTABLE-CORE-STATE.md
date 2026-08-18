# Portable Core Project State

Last updated: 2026-08-18

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active product branch: `agent/portable-cli`

Frozen porting/integration reference: `agent/portable-core-integration`

Frozen compiler-smoke reference: `agent/portable-core-smoke`

`main` remains untouched at the modern Embarcadero baseline.

Current active code milestone before this documentation-only update:

`428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`

The corresponding #78 integration run was reported successful. `docs/**` is excluded from push-triggered CI, so documentation-only commits do not start another integration run.

## CI

Workflow: `.github/workflows/portable-core-integration.yml`

The active branch uses:

- GitHub-hosted Windows runner / MSVC x86 via `vswhere.exe` + `vcvars32.bat`;
- `actions/checkout@v6`;
- fail-fast compile commands;
- concurrency cancellation;
- real generated legacy translation units;
- a deterministic x86 PE32 fixture;
- a neutral control-flow probe;
- an end-to-end `idr-cli.exe` execution against the real fixture.

x86 remains intentional because the real legacy decoder contains x86-specific code and the shipped `dis.dll` is x86.

## Clean active history

`agent/portable-cli` was rebuilt cleanly from `main` rather than carrying the full experimental integration history.

Important active commits:

- `7f3f3fe68db55c95a7928bbc86f3bf2580519113` — `Establish portable MSVC x86 core baseline`
- `aaf47e2382a3b6419a74230931a052f6481aa6d0` — `Introduce portable CLI host`
- `fcfc08db2b588036afc16a2e882b80a0d186dcb0` — `Decode loaded entry point in portable CLI`
- `c6093badd1a1d0f5a441b8cb64b2c2776c23e724` — `Avoid Windows max macro in CLI trace`
- `ee83293a665d6eeaca11b2a6315de163713c01af` — `Clamp file-aligned section data to analysis span`
- `182277558d0a0f9837d965b3833bb9769741c535` — `Trace direct call procedure candidate`
- `8238a97c54941107ef125bd4b588601eb7893e25` — `Queue bounded procedure candidates`
- `df4a270933e7e0de6d77af69b1c8e3856fd6a6d4` — `Extract bounded control flow from CLI`
- `19a63ffed2770125a7af6f2e88b26e552bfb2189` — `Prove neutral control flow independently`
- `2e6200adf640d3f6d6561703bca905ba811d0c83` — `Trace bounded basic blocks in control flow`
- `4e62a4139f5e1de94d8864150413008ddb350405` — `Mark analyzed procedure starts`
- `c71b2f12ae56d0a1ed3ef2402a2b26ccc4b55412` — `Model explicit control flow edge kinds`
- `78a373b99da00a305ea3b823eb877c1e3af3b9c9` — `Track procedure ownership on control flow edges`
- `d0d08fc108716597b1032a889c99fef6b61690a6` — `Summarize analyzed procedures`
- `3903097f2d6d5a279164a7366300299c3cfad0af` — `Track procedure call xrefs`
- `47c48d12f03a1dfa32a05c7cac6aff4ad84bddcf` — `Count incoming procedure calls`
- `ebbde9b286526725b6cfd645a6335906c78479f0` — `Inject address mapping into control flow`
- `428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`

The frozen `agent/portable-core-integration` branch remains the archaeology/evidence trail for the earlier #8-#57 linker and compatibility work.

## Verified architecture

The project now has runtime evidence for the following path:

`PE32 target -> IdrPeLoader -> authoritative loaded session -> real MDisasm/dis.dll decode -> bounded neutral control-flow engine -> shared AnalysisState -> deterministic CLI output`

The portable layer includes:

- neutral core types/services;
- segment-aware PE image context and loader;
- `AnalysisState` with legacy flags view;
- instruction navigation;
- legacy session bridge;
- generated real `Disasm`, `KnowledgeBase`, `Infos`, `Misc` and almost-complete `Decompiler` translation units;
- real `idr-cli.exe` host;
- neutral `IdrControlFlow` analysis model.

## PE/session behavior

The neutral PE32 loader retains the analysis-facing semantics extracted from legacy IDR, including section span handling, packed backed bytes, unbacked sections and the legacy `0x80000` marker.

A loaded image is bound to both neutral and legacy-facing state through the session bridge. The CLI verifies the loaded image and legacy session agree before running analysis.

A file-alignment bug found by the deterministic fixture was fixed by clamping copied raw section data to the analysis span rather than assuming `SizeOfRawData` fits the virtual/analysis range.

## Real decoder and CLI analysis

The CLI initializes the real legacy `MDisasm` path and decodes the actual PE32 entry point. Decoder results are adapted into a neutral `DecodedInstruction` structure rather than allowing the control-flow engine to depend directly on `Disasm` globals or legacy `DISINFO`.

The CLI currently produces deterministic trace/graph statistics in addition to PE/session metadata.

## Neutral control-flow engine

`IdrControlFlow` has moved progressively out of `wmain` into a neutral bounded engine.

It currently supports:

- bounded procedure-candidate worklist;
- deduplicated direct-call procedure discovery;
- bounded basic-block worklist per procedure;
- direct calls;
- direct conditional and unconditional branches;
- conditional branch taken + fall-through edges;
- `ret` block termination;
- instruction de-duplication within a procedure;
- shared `AnalysisState` flag propagation;
- explicit `ProcStart` only for procedures that were actually analyzed;
- explicit edge kinds: `Call`, `BranchTaken`, `FallThrough`;
- procedure ownership on every edge;
- procedure summaries;
- call xrefs containing caller, call-site and callee;
- incoming-call counts per procedure.

The engine deliberately does not mark branch-only basic-block targets as `ProcStart`.

`ProcEnd` is not currently inferred. Legacy `AnalyzeProc1` has the relevant `cfProcEnd` writes commented out on common return/exit paths, so the portable engine does not invent stronger semantics than the source supports.

## Current deterministic graph fixture

The neutral probe models three procedures and a conditional branch. Its established contract includes:

- entry procedure with three basic blocks;
- two discovered call-target procedures;
- five explicit edges total;
- three call edges;
- one branch-taken edge;
- one fall-through edge;
- entry owns three edges;
- TargetA owns two edges;
- TargetB owns zero edges;
- three call xrefs: Entry -> A once and A -> B twice;
- incoming call counts: Entry `0`, A `1`, B `2`.

The real MSVC x86 PE32 fixture independently exercises the same control-flow engine through `MDisasm` and requires the expected entry-block and edge counts.

## Image-context decoupling

The most recent architecture milestone removes `IdrControlFlow`'s direct dependency on global `AddressToOffset()`.

`AnalyzeBoundedControlFlow` now receives an injected `AddressMapper`.

- The CLI adapter supplies the existing segment-aware `IdrImageContext::AddressToOffset` semantics.
- The neutral probe supplies its own local mapper over synthetic bytes.
- `IdrControlFlow.h` no longer includes `IdrImageContext.h`.

#78 strengthens this from a source-level claim to a linker-level claim: the neutral control-flow probe links only against `IdrAnalysisState.obj` plus its own object. `IdrImageContext.obj`, PE loading, legacy bridge and the disassembler are not available to satisfy hidden dependencies.

## Known semantic risks

Successful compile/link/runtime probes do not establish full behavioral equivalence with Borland/VCL IDR.

Important open risks include:

- Borland 1-based direct `String[index]` semantics versus `std::string` 0-based indexing;
- incomplete `WideString`, `Variant`, `Currency`, `Comp` and formatting fidelity;
- exact legacy `SetFlags` / `ClearFlags` end-boundary behavior;
- transitional register/string/container ABI compatibility;
- metadata ownership/lifetime as deeper legacy analysis creates real `InfoRec` structures;
- indirect calls/jumps and switch/table control flow;
- procedure size/end semantics;
- imports/exports/resources and Delphi-specific discovery;
- deeper decompiler passes are not yet driven by the CLI.

## Immediate next phase

The next work should build on the now-neutral CFG rather than returning to linker-stub chasing.

Good next chunks are:

1. add richer procedure/xref query helpers without reintroducing global session dependencies;
2. extend deterministic fixtures for unconditional branches, loops/back-edges and out-of-image targets;
3. decide and prove procedure extent/size semantics from legacy behavior rather than guessing `ProcEnd`;
4. begin feeding discovered procedures into the next safe legacy analysis/decompiler stage;
5. audit runtime-reached Borland string/RTL semantics as those paths become active;
6. add controlled Delphi Win32 fixtures and compare with original IDR;
7. later publish `idr-cli.exe` together with required runtime `dis.dll` as a GitHub Actions artifact.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- Keep architecture-scale changes coherent.
- Do not disturb frozen reference branches.
- Do not push over an active `agent/portable-cli` workflow run.
- Preserve original legacy source unless a deliberate structural port warrants changing it.
- No upstream PR or merge to `main` yet.
- Keep these docs current at meaningful milestones.
- Green runs need metadata only; do not repeatedly fetch successful logs.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> real PE32 analysis -> headless idr-cli.exe -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling, or a self-hosted runner.
