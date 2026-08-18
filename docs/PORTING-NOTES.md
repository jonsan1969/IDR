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

Latest green architecture milestone:

`fc18fca5e3936a9997977fd6ae59920ae412d60a` — `Fold legacy decompiler preflight into integration`

Run #105 was reported green.

Latest pushed code milestone:

`77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop`

Run #106 was reported red and is not yet diagnosed. Do not infer its cause before reading the failed job log exactly once.

## Foundation retained from integration

The port has established that original IDR analysis/decompiler code can be carried into a free hosted MSVC x86 build without reimplementing the engine.

Retained evidence includes real generated `Disasm.cpp` running against shipped x86 `dis.dll`, complete generated `KnowledgeBase.cpp`, `Infos.cpp`, `Misc.cpp`, and essentially complete `Decompiler.cpp` compiling/linking, linker-driven unresolved-symbol reduction to zero, explicit headless service seams, neutral PE32 loading and a shared authoritative neutral/legacy session.

## PE32 / CFG path

`IdrPeLoader` validates PE32/x86, maps sections into analysis-facing bytes, preserves absolute entry point semantics and binds the loaded image to the active legacy session.

The CLI initializes the real `MDisasm`/`dis.dll` chain and adapts `DISINFO` into neutral decoded instruction facts.

The neutral CFG remains decoder/address-mapper injected and independent of PE globals. It supports bounded candidate/block worklists, direct calls, direct conditional/unconditional branches, explicit `Call`, `BranchTaken`, `FallThrough` edges, call xrefs, procedure ownership, summaries and observed instruction spans.

`observedStart`, `observedEndExclusive` and `observedSpan` are observational only. They are not mapped to legacy `procInfo->procSize`, and no synthetic `ProcEnd` is created.

## Procedure metadata / Infos[] bridge

Neutral prototype metadata covers kind, return type, flags, `bpBase`, `retBytes`, `stackSize`, arguments and locals. `procSize` is deliberately excluded.

Confirmed legacy procedure kinds:

- `ikRefine = 0x25`
- `ikConstructor = 0x26`
- `ikDestructor = 0x27`
- `ikProc = 0x28`
- `ikFunc = 0x29`

The bridge supports neutral -> legacy metadata seeding and read-only legacy -> neutral capture. Runtime roundtrip evidence preserves scalar fields, args, locals and register markers while leaving stored `procSize` untouched.

`ApplyDiscoveredProceduresToActiveLegacySession()` reconciles CFG procedures into `Infos[]` idempotently: valid existing procedure records are reused, empty slots can be materialized, non-procedure occupied slots fail, writes are preflighted, batch-created records roll back on failure, and metadata providers are not called for reused records.

The real CLI runs reconciliation after CFG analysis.

## Neutral decompiler input and prototype policy

`IdrDecompilerInput.h` contains neutral per-procedure analysis, current prototype and unique direct-callee prototype entries. The builder is supplied a prototype lookup and optional `HeadlessPrototypeResolver`, with no dependency on `InfoRec`, `Infos[]`, PE loader or legacy session globals.

`IdrLegacyDecompilerInput.h` is a thin active-session lookup adapter.

`IdrHeadlessPrototypePolicy.h` uses explicit `Resolved`, `Unavailable` and `Rejected` results. Complete prototypes bypass the resolver; incomplete metadata succeeds only when explicit resolver output is complete. No return or argument type is fabricated. Resolver-provided metadata is read-time only.

Prototype resolution has been runtime-tested for both current procedures and direct callees, with callee call-site identity preserved.

## Real legacy decompiler preflight — #101/#102

#101 (`9b69b2935e201407c539637a9bbbf5c87ef1f9ce`) proved a real active legacy session can construct:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags()`

under MSVC x86 using a tiny RET procedure with explicit `procInfo->procSize = 1`.

#102 (`f321469aed6ebb641c4ae7438a5776f382f62ad5`) extracted this into:

- `portable/core/IdrLegacyDecompilerRunner.h`
- `portable/core/IdrLegacyDecompilerRunner.cpp`

The public header exposes only portable types. Legacy decompiler classes remain implementation details.

## Explicit procedure-size policy — #103

#103:

`c462472b64d015e8fd5da7f98dd4ecda99449028` — `Add explicit procedure size resolution policy`

introduced `IdrProcedureSizePolicy.h`.

Important types:

- `ProcedureSizeResolutionStatus::{Resolved, Unavailable, Rejected}`
- `ProcedureSizeSource::{None, LegacyMetadata, HeadlessResolver}`
- `ProcedureSizeResolutionRequest`
- `ProcedureSizeResolutionResult`
- `ResolvedProcedureSize`
- `HeadlessProcedureSizeResolver`

Rules:

- `storedSize > 0` resolves immediately as `LegacyMetadata`;
- resolver is bypassed when stored size exists;
- zero stored size requires an explicit resolver;
- rejected/unavailable/non-positive resolver result fails;
- resolver-based size is not persisted into `InfoRec`;
- preflight reports the effective size source.

Both the normal integration and the then-separate preflight workflow were reported green.

## Unified legacy size source — #104

#104:

`cb8cda462f6576d4ad2de9d2ea7f88362400de57` — `Unify legacy procedure size resolution`

removed a dangerous transitional fallback in `PortableEstimateProcSize()`.

Before #104, missing stored size could be synthesized from the next `ProcStart`, or ultimately from the remainder of the image. This was not equivalent to legacy authoritative `procSize` and could have silently fed an invented extent into the decompiler.

Current portable rule is:

`stored procSize -> session HeadlessProcedureSizeResolver -> 0`

The active session exposes:

- `SetLegacyProcedureSizeResolver(...)`
- `LegacyProcedureSizeResolver()`

and session reset clears the resolver.

`PreflightActiveLegacyProcedure()` still accepts an explicitly injected resolver for focused testing; when none is supplied it uses the session resolver. `PortableEstimateProcSize()` uses the same session policy.

The runtime probe plants a later `ProcStart` specifically to prove the old hidden fallback no longer creates a size.

Both #104 CI paths were reported green.

## CI consolidation — #105

#105:

`fc18fca5e3936a9997977fd6ae59920ae412d60a` — `Fold legacy decompiler preflight into integration`

was deliberately mechanical CI consolidation with no production decompiler behavior change.

It moved compilation/link/execution of `IdrLegacyDecompilerRunner.cpp` and `idr_legacy_decompiler_runner_probe.cpp` into `.github/workflows/portable-core-integration.yml` and removed the temporary `.github/workflows/portable-legacy-decompiler-preflight.yml`.

Run #105 was reported green, so the isolated preflight workflow phase is complete and the project again has one primary CI line.

## First full legacy Decompile() attempt — #106

#106:

`77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop`

moves beyond preflight for the first time.

The intended fixture is deliberately minimal: one byte `C3` (`RET`) with explicit stored `procSize = 1`.

The intended execution sequence is:

1. validate session / real procedure `InfoRec`;
2. resolve procedure size;
3. construct `TDecompileEnv`;
4. construct `TDecompiler`;
5. `Init(address)`;
6. `InitFlags()`;
7. `SetStop(address + size)`;
8. `Decompile(address, 0, nullptr)`.

The probe also installs a counting `manualInput` service and expects zero calls. It intends to verify that `Decompile()` returns, `WasRet` is true, and stored `procSize` remains unchanged.

Run #106 is RED. Therefore the new `Decompile()` loop is not yet runtime-proven. The exact failing stage is unknown until the failed job log is obtained. No corrective code should be based on speculation.

## ManualInput inventory

Prototype completion is only one interactive family. Source inspection has already identified additional `ManualInput()` paths including missing import return-byte data, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.

Architecture decision remains: add narrow explicit headless policies only for runtime-reached families. Do not build a generic fake-GUI dialog shim.

## Procedure-size / ProcEnd invariant

Do not synthesize `ProcEnd` and do not map CFG `observedSpan` into `procInfo->procSize`.

Legacy evidence shows stored `procSize` has separate semantics and `TDecompileEnv` requires a concrete size. The portable path now makes any non-stored size source explicit through `HeadlessProcedureSizeResolver`.

No implicit next-`ProcStart`, image-remainder or observed-span fallback is allowed.

## GitHub Actions discovery state

Preferred push-run discovery procedure:

1. call `GitHub.fetch` on `/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20`;
2. locate the requested `run_number`;
3. verify its `head_sha` against the pushed commit;
4. take the internal run `id`;
5. call `fetch_workflow_run_jobs`;
6. only if failed, call `fetch_workflow_job_logs` for the failed job exactly once.

Current session limitation:

- the `/actions/runs` collection endpoint returns `endpoint not allowed` / HTTP 400;
- `api_tool.list_resources` with `workflow run` exposes no separate branch/push run-list function;
- `fetch_commit_workflow_runs` is explicitly PR-run-only and is not a substitute;
- with a known `run_id`, `fetch_workflow_run_jobs` and `fetch_workflow_job_logs` remain usable.

Before asking the user for a run id in future sessions, first retry the branch+event collection route and then capability discovery (`workflow run`, `actions`, or `runs`). If still unavailable, state explicitly that the session connector lacks run-discovery while downstream run-id tools remain available.

Do not start by attempting run discovery through commit SHA.

## Compatibility risks

### Borland String indexing

Borland `String` is 1-based; `std::string` is 0-based. Continue auditing runtime-reached indexing instead of speculative global rewrites.

### Variant / WideString / Currency / Comp

Compatibility support remains transitional and reached-case driven.

### Flag boundary fidelity

Exact old `SetFlags` / `ClearFlags` end-boundary behavior remains a targeted fidelity issue.

### Indirect control flow

Neutral CFG still focuses on direct targets. Indirect calls/jumps and jump tables remain outside the proven path.

### Decompiler mutation

Legacy decompiler routines may add locals and mutate procedure/flag state. A neutral full-decompile result boundary must make mutation and ownership explicit.

## Immediate technical frontier

1. Obtain #106 run/job evidence using the preferred branch push-run discovery when connector capability permits.
2. Fetch the failed #106 job log exactly once.
3. Fix only the evidenced failure.
4. Get the one-byte RET `Decompile()` path green with zero `ManualInput()` calls.
5. Extract its result/state behind a neutral API rather than exposing legacy engine classes.
6. Advance to a slightly richer no-interaction procedure.
7. Stop on the first runtime-reached unresolved interactive dependency and model that family explicitly.
8. Continue controlled Delphi Win32 fixture comparison against original IDR.
9. Eventually expose deterministic output from `idr-cli.exe` and publish `idr-cli.exe + dis.dll` as an Actions artifact.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent architecture commit per step.
- Frozen branches stay frozen; `main` stays untouched.
- Never push over an active relevant workflow.
- Green runs: status only; no log harvesting.
- Red runs: metadata -> jobs -> failed log exactly once.
- GitHub writes use `create_blob -> create_tree -> create_commit -> update_ref`.
- Do not intentionally use `create_file` or `update_file` for branch writes.
- Preserve original legacy sources; generated portable copies/adapters carry transitional changes unless deliberate structural porting requires otherwise.
- No PR/merge to `main` yet.
- Keep this journal synchronized with major runtime milestones.

## Write-history note

A number of earlier turns accidentally attempted `create_file` / `update_file` calls against deliberately nonexistent refs such as `__invalid__` or `__never__`. Those calls returned 404 and produced no repository writes. Actual branch-changing commits discussed here were made through the atomic git-object sequence unless explicitly documented otherwise. Do not use those contents-API calls as probes going forward.
