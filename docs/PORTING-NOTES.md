# IDR Portable Core - Technical Notes

Last updated: 2026-08-18

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen porting/integration reference: `agent/portable-core-integration`
- Frozen smoke/reference branch: `agent/portable-core-smoke`

`agent/portable-cli` is intentionally a clean branch from `main`. Its first commit after `main` is a squash baseline containing the verified portable core/session state. The long experimental history remains available on the frozen integration branch.

Current code milestone before this documentation-only update:

`428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`

The #78 run for that milestone was reported green.

## Foundation retained from the integration phase

The early work established that the original IDR analysis code can be carried into a free hosted MSVC x86 build without reimplementing the whole engine.

Key retained evidence:

- real generated `Disasm.cpp` executes against the shipped x86 `dis.dll`;
- complete generated `KnowledgeBase.cpp`, `Infos.cpp` and `Misc.cpp` compile/link;
- the complete real `Decompiler.cpp` compiles apart from three GUI-owned presentation/orchestration methods;
- linker-driven integration reduced unresolved externals from 103 to zero;
- neutral core services replaced only explicit headless seams instead of broad fake-VCL stubbing;
- a neutral PE32 loader and authoritative legacy/neutral loaded session were established.

The old integration branch is retained for the full #8-#57 evidence trail.

## Clean CLI branch transition

The active branch starts directly from `main`:

- `52788d54bedab3f088356e3284bf3105ad57b30b` — `main` baseline
- `7f3f3fe68db55c95a7928bbc86f3bf2580519113` — `Establish portable MSVC x86 core baseline`
- `aaf47e2382a3b6419a74230931a052f6481aa6d0` — `Introduce portable CLI host`

The active history then moved from host/loader proof into real decoding and a neutral control-flow architecture.

## PE32 loader/session results

`IdrPeLoader` follows the analysis-facing behavior extracted from legacy `Main.cpp`, not a generic Windows loader model.

Retained semantics include:

- PE32/x86 validation;
- absolute entry point from `ImageBase + AddressOfEntryPoint`;
- segment starts from image base + RVA;
- next-section RVA span behavior;
- last-section `VirtualSize` handling;
- zero-raw/resource/relocation sections represented as unbacked;
- legacy `0x80000` unbacked marker;
- packed backed-analysis bytes;
- `CodeBase` from the first section;
- packed analysis size as `CodeSize`.

The deterministic PE fixture exposed a real loader bug: raw section data can be file-aligned beyond the analysis/virtual span. Commit `ee83293a665d6eeaca11b2a6315de163713c01af` clamps copied raw bytes to the analysis span, preserving safe legacy intent.

The authoritative loaded session binds the same underlying image/state to both neutral and legacy-facing code, including entry point, image/code dimensions, code bytes, `AnalysisState`/`Flags` and `Infos` slot storage.

## Real decoder in the CLI

The CLI now initializes the actual `MDisasm`/`dis.dll` chain and decodes the loaded PE32 entry point.

`DISINFO` is adapted into a neutral `DecodedInstruction` containing only the analysis information needed by the new control-flow engine:

- instruction length;
- mnemonic/disassembly text;
- call flag;
- branch flag;
- conditional flag;
- return flag;
- direct target address.

This keeps legacy decoder mechanics at the host boundary instead of leaking them into neutral CFG code.

## Control-flow extraction history

### Direct call candidate

`182277558d0a0f9837d965b3833bb9769741c535` — `Trace direct call procedure candidate`

The first direct call target was followed as a procedure candidate with shared analysis flags.

### Bounded procedure queue

`8238a97c54941107ef125bd4b588601eb7893e25` — `Queue bounded procedure candidates`

Nested direct-call discovery became a bounded deduplicated queue. The fixture contains Entry -> TargetA -> TargetB, with TargetA calling TargetB twice. Discovery is deduplicated even though xrefs later remain call-site specific.

### Neutral engine extraction

`df4a270933e7e0de6d77af69b1c8e3856fd6a6d4` — `Extract bounded control flow from CLI`

Traversal moved out of `wmain` into `IdrControlFlow.h` using an injected decoder callback.

`19a63ffed2770125a7af6f2e88b26e552bfb2189` — `Prove neutral control flow independently`

A standalone synthetic probe proved bounded traversal without linking PE loader, legacy bridge, generated legacy translation units or `Disasm`.

## Basic-block CFG

`2e6200adf640d3f6d6561703bca905ba811d0c83` — `Trace bounded basic blocks in control flow`

Each procedure now has a bounded local basic-block worklist.

Current behavior:

- a call records a call edge and queues the callee as a procedure candidate while execution continues in the caller block;
- a conditional branch records the taken target, queues both target and fall-through blocks, and terminates the current block;
- an unconditional direct branch queues only its target and terminates the current block;
- `ret` terminates the current block;
- seen block starts and decoded instruction addresses prevent duplicate traversal;
- all targets remain constrained by the supplied address mapper and configured limits.

The established entry fixture produces three basic blocks.

## Procedure identity

`4e62a4139f5e1de94d8864150413008ddb350405` — `Mark analyzed procedure starts`

`CodeFlags::ProcStart` is set only after a procedure has actually produced a successful trace.

Therefore:

- entry gets `ProcStart`;
- analyzed direct-call candidates get `ProcStart`;
- ordinary branch/basic-block targets get `Loc`/code state but explicitly do not get `ProcStart`.

This mirrors the useful part of legacy `AnalyzeProc1` more closely than blindly promoting every control-flow target.

`ProcEnd` is intentionally not synthesized. The corresponding legacy writes are commented out on common return/exit paths, and procedure size is handled separately in the original code.

## Explicit CFG edges

`c71b2f12ae56d0a1ed3ef2402a2b26ccc4b55412` — `Model explicit control flow edge kinds`

The old boolean call/non-call distinction was replaced with:

- `Call`
- `BranchTaken`
- `FallThrough`

A conditional branch therefore creates two explicit graph edges rather than only creating a hidden fall-through block.

The deterministic graph contract is currently five edges total: three calls, one branch-taken and one fall-through.

## Procedure ownership

`78a373b99da00a305ea3b823eb877c1e3af3b9c9` — `Track procedure ownership on control flow edges`

Every `ControlFlowEdge` carries its owning procedure address in addition to from/to/kind.

The neutral fixture proves:

- Entry owns three edges;
- TargetA owns two edges;
- TargetB owns zero outgoing edges.

This avoids reconstructing ownership later when producing xrefs, summaries or GUI/decompiler views.

## Procedure summaries

`d0d08fc108716597b1032a889c99fef6b61690a6` — `Summarize analyzed procedures`

`ControlFlowResult` now exposes a deterministic procedure summary list, entry first followed by analyzed candidates.

Each summary currently contains:

- procedure address;
- basic-block count;
- instruction count;
- outgoing call-edge count;
- branch-taken edge count;
- fall-through edge count;
- incoming direct-call count.

## Call xrefs

`3903097f2d6d5a279164a7366300299c3cfad0af` — `Track procedure call xrefs`

A direct call xref stores:

`{ caller, callSite, callee }`

Call-sites are deliberately not deduplicated. In the synthetic fixture:

- Entry -> A occurs once;
- A -> B occurs twice at two different call-sites;
- total call xrefs = 3.

`47c48d12f03a1dfa32a05c7cac6aff4ad84bddcf` — `Count incoming procedure calls`

Procedure summaries derive incoming call counts from those xrefs:

- Entry = 0;
- A = 1;
- B = 2.

## Decoupling CFG from global image state

Until #76, `IdrControlFlow` was neutral with respect to decoder/legacy code but still called the global `AddressToOffset()` from `IdrImageContext`.

`ebbde9b286526725b6cfd645a6335906c78479f0` — `Inject address mapping into control flow`

The engine now receives:

`AddressMapper = std::function<int(DWord)>`

and all address validity/offset queries go through that injected mapper.

Consequences:

- `IdrControlFlow.h` no longer includes `IdrImageContext.h`;
- the CLI adapter still supplies the existing segment-aware legacy/image-context mapping;
- the neutral probe uses a local base+size mapping and no longer initializes global image segments.

`428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`

The neutral probe's linker command removes `IdrImageContext.obj` entirely. It now links only:

`IdrAnalysisState.obj + idr_control_flow_probe.obj`

This is the current strongest evidence that the bounded CFG engine has no hidden dependency on the global PE/image session, loader, legacy bridge or disassembler.

## Current deterministic fixture contract

The synthetic control-flow probe currently establishes:

- Entry procedure at `kBase`;
- conditional branch producing taken + fall-through blocks;
- TargetA direct-call procedure;
- TargetB nested direct-call procedure;
- three entry blocks;
- two candidate procedures;
- five total edges;
- three call edges;
- one branch-taken edge;
- one fall-through edge;
- procedure ownership 3 / 2 / 0;
- three procedure summaries;
- three call xrefs;
- incoming call counts 0 / 1 / 2;
- correct `Instruction`, `Code`, `Call`, `Loc` and `ProcStart` flag propagation;
- branch targets are not promoted to procedures.

The real MSVC x86 PE32 fixture independently exercises the same engine through the actual decoder path and asserts its expected entry-block/edge/candidate counts.

## Compatibility surface and risks

`IdrLegacyCompat` and generator transforms remain transitional bridges, not a complete Borland RTL implementation.

Important open hazards:

### Direct String indexing

Borland `String` is 1-based while `std::string` is 0-based. Audit runtime-reached direct indexing instead of applying blind global transforms.

### Variant / WideString / Currency / Comp

Only reached cases are represented. Full layout and formatting compatibility is not claimed.

### AnalysisState range fidelity

Exact legacy end-boundary behavior of `SetFlags` and `ClearFlags` remains a fidelity question.

### Procedure extent

`ProcStart` is now grounded. `ProcEnd`/procedure size still needs semantics derived from original IDR rather than inferred from a single `ret`, because procedures may contain multiple return blocks and legacy IDR does not simply mark every return as `ProcEnd`.

### Indirect control flow

The current neutral CFG is intentionally direct-target focused. Indirect calls/jumps, jump tables/switches and richer x86 control-flow semantics remain future work.

### Metadata lifetime

Deeper legacy analysis will create real `InfoRec`, type and procedure metadata. Ownership/reset rules must remain authoritative as those paths are enabled.

## Next technical frontier

The project no longer needs linker-stub chasing or proof that a CLI can decode a real PE. The next phase should deepen analysis from the now-clean neutral CFG boundary.

Preferred order:

1. add targeted procedure/xref query helpers where useful;
2. extend fixtures for unconditional jumps, loops/back-edges, cross-block joins and invalid/out-of-image targets;
3. derive procedure size/extent behavior from legacy code and evidence;
4. invoke the next safe legacy analysis/decompiler stage using discovered procedures;
5. inspect resulting `Flags`/`Infos`/type state;
6. audit runtime-reached Borland RTL/String semantics;
7. add controlled Delphi Win32 fixtures and compare results with original IDR;
8. expose richer deterministic CLI procedure/xref/type output;
9. package `idr-cli.exe` together with required `dis.dll` as a GitHub Actions artifact when it is useful to consume outside CI.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent commit per logical change where practical.
- Do not disturb frozen reference branches.
- Do not push over an active `agent/portable-cli` workflow run.
- Successful runs need metadata only; do not repeatedly fetch green logs.
- Preserve original legacy source unless a deliberate structural port warrants changing it.
- No upstream PR or merge to `main` yet.
- Keep these notes synchronized with meaningful architecture milestones.
