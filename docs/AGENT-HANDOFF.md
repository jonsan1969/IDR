# Agent Handoff — IDR Portable CLI

Last updated: 2026-08-28

## Repository and branches

Repository: `jonsan1969/IDR`
Active branch: `agent/portable-cli`
Frozen / do not modify: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`.

The repository is the source of truth. Never use old chat memory as current status.

## Current verified code HEAD

Immediately before this docs-only update:

`7c96a13f69317f0a2bbbef2f5149e574a32f4ce4` — `Publish portable CLI artifact`

Always verify `agent/portable-cli` HEAD again before future writes.

## Latest verified green run

Run #122:

- internal id `33143496591`
- head SHA `7c96a13f69317f0a2bbbef2f5149e574a32f4ce4`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`
- artifact `idr-portable-cli-win32` built successfully with `idr-cli.exe` + `dis.dll`

No #122 jobs or logs were fetched because the run was green.

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

## Runtime-proven fixtures and CLI bridge

- one-byte `RET` full decompile;
- classic stack frame `55 8B EC 5D C3`;
- direct E8 call to `ikProc` callee — #115 green;
- direct E8 call to `ikFunc` callee returning `Integer` — #117 green;
- direct Integer-returning call with one explicit EAX value argument — #119 green;
- direct Integer-returning call with two explicit register arguments EAX + ECX — #120 green;
- real PE32 fixture entry decompiled through the actual `idr-cli.exe` with explicit `--entry-size 12` — #121 green, zero `ManualInput()` and deterministic `begin`/`end` source envelope;
- publishable Windows artifact containing `idr-cli.exe` + `dis.dll` — #122 green.

Call/prototype metadata remains explicit and complete metadata is required to keep `ManualInput()` at zero. No procedure size is inferred from CFG span.

## Exact next technical frontier

The first external-testable artifact exists. Next work should use that milestone rather than returning to synthetic argument coverage:

1. exercise the artifact against a controlled real Delphi guinea-pig binary;
2. capture compiler/linker/runtime evidence for the first unsupported or incorrect behavior;
3. keep procedure-size resolution explicit — do not promote CFG `observedSpan` to `procSize`;
4. add only the narrow metadata/interaction policy reached by that real binary;
5. preserve deterministic CLI output and zero unexpected interactive prompts;
6. broaden imports, indirect dispatch, stack/calling-convention families and richer Delphi cases only from observed runtime evidence.

## Known risks

- authoritative procedure-size discovery for arbitrary real PE32 procedures;
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
- Node-based GitHub Actions/CI tooling must use Node.js 24 or higher. Do not introduce or retain action revisions that declare Node.js 20 when a Node.js 24+-compatible revision exists. Treat Node 20 deprecation annotations as migration work to remove.

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
