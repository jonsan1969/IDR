# Agent Handoff — IDR Portable CLI

Last updated: 2026-08-27

## Repository and branches

Repository: `jonsan1969/IDR`

Active branch: `agent/portable-cli`

Frozen / do not modify:

- `main`
- `agent/portable-core-integration`
- `agent/portable-core-smoke`

The repository is the source of truth. Never use old chat memory as current status.

## Current verified code HEAD

Immediately before this docs-only update:

`c49c327e58057a8cfd061ec3412bb625cffbb597` — `Exercise legacy stack frame decompile`

Always verify `agent/portable-cli` HEAD again before any future write.

## Latest verified green run

Run #109 is green:

- internal run id `33069570064`
- head SHA `c49c327e58057a8cfd061ec3412bb625cffbb597`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`

No green-run logs were fetched.

Recent green milestones:

- #107 `d479411c1e45ce13d2293de64ecfeb8ab207e3d4` — initialize real legacy disassembler before full decompile probe.
- #108 `1539163f8c90432699076a5ad0ff7eca1504390e` — expose neutral full-decompiler execution result.
- #109 `c49c327e58057a8cfd061ec3412bb625cffbb597` — exercise multi-instruction stack-frame decompile.

## #106 evidence — do not re-fetch its failed log

Run #106 (`77ceedc17fa238317489a19f9a01f962c865ba43`) failed at runtime with `0xC0000005` access violation while running the full legacy decompiler probe. Compiler and linker had succeeded.

The failed #106 job log has already been fetched exactly once. Never fetch it again.

Root cause: the focused probe reached `MDisasm::Disassemble()` without first calling `Disasm.Init()`, leaving the decoder backend unavailable. The normal CLI already performed this initialization. The fix was made in the portable probe, not original legacy source. Run #107 confirmed the diagnosis by going green.

## Current architecture

Verified runtime chain:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> explicit headless prototype resolution -> explicit procedure-size resolution -> TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result`

Important boundaries:

- Neutral CFG stays independent of PE/image globals.
- `observedSpan` is observational only and never becomes legacy `procInfo->procSize`.
- Never synthesize `ProcEnd`.
- Procedure size rule is `stored procSize -> explicit session HeadlessProcedureSizeResolver -> unavailable/0`.
- `IdrDecompilerInput.h` is the neutral decompiler read model; `IdrLegacyDecompilerInput.h` is only the legacy-session adapter.
- Prototype resolution is explicit and never fabricates missing return/argument types.
- `IdrLegacyDecompilerRunner` contains legacy engine classes; its public API exposes portable result types only.
- The neutral execution result is the boundary future CLI/output code should consume.

## Runtime-proven decompiler fixtures

Minimal fixture:

`C3` — `RET`

Proven green through full `Decompile()` with zero `ManualInput()` calls.

Richer fixture:

`55 8B EC 5D C3` — `PUSH EBP; MOV EBP,ESP; POP EBP; RET`

Run #109 green. This exercises multiple decoded instructions and stack/register state without calls/imports/interactivity.

## Exact next technical frontier

Starting from #109:

1. choose the smallest deterministic x86 procedure that produces meaningful decompiler body output rather than only frame/return state;
2. exercise it through `DecompileActiveLegacyProcedure()` and inspect only the neutral result;
3. keep a counting `manualInput` service installed and require zero calls;
4. if green, advance incrementally;
5. at the first runtime-reached unresolved interactive dependency, stop and add a narrow explicit headless policy for that family only;
6. do not expose legacy GUI/list/decompiler classes to portable callers.

Do not jump ahead to broad interaction emulation or rewrite original legacy decompiler behavior without runtime evidence.

## GitHub Actions push-run discovery

Primary route:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

This endpoint is currently working.

Then:

- locate the requested `run_number`;
- verify `head_sha`;
- take internal run `id`;
- green: metadata only, no logs;
- red: `fetch_workflow_run_jobs` -> failed job -> `fetch_workflow_job_logs` exactly once.

If the collection endpoint becomes blocked:

1. capability-check GitHub tools for `workflow run`, `actions`, or `runs`;
2. if no branch/push run-list function exists, state the connector gap explicitly;
3. do not use `fetch_commit_workflow_runs` because it is PR-run limited.

## CI / log discipline

- Never push over an active relevant run.
- Green run: metadata/status only.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed job log repeatedly.

Primary workflow:

`.github/workflows/portable-core-integration.yml`

`docs/**` is under `paths-ignore`; docs-only commits should not start the normal integration run while that workflow setting remains unchanged.

## Atomic Git write discipline

All active-branch writes:

`create_blob -> create_tree -> create_commit -> update_ref`

Rules:

- verify branch HEAD immediately before writes;
- verify no active relevant workflow;
- build from the current tree;
- current HEAD is the commit parent;
- non-forced fast-forward `update_ref` only;
- do not intentionally use `create_file` / `update_file` for branch writes;
- no PR/merge to `main` yet.

## Legacy-source discipline

Do not modify original legacy source unless it is a conscious structural porting decision.

Prefer portable adapters, generated portable copies, explicit policy/service seams and runtime-reached compatibility fixes.

## Known risks

- Borland `String` indexing (1-based) versus `std::string` (0-based).
- Transitional `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility.
- Exact legacy flag-range boundary fidelity.
- Indirect calls/jumps and jump-table/switch CFG behavior.
- Legacy decompiler mutation of locals/args/flags/`InfoRec`.
- Remaining `ManualInput()` families: import return-byte questions, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.

## Files to read first in every continuation

1. `docs/PORTABLE-CORE-STATE.md`
2. `docs/PORTING-NOTES.md`
3. `docs/AGENT-HANDOFF.md`
4. `docs/CHATGPT-PROJECT-INSTRUCTIONS.md`

Then verify `agent/portable-cli` HEAD and relevant Actions state directly from GitHub before any write.
