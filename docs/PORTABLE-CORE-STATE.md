# Portable Core Project State

Last updated: 2026-08-18

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active product branch: `agent/portable-cli`

Frozen references — do not modify:

- `agent/portable-core-integration`
- `agent/portable-core-smoke`

`main` remains untouched at the modern Embarcadero baseline.

Current verified code milestone:

`f321469aed6ebb641c4ae7438a5776f382f62ad5` — `Extract legacy decompiler preflight adapter`

Both the normal integration run #102 and the first dedicated `Portable legacy decompiler preflight` workflow run were reported green.

## Current architecture milestone

The project has moved beyond loader/CFG plumbing and has now entered the real legacy decompiler engine under MSVC.

The verified runtime chain is now:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> real TDecompileEnv/TDecompiler preflight`

The legacy decompiler has been constructed and initialized under MSVC x86 using:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags()`

The current boundary deliberately stops before `Decompile()` so unresolved interactive `ManualInput()` paths cannot yet be reached.

## CI

Primary workflow:

`.github/workflows/portable-core-integration.yml`

Dedicated temporary preflight workflow:

`.github/workflows/portable-legacy-decompiler-preflight.yml`

Both use stock `windows-latest`, MSVC x86 via `vswhere.exe`/`vcvars32.bat`, generated portable legacy translation units and the real x86 `dis.dll` decoder path.

The dedicated preflight workflow currently compiles and links `IdrLegacyDecompilerRunner.cpp` and runs `idr_legacy_decompiler_runner_probe.cpp`. It exists to prove the extracted adapter independently before folding that adapter into the main integration workflow.

x86 remains intentional because the real legacy decoder and shipped `dis.dll` are x86.

### Actions discipline

- Do not push while a relevant workflow run is active.
- Green run: status/metadata is enough; do not fetch logs.
- Failed run: inspect metadata -> jobs -> failed job log exactly once.
- Do not repeatedly fetch the same failed log.
- Current connector limitations may require the user to report a run number/status when SHA-to-run discovery is unavailable.

## Active milestone history

Key commits on `agent/portable-cli`:

- `7f3f3fe68db55c95a7928bbc86f3bf2580519113` — `Establish portable MSVC x86 core baseline`
- `aaf47e2382a3b6419a74230931a052f6481aa6d0` — `Introduce portable CLI host`
- `fcfc08db2b588036afc16a2e882b80a0d186dcb0` — `Decode loaded entry point in portable CLI`
- `df4a270933e7e0de6d77af69b1c8e3856fd6a6d4` — `Extract bounded control flow from CLI`
- `428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`
- `0631b5d52bcf0a02079d70e5490653a4e4f09ab3` — `Exercise richer neutral control flow`
- `35ad39eb8c61b7c36be59e9bfcc42e3e0dfc0eab` — `Add neutral control flow queries`
- `6fd7901ca9779f817af9574098b7280a8cb26d6c` — `Record observed procedure instruction span`
- `4da780e359051bec7f9521152a49a7ac3e4dcb30` — `Expose observed procedure spans in CLI`
- `7361b7b42381049498a489c6b3592a76ef54f884` — `Introduce neutral procedure analysis input`
- `3f01aec1c5867bb1176652296be6c486342b3d74` — `Exercise neutral procedure analysis boundary`
- `ad156ad3de342c413df820bcb9dc8e8d69c8f99a` — `Model neutral procedure prototype metadata`
- `e1e7b6cec901962aace44e8e2f8ee1fabecb4ef2` — `Exercise neutral procedure prototype metadata`
- `5921ac0018fd55c13d0e27bf01c22f44afb7026b` — `Run neutral prototype metadata fixture`
- `e346d5379710c119e432c2df7afcf54fdcb65f0c` — `Seed legacy procedure metadata bridge`
- `70552948b69a357ce73f679cdd9536027634d003` — `Exercise legacy metadata seed mapping`
- `252f2560fe43fd205cea094586b479271e856b09` — `Apply metadata seed to legacy procedure record`
- `fc66b4cdfd7e1ddabd5a070edd4755c3194258b6` — `Seed active legacy procedure slot`
- `6823bc1302a8c3c6f445d6249bdc55494732ff45` — `Materialize discovered procedures in legacy session`
- `41e592115adce5fd594a3f10e9c8116997a1ecd9` — `Capture legacy procedure prototype metadata`
- `2c25199633d1a73ce0e6a6c0815ada4f8c8bf6fe` — `Exercise legacy prototype metadata roundtrip`
- `9a76d87c9b72753ba3b2a7327add1112208148eb` — `Reuse existing legacy procedure metadata`
- `00cc79babab61cb5b175baab6130f01b08423abe` — `Materialize CFG procedures in portable CLI`
- `f9ad78618505f7866dc0d82ee5a3e6ca8634bf83` — `Build decompiler input from reconciled procedures`
- `b16872d68f6cfeb375aed2ea358fe3743ddf06cd` — `Define headless prototype resolution policy`
- `2260c705bb7e24dbcd3276a6afaed560ff58afa2` — `Use headless resolver for decompiler input`
- `06c76bc2aed27c3434f21dab3eff5e3c11069b4d` — `Separate neutral decompiler input builder`
- `9b69b2935e201407c539637a9bbbf5c87ef1f9ce` — `Exercise legacy decompiler preflight`
- `f321469aed6ebb641c4ae7438a5776f382f62ad5` — `Extract legacy decompiler preflight adapter`

## Neutral CFG state

The bounded neutral CFG remains independent of PE/image globals. Its standalone probe links only `IdrAnalysisState.obj + idr_control_flow_probe.obj`.

Current behavior includes:

- bounded deduplicated procedure-candidate worklist;
- bounded basic-block worklist per procedure;
- direct calls and direct conditional/unconditional branches;
- `Call`, `BranchTaken`, `FallThrough` edge kinds;
- edge ownership by procedure;
- direct-call xrefs `{ caller, callSite, callee }`;
- incoming/outgoing query helpers;
- procedure summaries;
- observed instruction span fields;
- `Instruction`, `Code`, `Call`, `Loc`, `ProcStart` propagation;
- no synthetic `ProcEnd`;
- invalid/out-of-image targets create no edges/xrefs/candidates.

`observedSpan` is observational only. It is not legacy `procInfo->procSize`.

## Procedure metadata bridge

The portable model now has neutral value-only procedure metadata for:

- kind;
- return type;
- flags;
- `bpBase`;
- `retBytes`;
- `stackSize`;
- register/stack arguments;
- locals.

Actual legacy procedure constants are confirmed:

- `ikRefine = 0x25`
- `ikConstructor = 0x26`
- `ikDestructor = 0x27`
- `ikProc = 0x28`
- `ikFunc = 0x29`

`procSize` is deliberately excluded from the neutral prototype/seed model.

The bridge is bidirectional on the prototype surface:

`ProcedurePrototypeMetadata -> LegacyProcedureMetadataSeed -> InfoRec/InfoProcInfo`

and

`InfoRec/InfoProcInfo -> ProcedurePrototypeMetadata`

A runtime roundtrip test proves scalar fields, args, locals, register markers and types are preserved while an existing legacy `procSize` remains untouched.

## Active Infos[] reconciliation

CFG-discovered procedures can now be reconciled into the active legacy `Infos[]` session.

Rules proven at runtime:

- only addresses present in `flow.procedures` are processed by the batch bridge;
- address must map into backed active analysis data;
- empty slots may be materialized from caller-supplied metadata;
- existing valid procedure records are reused and not overwritten;
- non-procedure occupied slots fail reconciliation;
- duplicate mapped positions are rejected;
- writes are preflighted before publication;
- partial batch writes roll back newly created records on failure;
- a second reconciliation is idempotent;
- provider is not called for already valid existing procedure records;
- CFG `ProcStart` state remains separate from metadata publication;
- no procedure size is inferred from `observedSpan`.

The real `idr-cli.exe` path now performs this reconciliation after CFG analysis.

## Decompiler input boundary

`IdrDecompilerInput.h` defines a neutral read model containing:

- `ProcedureAnalysisInput`;
- current procedure prototype;
- unique direct-callee prototypes.

The neutral builder receives:

- `ControlFlowResult`;
- target procedure address;
- function-kind constant;
- caller-supplied prototype lookup;
- optional headless prototype resolver.

The active legacy wrapper supplies `Infos[]` only as a prototype lookup source. The neutral builder itself has no dependency on `InfoRec`, `Infos[]`, PE loader or legacy session globals.

Direct calls to the same callee are deduplicated in the decompiler input, while call-site identity remains available to the resolver.

## Headless prototype resolution policy

`IdrHeadlessPrototypePolicy.h` provides an explicit policy result:

- `Resolved`
- `Unavailable`
- `Rejected`

Complete prototypes pass through without invoking the resolver.

Incomplete prototypes are accepted only if the resolver explicitly returns `Resolved` with a complete prototype. No return type or argument type is fabricated automatically.

Runtime tests prove resolution for both the current procedure and a direct callee. Resolver-provided metadata is read-time only; source metadata is not mutated.

This policy replaces only prototype-completeness interaction. It does not yet cover every legacy `ManualInput()` path.

## Real legacy decompiler preflight

Commit #101 proved the first real invocation of the legacy decompiler engine under MSVC x86.

The tested sequence is:

1. activate a real loaded legacy session;
2. create a real procedure `InfoRec`;
3. provide explicit authoritative `procInfo->procSize`;
4. construct `TDecompileEnv`;
5. construct `TDecompiler`;
6. run `TDecompiler::Init()`;
7. run `TDecompiler::InitFlags()`;
8. stop before `Decompile()`.

Commit #102 extracted that sequence into:

- `portable/core/IdrLegacyDecompilerRunner.h`
- `portable/core/IdrLegacyDecompilerRunner.cpp`

Public API:

`PreflightActiveLegacyProcedure(DWord address, LegacyDecompilerPreflightResult &result)`

The public header does not expose `TDecompiler` or `TDecompileEnv`.

Preflight rejects:

- missing/inactive legacy session;
- invalid/unbacked address;
- missing `InfoRec`;
- non-procedure record;
- missing `procInfo`;
- `procSize <= 0`;
- incomplete legacy prototype causing `Init()` failure.

Successful output currently records procedure size, effective stack size, BP-based state and initialization success.

The dedicated preflight workflow independently proved this adapter green.

## Procedure size / ProcEnd rule

Do not synthesize `ProcEnd` and do not map neutral `observedSpan` into legacy `procInfo->procSize`.

Legacy evidence remains:

- `AnalyzeProc1` writes `procInfo->procSize` on selected terminating paths;
- common adjacent `cfProcEnd` writes are commented out;
- `GetProcSize()` prefers stored `procSize`, otherwise calls GUI-owned `EstimateProcSize()`;
- `TDecompileEnv` requires a concrete decompilation size.

The current preflight therefore deliberately refuses zero `procSize` instead of silently using the neutral observed span.

A future headless procedure-size policy/estimator is still required before arbitrary CFG-discovered procedures can enter the real decompiler.

## Remaining interactive legacy risks

`ManualInput()` is used in more places than prototype completion. Known areas include:

- import return-byte questions;
- `@DispInvoke`-related paths;
- unknown function types;
- indirect calls;
- some virtual/interface call cases.

These must be replaced by explicit headless policy seams one family at a time. Do not create a broad fake-GUI shim.

## Known risks

- Borland `String` direct indexing is 1-based versus `std::string` 0-based.
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility remain transitional.
- exact legacy `SetFlags`/`ClearFlags` end-boundary fidelity remains open.
- indirect calls/jumps, switch/jump tables and richer x86 control flow remain unsupported by neutral CFG.
- headless procedure-size estimation is not yet established.
- imports/exports/resources and Delphi-specific discovery are not yet fully driven through CLI.
- the real decompiler has not yet executed `Decompile()` in headless mode.
- runtime-reached legacy decompiler paths may expose additional RTL or service dependencies.

## Immediate next phase

1. fold `IdrLegacyDecompilerRunner` into the primary integration workflow once the dedicated preflight proof is considered stable;
2. establish an evidence-backed headless procedure-size source instead of using `observedSpan` as `procSize`;
3. prepare a deliberately trivial procedure fixture whose first actual `TDecompiler::Decompile()` step cannot require interactive input;
4. introduce explicit headless policies for the first runtime-reached `ManualInput()` family as required;
5. capture decompiler output/state behind neutral result types rather than exposing legacy classes;
6. continue auditing runtime-reached Borland String/RTL semantics;
7. add controlled Delphi Win32 fixtures and compare results with original IDR;
8. later package `idr-cli.exe` and required `dis.dll` as an Actions artifact.

## Working rules

- Compiler/linker/runtime evidence before speculation.
- One coherent architecture commit at a time.
- Do not disturb frozen branches or `main`.
- Do not push while a relevant workflow run is active.
- Successful runs: metadata/status only, no logs.
- Failed runs: metadata -> jobs -> failed job log exactly once.
- Preserve original legacy source unless a deliberate structural port requires a change.
- GitHub writes on the active branch use `create_blob -> create_tree -> create_commit -> update_ref`.
- No PR/merge to `main` yet.
- Keep `PORTABLE-CORE-STATE.md` and `PORTING-NOTES.md` synchronized at major architecture milestones.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> real PE32 analysis -> headless legacy decompiler -> deterministic idr-cli.exe output -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
