# Agent Handoff — IDR Portable CLI

Last updated: 2026-08-28

## Repository and branches

Repository: `jonsan1969/IDR`
Active branch: `agent/portable-cli`
Frozen / do not modify: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`.

The repository is the source of truth. Never use old chat memory as current status.

## Current verified code HEAD

Immediately before this docs-only update:

`8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a` — `Exercise two register arguments`

Always verify `agent/portable-cli` HEAD again before future writes.

## Latest verified green run

Run #120:

- internal id `33104933853`
- head SHA `8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`

No #120 jobs or logs were fetched.

## Current architecture

Verified runtime chain:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> explicit prototype policy -> explicit procedure-size policy -> real TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result -> neutral source envelope`

Important invariants:

- `observedSpan` is not legacy `procInfo->procSize`.
- Never synthesize `ProcEnd`.
- Size rule: `stored procSize -> explicit HeadlessProcedureSizeResolver -> unavailable/0`.
- Prototype resolution never fabricates missing return/argument types.
- Legacy engine classes stay behind `IdrLegacyDecompilerRunner`.
- The source wrapper does not call GUI/presentation-owned `TDecompileEnv::DecompileProc()`.

## Runtime-proven fixtures

- one-byte `RET` full decompile;
- classic stack frame `55 8B EC 5D C3`;
- direct E8 call to `ikProc` callee — #115 green;
- direct E8 call to `ikFunc` callee returning `Integer` — #117 green;
- direct Integer-returning call with one explicit EAX value argument — #119 green;
- direct Integer-returning call with two explicit register arguments EAX + ECX — #120 green.

Call/prototype metadata is explicit and round-tripped through the neutral/legacy adapter. Complete metadata is required to keep `ManualInput()` at zero. No procedure size is inferred from CFG span.

## Relevant diagnosis history

- #112 direct function call: runtime `0xC0000005`; failed log fetched exactly once.
- #113 stage tracing: crash after `decompile-enter`; failed log fetched exactly once.
- #114 fixture seed mistake: controlled exit 39; failed log fetched exactly once.
- #115 direct procedure call green.
- #116 `GetTypeKind("Integer")` localized AV before decompile; failed log fetched exactly once.
- #117 generated-only builtin Integer classification bypassed unsafe RTTI/KB lookup for already-known builtin integers; original `Misc.cpp` untouched.
- #119 one register argument green.
- #120 two register arguments green.

Never fetch failed logs again for #106, #110, #112, #113, #114, or #116.

## Exact next technical frontier

Stop expanding synthetic register-argument coverage for now. The next frontier is the real CLI boundary:

1. use the existing real PE32 CLI fixture as the controlled integration target;
2. identify an explicit authoritative procedure-size source for exactly one real analyzed procedure, without using CFG `observedSpan` as `procSize`;
3. link/use `IdrLegacyDecompilerRunner` from `idr-cli.exe` only for procedures whose size contract is satisfied;
4. call `DecompileActiveLegacyProcedureSource()` for one controlled procedure;
5. require deterministic source-envelope output and zero unexpected `ManualInput()`;
6. fail/report unsupported procedures explicitly instead of fabricating metadata;
7. once green, extend deterministic CLI output and prepare `idr-cli.exe + dis.dll` artifacts for external guinea-pig testing.

Do not combine this first CLI bridge with imports, indirect dispatch, stack arguments or broad calling-convention work.

## Known risks

- authoritative procedure-size discovery for real PE32 procedures;
- Borland String 1-based indexing versus `std::string` 0-based indexing;
- genuine RTTI/KnowledgeBase-dependent type resolution beyond known builtin integers;
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility;
- exact flag-range fidelity;
- indirect calls/jumps and jump-table/switch behavior;
- decompiler mutation of locals/args/flags/`InfoRec`;
- remaining interactive families: imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls, virtual/interface dispatch.

## GitHub Actions discovery

Primary route:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

Green: metadata only, no jobs/logs.
Red: metadata -> `fetch_workflow_run_jobs` -> failed job -> `fetch_workflow_job_logs` exactly once.

If the collection endpoint is blocked, capability-check GitHub tools for `workflow run`, `actions`, or `runs`. If no branch/push run listing exists, state the connector gap explicitly. Never substitute PR-limited `fetch_commit_workflow_runs`.

## CI / log discipline

- Never push over an active relevant workflow run.
- Never re-fetch a failed log already consumed.
- `docs/**` is under `paths-ignore`; docs-only commits should not start normal integration while that remains true.

## Atomic Git write discipline

All active-branch writes:

`create_blob -> create_tree -> create_commit -> update_ref`

Before writing: verify current branch HEAD and no active relevant run. Use current HEAD as parent and non-forced fast-forward `update_ref`.

## Legacy-source discipline

Do not modify original legacy source unless making a conscious structural porting decision. Prefer adapters, generated portable copies, explicit policy/service seams, and runtime-reached compatibility fixes.

## Files to read first in every continuation

1. `docs/PORTABLE-CORE-STATE.md`
2. `docs/PORTING-NOTES.md`
3. `docs/AGENT-HANDOFF.md`
4. `docs/CHATGPT-PROJECT-INSTRUCTIONS.md`

Then verify `agent/portable-cli` HEAD and relevant Actions state directly from GitHub before any write.
