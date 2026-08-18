# Agent Handoff — IDR Portable CLI

Last updated: 2026-08-18

## Repository and branches

Repository: `jonsan1969/IDR`

Active branch: `agent/portable-cli`

Frozen / do not modify:

- `main`
- `agent/portable-core-integration`
- `agent/portable-core-smoke`

The repository is the source of truth. Do not use old chat memory as current project status.

## Verified HEAD at handoff creation

Verified branch baseline immediately before this docs-only handoff commit:

`2e3c1f828b91c380cb15c5564232278efb16bec5` — `Update portable decompiler working state`

`GitHub.compare_commits(base=2e3c1f8..., head=agent/portable-cli)` reported `identical`, `ahead_by=0`, `behind_by=0`.

This handoff itself is added as a docs-only commit on top of that verified baseline. Always verify `agent/portable-cli` HEAD again before any future write.

## Latest verified green run

Latest verified green architecture milestone:

`fc18fca5e3936a9997977fd6ae59920ae412d60a` — `Fold legacy decompiler preflight into integration`

Run #105 was reported green.

Latest pushed code milestone before this handoff:

`77ceedc17fa238317489a19f9a01f962c865ba43` — `Run minimal legacy decompiler loop`

Run #106 was reported RED and is not yet diagnosed. Do not infer its cause until the failed job log has been obtained exactly once.

## Current architecture

Verified runtime chain through #105:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> explicit procedure-size resolution -> real TDecompileEnv/TDecompiler preflight`

Verified real legacy decompiler sequence:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags()`

Important architecture boundaries:

- Neutral CFG stays independent of PE/image globals.
- `observedSpan` is observational only; never map it to legacy `procInfo->procSize`.
- Do not synthesize `ProcEnd`.
- Neutral prototype metadata deliberately excludes `procSize`.
- `IdrDecompilerInput.h` is the neutral decompiler-input builder; `IdrLegacyDecompilerInput.h` is only the active-session lookup adapter.
- Prototype resolution uses explicit `Resolved / Unavailable / Rejected` policy and never fabricates missing return or argument types.
- Procedure-size resolution uses `stored procSize -> explicit HeadlessProcedureSizeResolver -> unavailable/0`.
- Resolver-provided size is read-time only and is not persisted into `InfoRec`.
- `PortableEstimateProcSize()` no longer invents size from the next `ProcStart` or image remainder.
- `IdrLegacyDecompilerRunner` wraps the real legacy decompiler without exposing `TDecompiler` / `TDecompileEnv` in its public header.

## #106 — current red frontier

#106 attempts the first real full-loop legacy decompile on a deliberately minimal one-byte `RET` fixture with stored `procSize = 1`.

Intended sequence:

`TDecompileEnv -> TDecompiler -> Init() -> InitFlags() -> SetStop(StartAdr + Size) -> Decompile(StartAdr, 0, nullptr)`

The probe also installs a counting `manualInput` service and requires zero calls. It intends to prove that `Decompile()` returns, `WasRet` becomes true, and stored `procSize` remains unchanged.

Because #106 is red, NONE of those new full-loop assertions are runtime evidence yet.

## Exact next technical frontier

1. Discover push run #106 if connector capability permits.
2. Verify `run_number == 106` and `head_sha == 77ceedc17fa238317489a19f9a01f962c865ba43`.
3. Use the run's internal `id` with `fetch_workflow_run_jobs`.
4. If failed, fetch the failed job with `fetch_workflow_job_logs` exactly once.
5. Diagnose compiler/link/runtime failure only from that evidence.
6. Fix only the evidenced failure.
7. Re-run until the one-byte RET `Decompile()` path is green with zero `ManualInput()` calls.
8. Then capture full-loop result/state behind neutral portable result types before moving to richer fixtures.

Do not advance to broader decompiler behavior before #106-equivalent minimal full-loop execution is green.

## GitHub Actions push-run discovery

Primary route:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

Then:

- locate the requested `run_number`;
- verify `head_sha` against the pushed commit;
- take the internal run `id`;
- call `fetch_workflow_run_jobs`;
- only for a failed run, call `fetch_workflow_job_logs` for the failed job exactly once.

If the collection endpoint returns `endpoint not allowed` / HTTP 400:

1. run `api_tool.list_resources` for GitHub with query `workflow run`, `actions`, or `runs`;
2. check whether the session exposes a branch/push workflow-run listing function;
3. if no such function exists, state explicitly that this session's GitHub connector lacks push-run discovery;
4. do NOT use `fetch_commit_workflow_runs` as a substitute because it is PR-triggered-run limited;
5. once a `run_id` is otherwise known, `fetch_workflow_run_jobs` and `fetch_workflow_job_logs` remain valid downstream tools.

Do not ask the user for information GitHub can read when connector capability exists. Do not improvise a different discovery path merely because the established one is temporarily blocked.

## CI / logging discipline

- Never push over an active relevant workflow run.
- Green run: metadata/status only. Do not fetch logs.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed job log repeatedly.
- Analyze the one fetched log locally / from the returned result.
- Fetch again only for a new run or when explicitly requested.

Primary workflow:

`.github/workflows/portable-core-integration.yml`

The former separate legacy-decompiler-preflight workflow was removed in #105. The runner probe is now integrated into the primary workflow.

`docs/**` is currently under `paths-ignore`, so docs-only commits should not trigger the normal integration workflow.

## Git write discipline

All active-branch writes must use the low-level atomic sequence:

`create_blob -> create_tree -> create_commit -> update_ref`

Rules:

- verify branch HEAD immediately before writes;
- build the new tree from the verified current tree;
- use the verified current HEAD as commit parent;
- use non-forced fast-forward `update_ref`;
- do not intentionally use `create_file` / `update_file` for branch writes, even as probes;
- do not create PRs or merge to `main` yet.

## Legacy-source discipline

Do not modify original legacy source unless the change is a deliberate structural porting decision.

Prefer:

- neutral portable adapters;
- generated portable translation units;
- explicit service/policy seams;
- focused compatibility shims only for runtime-reached behavior.

Do not build broad fake-GUI/VCL behavior merely to silence compilation or interaction.

## Known risks

- Borland `String` indexing is 1-based while `std::string` is 0-based; audit runtime-reached direct indexing only.
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility remain transitional.
- Exact legacy flag-range end-boundary fidelity remains open.
- Neutral CFG still lacks indirect calls/jumps and jump-table/switch handling.
- Legacy decompiler routines can mutate locals, args, flags and `InfoRec`; future neutral result ownership must make this explicit.
- Known remaining `ManualInput()` families include imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.
- The first real `Decompile()` loop is implemented but not yet runtime-proven because #106 is red.

## Files to read first in every continuation

1. `docs/PORTABLE-CORE-STATE.md`
2. `docs/PORTING-NOTES.md`
3. `docs/AGENT-HANDOFF.md`
4. `docs/CHATGPT-PROJECT-INSTRUCTIONS.md`

Then verify `agent/portable-cli` HEAD directly from GitHub before any write.
