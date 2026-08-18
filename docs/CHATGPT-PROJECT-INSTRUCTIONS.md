# ChatGPT Project Instructions — IDR Portable CLI

## Source of truth

The repository is authoritative. Do not treat old chat memory, summaries, or handoff prose as current project state when they conflict with GitHub.

Active repository/branch:

- `jonsan1969/IDR`
- `agent/portable-cli`

Frozen / do not modify:

- `main`
- `agent/portable-core-integration`
- `agent/portable-core-smoke`

## Always start here

Before making changes:

1. read `docs/PORTABLE-CORE-STATE.md`;
2. read `docs/PORTING-NOTES.md`;
3. read `docs/AGENT-HANDOFF.md`;
4. verify the current `agent/portable-cli` HEAD directly from GitHub;
5. use that verified HEAD as the parent for any write.

## Working style

- Compiler/linker/runtime evidence before speculation.
- One coherent architecture change per commit.
- Continue automatically after a green run when the next step is clear.
- Do not ask the user for information that GitHub/connected tools can read.
- Do not invent a new workflow or methodology when an established project path already exists.
- If a connector capability is missing or blocked, say so explicitly instead of guessing around it.
- Preserve original legacy source unless a deliberate structural porting decision requires otherwise; prefer adapters and generated portable copies.

## CI discipline

- Never push while a relevant workflow run is active.
- Green run: metadata/status only; do not fetch logs.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed log repeatedly.

Preferred push-run discovery for `agent/portable-cli`:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

Select the expected `run_number`, verify `head_sha`, then use its internal `id` with `fetch_workflow_run_jobs`. Only if failed, fetch the failed job log once with `fetch_workflow_job_logs`.

If `/actions/runs` is blocked with `endpoint not allowed`, check connector capabilities with `api_tool.list_resources` using `workflow run`, `actions`, or `runs`. If no push/branch run-list tool exists, state explicitly that the session lacks run-discovery. Do not substitute `fetch_commit_workflow_runs`; it is PR-run limited.

## Git write discipline

Writes to `agent/portable-cli` must use:

`create_blob -> create_tree -> create_commit -> update_ref`

Verify HEAD first and use non-forced fast-forward `update_ref`.

Do not intentionally use `create_file` or `update_file` for branch writes or probing.

## Important invariants

- `observedSpan` is not legacy `procInfo->procSize`.
- Do not synthesize `ProcEnd`.
- Procedure size must come from stored legacy metadata or an explicit headless size resolver.
- Resolver-provided metadata/size is read-time unless a deliberate persistence step is designed.
- Replace runtime-reached `ManualInput()` families with narrow explicit headless policies, not a generic fake-GUI shim.

## Current frontier

The exact current technical frontier is maintained in `docs/AGENT-HANDOFF.md` and the two main state docs. Read them rather than relying on this file for milestone numbers.
