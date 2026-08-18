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

## Current status

Latest green architecture milestone:

`fc18fca5e3936a9997977fd6ae59920ae412d60a` — `Fold legacy decompiler preflight into integration`

Run #105 was reported green.

Latest pushed code milestone:

`77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop`

Run #106 was reported red. Its failure has NOT yet been diagnosed because this ChatGPT session's GitHub connector currently lacks working push-run discovery. Do not infer the cause before obtaining the failed job log exactly once.

## Current architecture milestone

The verified runtime chain through #105 is:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> explicit procedure-size resolution -> real TDecompileEnv/TDecompiler preflight`

The real legacy decompiler is proven under MSVC x86 through:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags()`

#106 extends the runner to attempt:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags() -> SetStop() -> Decompile()`

using a one-byte `RET` procedure, but this full-loop path is not yet runtime-proven because #106 is red.

## CI

Primary workflow:

`.github/workflows/portable-core-integration.yml`

The former temporary workflow:

`.github/workflows/portable-legacy-decompiler-preflight.yml`

was removed by #105 after its purpose was fulfilled. `IdrLegacyDecompilerRunner.cpp` and `idr_legacy_decompiler_runner_probe.cpp` are now compiled, linked and run inside the main integration workflow.

CI platform remains stock `windows-latest`, MSVC x86 via `vswhere.exe` / `vcvars32.bat`, generated portable legacy translation units, and the real x86 `dis.dll` decoder path.

x86 remains intentional because the shipped decoder and legacy engine path are x86.

### Actions discipline

- Do not push while a relevant workflow run is active.
- Green run: status/metadata only; do not fetch logs.
- Failed run: metadata -> jobs -> failed job log exactly once.
- Never repeatedly fetch the same failed log.
- Preferred push-run discovery is `GitHub.fetch` against `/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20`, then select `run_number`, verify `head_sha`, use the internal run `id`, call `fetch_workflow_run_jobs`, and only for a failed run call `fetch_workflow_job_logs` once.
- In the current connector session that `/actions/runs` collection endpoint is blocked with `endpoint not allowed`.
- Capability discovery with `api_tool.list_resources` confirms there is currently no separate function for listing push/workflow runs by branch.
- `fetch_commit_workflow_runs` is not a substitute: it is explicitly limited to pull-request-triggered runs.
- Once a `run_id` is known, `fetch_workflow_run_jobs` and `fetch_workflow_job_logs` remain available.
- Do not fall back to commit-SHA run discovery as the normal approach.

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
- `67d6c8ff1a960a9f0edef2c1dad31a988f527bf5` — `Update portable core working state`
- `c462472b64d015e8fd5da7f98dd4ecda99449028` — `Add explicit procedure size resolution policy`
- `cb8cda462f6576d4ad2de9d2ea7f88362400de57` — `Unify legacy procedure size resolution`
- `fc18fca5e3936a9997977fd6ae59920ae412d60a` — `Fold legacy decompiler preflight into integration`
- `77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop` — run #106 red, failure pending diagnosis.

## Neutral CFG state

The bounded neutral CFG remains independent of PE/image globals. Its standalone probe links only `IdrAnalysisState.obj + idr_control_flow_probe.obj`.

Current behavior includes bounded deduplicated candidate/block worklists, direct calls, direct conditional/unconditional branches, explicit `Call` / `BranchTaken` / `FallThrough` edges, per-procedure ownership, call xrefs, procedure summaries, incoming/outgoing query helpers and observed instruction spans.

Successful procedures get `ProcStart`; ordinary branch locations do not. Invalid/out-of-image targets create no edges/xrefs/candidates. No synthetic `ProcEnd` is created.

`observedSpan` remains observational only and is not legacy `procInfo->procSize`.

## Procedure metadata and active Infos[] reconciliation

The neutral procedure prototype model includes kind, return type, flags, `bpBase`, `retBytes`, `stackSize`, arguments and locals. `procSize` is deliberately excluded.

Legacy constants remain confirmed:

- `ikRefine = 0x25`
- `ikConstructor = 0x26`
- `ikDestructor = 0x27`
- `ikProc = 0x28`
- `ikFunc = 0x29`

The bridge supports neutral -> legacy seed -> real `InfoRec`, and read-only legacy -> neutral capture. Runtime roundtrip tests preserve scalar fields, args, locals and register markers while leaving existing `procSize` untouched.

CFG-discovered procedures can be reconciled into active `Infos[]` idempotently. Existing valid procedure records are reused; empty slots may be materialized; occupied non-procedure slots fail; publication is preflighted; current-batch writes roll back on failure; provider calls are skipped for reused records; no size is inferred from CFG span.

The real CLI performs this reconciliation after CFG analysis.

## Neutral decompiler-input boundary

`IdrDecompilerInput.h` defines a neutral read model containing per-procedure analysis, current prototype and unique direct-callee prototypes.

The neutral builder takes `ControlFlowResult`, target address, function-kind constant, caller-supplied prototype lookup and an optional headless prototype resolver. It has no dependency on `InfoRec`, `Infos[]`, PE loader or legacy session globals.

`IdrLegacyDecompilerInput.h` is only a thin active-session lookup adapter.

The resolver receives call-site identity for unresolved callees. Resolver output is read-time only and does not mutate source metadata.

## Headless prototype resolution

`IdrHeadlessPrototypePolicy.h` exposes `Resolved`, `Unavailable` and `Rejected` states.

Complete prototypes bypass the resolver. Incomplete prototypes succeed only when the resolver returns a complete explicit prototype. No return type or argument type is fabricated automatically.

Runtime tests cover unresolved current functions and unresolved direct callees.

This solves prototype completeness only; other `ManualInput()` families remain separate.

## Explicit procedure-size policy (#103)

`IdrProcedureSizePolicy.h` introduces explicit procedure-size resolution.

Sources:

- `ProcedureSizeSource::LegacyMetadata`
- `ProcedureSizeSource::HeadlessResolver`
- `ProcedureSizeSource::None`

Rules proven green in #103:

- positive stored `procInfo->procSize` wins and bypasses the resolver;
- zero stored size may be resolved only by an explicit `HeadlessProcedureSizeResolver`;
- rejected/unavailable/non-positive resolver output fails;
- resolver-provided size is read-time only and is not written back to `InfoRec`;
- the decompiler preflight result reports the size source.

## Unified legacy procedure-size resolution (#104)

#104 removed the transitional hidden fallback from `PortableEstimateProcSize()`.

Previously it could fall back to the next `ProcStart`, then to the remainder of the image. That behavior is no longer allowed in the headless bridge.

Current rule:

`stored procSize -> session HeadlessProcedureSizeResolver -> 0`

The active resolver is stored in the legacy bridge through:

- `SetLegacyProcedureSizeResolver(...)`
- `LegacyProcedureSizeResolver()`

`ResetLegacyLoadedPeSession()` clears it.

`PreflightActiveLegacyProcedure()` uses a directly supplied resolver for isolated tests, otherwise the session resolver. `PortableEstimateProcSize()` uses the same session policy, so the portable legacy path no longer has two competing definitions of procedure size.

The probe explicitly plants a later `ProcStart` and verifies it does NOT synthesize a size.

#104 integration and dedicated preflight runs were both reported green.

## Legacy decompiler runner

The real engine is wrapped by:

- `portable/core/IdrLegacyDecompilerRunner.h`
- `portable/core/IdrLegacyDecompilerRunner.cpp`

The public header does not expose `TDecompiler` or `TDecompileEnv`.

Through #105, the runtime-proven preflight sequence is:

1. validate active legacy session and procedure record;
2. resolve explicit procedure size;
3. construct `TDecompileEnv`;
4. construct `TDecompiler`;
5. call `Init()`;
6. call `InitFlags()`;
7. return a neutral `LegacyDecompilerPreflightResult`.

The result includes procedure size, size source, effective stack size, BP-based state and initialization success.

## First full Decompile() attempt (#106)

#106 extends the runner with a minimal full-loop path using a one-byte `RET` fixture and explicit stored `procSize = 1`.

The intended sequence is:

`TDecompileEnv -> TDecompiler -> Init -> InitFlags -> SetStop(StartAdr + Size) -> Decompile(StartAdr, 0, nullptr)`

The probe installs a `manualInput` service that counts calls and requires zero interactive calls. It also intends to verify that `Decompile()` returns and `WasRet` becomes true without mutating the stored `procSize`.

Run #106 is red. Therefore NONE of those new full-loop assertions are yet runtime evidence. The compiler/link/runtime failure must be diagnosed from the failed job log before changing code.

## Remaining interactive legacy risks

Known `ManualInput()` families include import return-byte questions, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.

Continue replacing these with narrow explicit headless policies only when runtime evidence reaches them. Do not build a broad fake-dialog layer.

## Known risks

- Borland `String` direct indexing is 1-based versus `std::string` 0-based.
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility remain transitional.
- exact legacy flag-range boundary fidelity remains open.
- indirect control flow, switch/jump tables and richer x86 CFG remain unsupported.
- imports/exports/resources and Delphi-specific discovery are not yet fully driven through CLI.
- decompiler paths may mutate locals/args/flags/InfoRec; result ownership must be made explicit.
- full `TDecompiler::Decompile()` has been attempted but is not yet proven green.

## Immediate next phase

1. Diagnose #106 from the actual failed job log, exactly once, when run discovery or a run id becomes available.
2. Fix only the evidenced compiler/link/runtime failure.
3. Re-run the minimal one-byte RET decompiler loop until the full `Decompile()` call is green with zero `ManualInput()` calls.
4. Capture full-loop result/state behind neutral result types.
5. Move next to a slightly richer no-interaction fixture, then stop at the first real interactive dependency.
6. Continue controlled Delphi Win32 fixture comparisons against original IDR.
7. Later publish deterministic `idr-cli.exe + dis.dll` artifacts.

## Working rules

- Compiler/linker/runtime evidence before speculation.
- One coherent architecture commit at a time.
- Frozen branches and `main` stay untouched.
- Do not push while a relevant workflow run is active.
- Green runs: metadata/status only.
- Failed runs: metadata -> jobs -> failed job log exactly once.
- Preferred write path: `create_blob -> create_tree -> create_commit -> update_ref`.
- Do not intentionally use `create_file` / `update_file` for branch writes.
- Preserve original legacy source unless a deliberate structural port requires a change.
- No PR/merge to `main` yet.
- Keep `PORTABLE-CORE-STATE.md` and `PORTING-NOTES.md` synchronized at major architecture milestones.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> real PE32 analysis -> headless legacy decompiler -> deterministic idr-cli.exe output -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
