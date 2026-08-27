# Portable Core Project State

Last updated: 2026-08-27

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active product branch: `agent/portable-cli`

Frozen / do not modify:

- `main`
- `agent/portable-core-integration`
- `agent/portable-core-smoke`

The repository is the source of truth. Always verify branch HEAD directly from GitHub before writes.

## Current verified status

Current code HEAD before this docs-only update:

`c49c327e58057a8cfd061ec3412bb625cffbb597` — `Exercise legacy stack frame decompile`

Latest verified green run:

- run #109
- internal run id `33069570064`
- head SHA `c49c327e58057a8cfd061ec3412bb625cffbb597`
- workflow `Portable CLI integration`
- conclusion `success`

No #109 logs were fetched because the run was green.

Recent runtime milestones:

- `d479411c1e45ce13d2293de64ecfeb8ab207e3d4` — `Initialize legacy disassembler before decompile probe` — run #107 green.
- `1539163f8c90432699076a5ad0ff7eca1504390e` — `Expose neutral decompiler execution result` — run #108 green.
- `c49c327e58057a8cfd061ec3412bb625cffbb597` — `Exercise legacy stack frame decompile` — run #109 green.

## #106 diagnosis and resolution

`77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop` — run #106 failed at runtime with Windows access violation `0xC0000005` while executing `idr-legacy-decompiler-runner-probe.exe`.

The failed job log was fetched exactly once. Do not fetch that #106 failed job log again.

The concrete cause was that the full `Decompile()` loop calls `MDisasm::Disassemble()`, whose backend pointer is initialized by `Disasm.Init()`. The CLI already initialized the real `MDisasm/dis.dll` chain, but the focused #106 probe did not. The fix was confined to the portable probe; original legacy source was not modified.

Run #107 proved the corrected minimal one-byte `RET` full-loop path green.

## Current architecture milestone

Verified runtime chain:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> explicit procedure-size resolution -> real TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result`

The real legacy decompiler is now runtime-proven under MSVC x86 through:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags() -> SetStop() -> Decompile()`

The minimal `RET` fixture and a richer classic stack-frame fixture both complete without interactive input.

## Neutral decompiler execution result

`IdrLegacyDecompilerRunner` remains the legacy adapter; its public API does not expose `TDecompiler`, `TDecompileEnv` or other legacy engine classes.

A neutral full-decompile result now carries portable data including procedure address/size information, size source, end address, RET/completion state and decompiler body lines as standard strings. Legacy `TStringList` ownership remains on the implementation side.

Run #108 verified this neutral result boundary in the primary integration workflow.

## Richer no-interaction fixture

Run #109 exercises a multi-instruction x86 stack-frame procedure:

`55 8B EC 5D C3`

which is:

`PUSH EBP; MOV EBP,ESP; POP EBP; RET`

This advances beyond the single-byte `RET` fixture into real stack/register state transitions while avoiding calls/imports/prototype prompts. Run #109 is green.

## Explicit procedure-size invariant

Current rule:

`stored procSize -> session HeadlessProcedureSizeResolver -> unavailable/0`

Do not infer size from:

- next `ProcStart`;
- image remainder;
- neutral CFG `observedSpan`.

Do not synthesize `ProcEnd`.

Resolver-provided size is read-time only and is not persisted into `InfoRec`.

## Neutral CFG / metadata boundaries

- Neutral CFG remains independent of PE/image globals.
- `observedSpan` is observational only.
- Neutral prototype metadata excludes `procSize`.
- `IdrDecompilerInput.h` is the neutral read-model builder.
- `IdrLegacyDecompilerInput.h` is the active-session adapter only.
- Prototype resolution uses explicit `Resolved / Unavailable / Rejected` policy and never fabricates missing return/argument types.
- CFG-discovered procedures reconcile into active `Infos[]` idempotently; existing valid procedure records are reused and no size is inferred from CFG span.

## CI

Primary workflow:

`.github/workflows/portable-core-integration.yml`

Platform:

- stock GitHub-hosted Windows runner;
- MSVC x86;
- generated portable legacy translation units;
- shipped real x86 `dis.dll` decoder path.

x86 remains intentional because the shipped decoder and legacy engine path are x86.

### Actions discipline

- Never push while a relevant workflow run is active.
- Green run: metadata/status only; do not fetch logs.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed job log twice.

Preferred push-run discovery:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

The collection endpoint is working in the current connector session.

If it returns `endpoint not allowed`, capability-check GitHub tools with queries such as `workflow run`, `actions`, or `runs`. If no branch/push run-listing capability exists, state that explicitly. Do not use `fetch_commit_workflow_runs` as a substitute because it is PR-triggered-run limited.

Once a `run_id` is known, use `fetch_workflow_run_jobs`; fetch `fetch_workflow_job_logs` only for a failed job and only once.

## Git write discipline

All active-branch writes use:

`create_blob -> create_tree -> create_commit -> update_ref`

Before writes:

1. verify current branch HEAD;
2. verify no relevant workflow is active;
3. build from that exact current tree;
4. use non-forced fast-forward ref update.

Do not intentionally use contents-API `create_file` / `update_file` for branch writes.

## Legacy-source discipline

Preserve original legacy source unless a deliberate structural porting decision requires otherwise.

Prefer:

- portable adapters;
- generated portable copies;
- narrow explicit policies/services;
- runtime-reached compatibility shims.

Do not build a broad fake-GUI/dialog layer.

## Known risks

- Borland `String` direct indexing is 1-based versus `std::string` 0-based.
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility remain transitional.
- Exact legacy flag-range boundary fidelity remains open.
- Neutral CFG still lacks indirect calls/jumps and jump-table/switch handling.
- Legacy decompiler routines can mutate locals, args, flags and `InfoRec`; neutral ownership/mutation semantics must remain explicit.
- Remaining `ManualInput()` families include imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.

## Immediate next phase

1. Advance from the green #109 stack-frame fixture to the smallest deterministic fixture that produces meaningful neutral body output.
2. Keep `manualInput` counting enabled and require zero calls.
3. Stop at the first runtime-reached unresolved interactive dependency; model only that specific family with an explicit headless policy.
4. Preserve the neutral result boundary; do not expose legacy engine classes.
5. Continue controlled Delphi Win32 fixture comparison against original IDR.
6. Later expose deterministic `idr-cli.exe` output and publish `idr-cli.exe + dis.dll` artifacts.

## Working rules

- Compiler/linker/runtime evidence before speculation.
- One coherent architecture commit at a time.
- Frozen branches and `main` stay untouched.
- Green runs: metadata/status only.
- Failed runs: metadata -> jobs -> failed log exactly once.
- Keep these docs synchronized at meaningful runtime milestones.
