# IDR Portable Core - Technical Notes

Last updated: 2026-08-18

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen porting/integration reference: `agent/portable-core-integration`
- Frozen smoke/reference branch: `agent/portable-core-smoke`

`agent/portable-cli` was rebuilt cleanly from `main`; the long experimental linker/compiler archaeology remains on the frozen integration branch.

Current verified code milestone:

`f321469aed6ebb641c4ae7438a5776f382f62ad5` — `Extract legacy decompiler preflight adapter`

Normal integration #102 and the first dedicated legacy-decompiler preflight workflow were both reported green.

## Foundation retained from the integration phase

The early work established that original IDR analysis/decompiler code can be carried into a free hosted MSVC x86 build without reimplementing the engine.

Retained evidence:

- real generated `Disasm.cpp` executes against shipped x86 `dis.dll`;
- complete generated `KnowledgeBase.cpp`, `Infos.cpp` and `Misc.cpp` compile/link;
- essentially complete real `Decompiler.cpp` compiles after removing three GUI-owned presentation/orchestration definitions from the generated copy;
- linker-driven integration reduced unresolved externals to zero;
- neutral services replace explicit GUI/headless seams rather than broad VCL stubbing;
- neutral PE32 loading and an authoritative shared legacy/neutral image session are established.

## PE32 and decoder path

`IdrPeLoader` models IDR's analysis-facing PE behavior rather than a generic Windows loader.

Important semantics retained:

- PE32/x86 validation;
- absolute entry point from `ImageBase + AddressOfEntryPoint`;
- section/segment mapping;
- unbacked resource/reloc/zero-raw spans;
- packed backed analysis bytes;
- safe clamping of file-aligned raw data to the analysis span;
- shared code bytes, `Flags`/`AnalysisState` and `Infos[]` storage in the active session.

The CLI initializes the real `MDisasm`/`dis.dll` chain and adapts `DISINFO` into neutral decoded-instruction facts.

## Neutral bounded control flow

The current CFG engine is injected with decoder and address-mapper callbacks and remains independent of the PE loader/global image context.

The standalone control-flow probe links only:

`IdrAnalysisState.obj + idr_control_flow_probe.obj`

Current CFG behavior:

- bounded candidate queue;
- bounded block queue;
- direct call discovery;
- direct conditional/unconditional branches;
- explicit `Call`, `BranchTaken`, `FallThrough` edges;
- procedure ownership on edges;
- call xrefs `{ caller, callSite, callee }`;
- procedure summaries;
- incoming/outgoing edge/xref query helpers;
- successful analyzed procedures get `ProcStart`;
- ordinary branch targets get location/code flags but not `ProcStart`;
- invalid targets create no edge/xref/candidate;
- unconditional jumps, joins and loop back-edges are covered by the richer fixture.

### Observed extent

Procedure summaries now contain:

- `observedStart`
- `observedEndExclusive`
- `observedSpan`

These describe the enclosing numeric range of positively-sized decoded instructions owned by the procedure. Holes are included in the numeric span.

They are observational only and are not mapped to legacy `procInfo->procSize`.

## Procedure-analysis input

`ProcedureAnalysisInput` packages the neutral per-procedure analysis surface:

- summary;
- instructions;
- blocks;
- owned edges;
- incoming calls;
- outgoing calls.

The builder is runtime-tested for entry and candidate procedures and rejects missing procedure addresses.

## Neutral prototype metadata

The neutral prototype model includes:

- procedure kind;
- return type;
- flags;
- `bpBase`;
- `retBytes`;
- `stackSize`;
- argument tag/register/index/size/name/type;
- local offset/size/name/type.

Legacy seed types mirror this value surface without exposing `InfoRec`.

Actual legacy procedure kinds confirmed from IDR:

- `ikRefine = 0x25`
- `ikConstructor = 0x26`
- `ikDestructor = 0x27`
- `ikProc = 0x28`
- `ikFunc = 0x29`

The older synthetic probe values `1` and `2` are test-only kinds and must not be confused with real legacy constants.

## Prototype completeness

Legacy `TDecompiler::CheckPrototype()` establishes the key rule:

- every argument requires a non-empty type;
- `ikFunc` additionally requires a non-empty return type;
- procedure-style kinds do not require a return type.

The neutral `IsProcedurePrototypeComplete()` and seed-building path preserve this behavior.

## Neutral -> legacy metadata adaptation

`ApplyLegacyProcedureMetadataSeed()` writes neutral seed metadata into a real detached procedure-style `InfoRec`/`InfoProcInfo`.

Runtime evidence proves:

- kind/type/flags/bpBase/retBytes/stackSize map correctly;
- register arguments remain register arguments even though legacy `AddArg()` defaults them to non-register;
- stack args map correctly;
- locals map correctly;
- an already-populated args/locals surface is rejected;
- existing `procInfo->procSize` remains untouched.

## Legacy -> neutral metadata capture

`CaptureLegacyProcedurePrototypeMetadata()` reads the prototype/stack surface of a real legacy procedure record back into neutral metadata.

It is deliberately read-only and excludes `procSize`.

The roundtrip fixture proves:

`neutral prototype -> legacy seed -> InfoRec/InfoProcInfo -> neutral prototype`

with full scalar/argument/local equality and unchanged manually-seeded `procSize`.

## Active legacy procedure slots

`ApplyLegacyProcedureMetadataSeedToActiveSession()` creates a detached real `InfoRec`, populates it, then publishes it into the active `Infos[]` slot only after success.

Safety rules:

- active shared session required;
- address must map to backed analysis data;
- slot must be empty;
- no `ProcStart` is synthesized by metadata publication;
- no `procSize` is synthesized.

`loadedInfoSlots` owns the published pointer and session reset cleans it up.

## CFG -> legacy reconciliation

`ApplyDiscoveredProceduresToActiveLegacySession()` bridges procedures already discovered by the neutral CFG into active legacy state.

The design became idempotent after runtime evidence:

- existing valid procedure `InfoRec` records are captured and reused untouched;
- only empty procedure slots invoke the supplied metadata provider;
- a non-procedure occupied slot rejects the batch;
- mapped-position duplicates are rejected;
- all new entries are preflighted before publication;
- if a later publication fails, entries created by that batch are rolled back;
- second execution over the same flow creates nothing and does not call the provider.

The test specifically preserves an existing `ikFunc` with args, locals, flags and `procSize=77` while materializing only the missing callee.

## CLI reconciliation

`idr-cli.exe` now performs the CFG -> legacy procedure reconciliation in its real PE32 execution path.

At present a newly-loaded CLI session normally has empty `Infos[]`, so conservative fallback metadata is `ikProc`; no Delphi prototype is invented. The reuse path remains available for later analysis stages that populate authoritative metadata before reconciliation.

CLI invariants verify that every CFG procedure has a readable procedure-style legacy record after reconciliation.

## Decompiler input model

The first decompiler-facing model originally lived in the legacy wrapper and was then separated into a truly neutral builder.

`IdrDecompilerInput.h` defines:

- `ProcedureDecompileInput`
- current procedure `ProcedureAnalysisInput`
- current `ProcedurePrototypeMetadata`
- unique direct-callee prototype entries.

The neutral builder takes a caller-supplied `ProcedurePrototypeLookup`; therefore it does not depend on:

- `InfoRec`;
- `Infos[]`;
- legacy bridge;
- PE loader;
- image globals.

`IdrLegacyDecompilerInput.h` is now only a thin active-session lookup adapter.

Direct calls to the same callee are deduplicated by callee address in the decompiler input while the original call-site remains available when resolution is needed.

## Headless prototype policy

`IdrHeadlessPrototypePolicy.h` defines:

- `PrototypeResolutionStatus::Resolved`
- `PrototypeResolutionStatus::Unavailable`
- `PrototypeResolutionStatus::Rejected`

and an injected `HeadlessPrototypeResolver`.

Rules:

- complete current metadata bypasses the resolver;
- incomplete metadata without a resolver fails;
- `Unavailable` fails;
- `Rejected` fails;
- `Resolved` still fails unless the returned prototype is complete;
- no automatic fake return type or argument type is permitted.

The neutral decompiler-input fixture forces resolver execution for both:

1. an incomplete current function;
2. an incomplete direct callee function.

For a callee, the resolver receives both callee address and the actual call-site. Source metadata remains unchanged: resolution is read-time, not implicit persistence.

## Legacy ManualInput inventory

Prototype completion is only one interactive family. Source inspection shows `ManualInput()` is also used for cases including:

- missing import return-byte information;
- `@DispInvoke` paths;
- unknown function types;
- indirect-call questions;
- some interface/virtual dispatch cases.

Architecture decision: replace these with narrow explicit headless policies as they become runtime-reachable. Do not build a universal fake dialog layer.

## Real legacy decompiler preflight

### #101 runtime proof

`9b69b2935e201407c539637a9bbbf5c87ef1f9ce` — `Exercise legacy decompiler preflight`

A real active legacy session is constructed with a tiny RET procedure. A real procedure record is seeded, then given explicit `procInfo->procSize = 1`.

The linked MSVC x86 core probe executes:

`TDecompileEnv environment(address, procSize, record)`

then:

`TDecompiler decompiler(&environment)`

then:

`decompiler.Init(address)`

then:

`decompiler.InitFlags()`

Runtime checks prove environment address/size, default stack size (`0x8000` when metadata stack size is zero), BP-based state and preservation of stored `procSize`.

The probe intentionally stops before `TDecompiler::Decompile()`.

This is the first runtime proof that the real legacy decompiler engine itself starts successfully under MSVC x86.

### #102 extracted adapter

`f321469aed6ebb641c4ae7438a5776f382f62ad5` — `Extract legacy decompiler preflight adapter`

The proven sequence moved into:

- `portable/core/IdrLegacyDecompilerRunner.h`
- `portable/core/IdrLegacyDecompilerRunner.cpp`

Public result type:

`LegacyDecompilerPreflightResult`

Current fields:

- `procedureSize`
- `stackSize`
- `bpBased`
- `initialized`

Public call:

`PreflightActiveLegacyProcedure(address, result)`

The public header includes only portable core types. `TDecompiler`, `TDecompileEnv` and generated legacy headers remain implementation details in the `.cpp`.

The runner rejects zero/negative `procSize`; this is deliberate.

### Dedicated preflight workflow

`.github/workflows/portable-legacy-decompiler-preflight.yml`

was added to compile/link/run the extracted runner separately under stock `windows-latest` + MSVC x86.

Its first reported run was green, independently proving the extracted adapter rather than only the inline #101 probe.

The workflow is intended as a temporary isolation proof. Once stable, fold the runner into the primary integration workflow and remove redundant coverage if appropriate.

## Procedure size remains a hard boundary

Legacy behavior demonstrates that `procInfo->procSize` is not interchangeable with neutral CFG observed span.

Known evidence:

- `AnalyzeProc1` writes `procSize` on selected terminating paths;
- nearby `cfProcEnd` writes are commonly commented out;
- `GetProcSize()` returns stored `procSize` first;
- if missing, legacy code calls GUI-owned `EstimateProcSize()`;
- `TDecompileEnv` requires an explicit size.

Therefore:

- do not synthesize `ProcEnd`;
- do not copy `observedSpan` into `procSize`;
- do not silently call the legacy GUI estimator from the portable path;
- preflight currently refuses procedures with no authoritative size.

A headless evidence-backed size source is now one of the immediate blockers before broad real-decompiler execution.

## Compatibility risks

### Borland String indexing

Borland `String` is 1-based; `std::string` is 0-based. Continue auditing only runtime-reached direct indexing rather than globally rewriting source semantics.

### Variant / WideString / Currency / Comp

Compatibility support is transitional and reached-case driven.

### Flags range fidelity

Exact end-boundary behavior of old `SetFlags`/`ClearFlags` still needs fidelity checks when deeper code depends on it.

### Indirect control flow

Neutral CFG currently focuses on direct targets. Indirect calls/jumps, switches/jump tables and richer x86 semantics remain outside the proven engine.

### Metadata mutation during decompilation

Legacy decompiler routines may add locals and otherwise mutate `InfoRec/procInfo`. Any future neutral decompile result boundary must make mutation/ownership explicit.

## Immediate technical frontier

The next work should proceed in this order unless compiler/runtime evidence changes the plan:

1. fold the extracted `IdrLegacyDecompilerRunner` into the normal integration workflow and avoid permanent duplicate CI if the dedicated proof is no longer needed;
2. establish a headless procedure-size policy/source based on legacy evidence instead of `observedSpan` substitution;
3. create a deliberately tiny no-interaction procedure fixture suitable for the first controlled call to real `TDecompiler::Decompile()`;
4. stop immediately on the first runtime-reached unresolved interactive dependency and replace that one family with a narrow headless policy;
5. capture decompiler result/state behind neutral portable types;
6. inspect mutations to locals/args/flags/Infos and lock their semantics with fixtures;
7. continue Borland RTL/String audits only on runtime-reached paths;
8. move to controlled Delphi Win32 fixtures and compare against original IDR;
9. eventually expose deterministic decompiler output from `idr-cli.exe` and publish `idr-cli.exe + dis.dll` as an Actions artifact.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent architecture commit per step.
- Frozen branches stay frozen; `main` stays untouched.
- Never push over an active relevant workflow.
- Green runs: status only, no log harvesting.
- Red runs: metadata -> jobs -> failed log exactly once.
- GitHub writes use `create_blob -> create_tree -> create_commit -> update_ref`.
- Preserve original legacy sources; generated portable copies/adapters carry transitional changes unless deliberate structural porting requires otherwise.
- No PR/merge to `main` yet.
- Keep this journal synchronized with major runtime milestones.
