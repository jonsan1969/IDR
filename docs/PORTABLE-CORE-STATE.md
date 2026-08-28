# Portable Core Project State

Last updated: 2026-08-28

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR`.
Active branch: `agent/portable-cli`.
Frozen / do not modify: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`.

The repository is authoritative. Verify branch HEAD directly from GitHub before every write.

## Current verified status

Current code HEAD before this docs-only update:

`8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a` — `Exercise two register arguments`

Latest verified green integration run:

- run #120
- internal run id `33104933853`
- head SHA `8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`

No #120 jobs or logs were fetched because it was green.

## Current runtime architecture

Verified chain:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> explicit procedure-size resolution -> real TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result -> neutral procedure-source envelope`

The public portable boundary remains neutral; legacy engine ownership stays behind adapters/generated integration code.

## Runtime-proven decompiler fixtures

- `C3` — `RET` — full legacy `Decompile()` green.
- `55 8B EC 5D C3` — classic stack frame — green, zero `ManualInput()`.
- Direct `E8` call to an `ikProc` callee — green in #115.
- Direct `E8` call to an `ikFunc` callee returning `Integer` — green in #117.
- Direct Integer-returning callee with one explicit register argument in EAX — green in #119.
- Direct Integer-returning callee with two explicit register arguments in EAX + ECX — green in #120.

The call fixtures use explicit caller/callee `InfoRec/procInfo`, explicit `procSize`, explicit `ProcStart`, complete neutral/legacy prototype metadata, and zero `ManualInput()` where metadata is complete. No size is inferred from CFG span.

## Important resolved dependency

#116 localized a portable-runtime AV to legacy `GetTypeKind("Integer")` performing RTTI/KnowledgeBase lookup before its later builtin Integer branch. #117 fixed only the generated portable Misc path by classifying already-known builtin integer types first. Original `Misc.cpp` remains untouched.

## Procedure-size invariant

Current rule:

`stored procSize -> session HeadlessProcedureSizeResolver -> unavailable/0`

Never infer size from next `ProcStart`, image remainder, or CFG `observedSpan`. Never synthesize `ProcEnd`. Resolver output remains read-time unless a deliberate persistence design is introduced.

## Current CLI boundary

`idr-cli.exe` already builds and runs on a real PE32 fixture. It currently performs:

`PE32 load -> disassembly -> bounded CFG -> procedure discovery -> Infos[] reconciliation -> neutral decompiler-input construction`

The main CLI does not yet invoke `IdrLegacyDecompilerRunner` / `DecompileActiveLegacyProcedureSource()` for real analyzed procedures. The integrated decompiler runner is proven separately by the focused runtime probe.

Do not bridge that gap by treating CFG `observedSpan` as authoritative `procSize`.

## Compatibility strategy

Preserve original legacy source unless a deliberate structural port is required. Prefer portable adapters, generated portable copies, narrow runtime-reached compatibility shims, and explicit headless policy/service seams.

## Known risks

- authoritative procedure-size discovery for real PE32 procedures;
- Borland `String` 1-based indexing versus `std::string` 0-based indexing;
- genuine RTTI/KnowledgeBase-dependent type resolution beyond known builtin integers;
- `WideString`, `Variant`, `Currency`, `Comp` and formatting behavior;
- indirect calls/jumps, jump tables and switch handling;
- decompiler mutation of locals/args/flags/`InfoRec`;
- remaining interactive families: imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls and virtual/interface dispatch.

## CI discipline

Primary push-run discovery:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

- Never push over an active relevant run.
- Green run: metadata/status only; no job/log fetch.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed log twice.
- If branch push-run discovery is unavailable, capability-check connector tools and state the gap explicitly. Do not substitute PR-limited `fetch_commit_workflow_runs`.

## Git write discipline

All active-branch writes:

`create_blob -> create_tree -> create_commit -> update_ref`

Verify HEAD and relevant run state immediately before writing. Use non-forced fast-forward ref updates.

## Immediate next phase

The synthetic direct-call/prototype baseline is now strong enough. Resume real CLI integration:

1. identify a controlled authoritative procedure-size source for the existing real PE32 fixture without using CFG `observedSpan` as `procSize`;
2. wire exactly one real analyzed procedure through `IdrLegacyDecompilerRunner` / `DecompileActiveLegacyProcedureSource()`;
3. require deterministic source-envelope output and zero unexpected `ManualInput()` calls;
4. keep unsupported procedures explicit rather than fabricating metadata;
5. then extend to deterministic CLI decompiler output and publishable `idr-cli.exe + dis.dll` artifacts;
6. only after that broaden into imports, indirect dispatch, stack/calling-convention families and richer real Delphi fixtures.
