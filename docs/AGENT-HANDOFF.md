# Agent Handoff — IDR Portable CLI

Last updated: 2026-08-27

## Repository and branches

Repository: `jonsan1969/IDR`
Active branch: `agent/portable-cli`
Frozen / do not modify: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`.

The repository is the source of truth. Never use old chat memory as current status.

## Current verified code HEAD

Immediately before this docs-only update:

`0e69ef2ad308c75c2fce8753dc76eee213f83f80` — `Bypass KB for builtin integer types`

Always verify `agent/portable-cli` HEAD again before future writes.

## Latest verified green run

Run #117:

- internal id `33075436583`
- head SHA `0e69ef2ad308c75c2fce8753dc76eee213f83f80`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`

No #117 logs were fetched.

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
- direct E8 call to `ikFunc` callee returning `Integer` — #117 green.

All direct-call metadata is explicit: caller/callee `InfoRec/procInfo`, `procSize`, and `ProcStart` are prepopulated; no size inference is used.

## Direct-call diagnosis history

- #112 direct function call: runtime `0xC0000005`; failed log fetched exactly once.
- #113 stage tracing: activation, session, metadata and records all passed; crash after `decompile-enter`; failed log fetched exactly once.
- #114 procedure isolation accidentally used the wrong `functionKind` argument to `BuildLegacyProcedureMetadataSeed`, causing controlled exit 39; failed log fetched exactly once.
- #115 corrected fixture: direct procedure call green.
- #116 restored `ikFunc(Integer)` and explicitly called `GetTypeKind("Integer")` before decompile. Runtime reached `records-ready` but not the next marker, proving the AV was inside `GetTypeKind(String,int*)`; failed log fetched exactly once.
- #117 fixed the portable generated Misc path by classifying already-known builtin integer types before RTTI/KnowledgeBase lookup. Original `Misc.cpp` was not modified. Same direct function probe then went green.

Never fetch failed logs again for #106, #110, #112, #113, #114, or #116.

## Exact next technical frontier

Advance one dimension beyond the now-green direct-call baseline. Preferred next experiment:

1. direct callee with one explicit argument and complete neutral/legacy prototype metadata;
2. keep the call target and return type deterministic (Integer is now proven safe);
3. require zero `ManualInput()` calls;
4. inspect only neutral execution/source results;
5. if a new runtime dependency appears, localize it with flushed probe markers before changing architecture;
6. prefer generated adapters/policies over modifying original legacy source.

A different simple builtin return type is a reasonable alternate frontier, but do not combine multiple new dimensions in one commit.

## Known risks

- Borland String 1-based indexing versus `std::string` 0-based indexing.
- Genuine RTTI/KnowledgeBase-dependent type resolution is not broadly hardened; #117 only bypasses KB for already-known builtin integers.
- Transitional `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility.
- Exact flag-range fidelity.
- Indirect calls/jumps and jump-table/switch behavior.
- Decompiler mutation of locals/args/flags/`InfoRec`.
- Remaining interactive families: imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls, virtual/interface dispatch.

## GitHub Actions discovery

Primary route:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

Green: metadata only, no logs.
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
