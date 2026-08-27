# Portable Core Project State

Last updated: 2026-08-27

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR`.
Active branch: `agent/portable-cli`.
Frozen / do not modify: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`.

The repository is authoritative. Verify branch HEAD directly from GitHub before every write.

## Current verified status

Current code HEAD before this docs-only update:

`0e69ef2ad308c75c2fce8753dc76eee213f83f80` — `Bypass KB for builtin integer types`

Latest verified green integration run:

- run #117
- internal run id `33075436583`
- head SHA `0e69ef2ad308c75c2fce8753dc76eee213f83f80`
- workflow `Portable CLI integration`
- status `completed`
- conclusion `success`

No #117 logs were fetched because it was green.

## Current runtime architecture

Verified chain:

`PE32 -> IdrPeLoader -> authoritative loaded session -> MDisasm/dis.dll -> bounded neutral CFG -> procedure summaries/xrefs -> legacy Infos[] reconciliation -> neutral decompiler input -> headless prototype resolution -> explicit procedure-size resolution -> real TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result -> neutral procedure-source envelope`

The public portable boundary remains neutral; legacy `TDecompiler`, `TDecompileEnv` and `TStringList` ownership stay inside adapters/generated integration code.

## Runtime-proven decompiler fixtures

- `C3` — `RET` — full legacy `Decompile()` green.
- `55 8B EC 5D C3` — classic stack frame — green, zero `ManualInput()`.
- Direct `E8` call to an `ikProc` callee — green in #115.
- Direct `E8` call to an `ikFunc` callee returning `Integer` — green in #117.

The direct-call fixture uses explicit caller/callee `InfoRec/procInfo`, explicit `procSize`, explicit `ProcStart`, and headless services. No size is inferred from CFG span.

## Direct-call failure sequence and resolution

- #112 (`0938c4b602c41fc865e04a2c3f125584a5d69fa9`) failed with `0xC0000005` in the new direct function-call path. Failed job log already fetched exactly once; never fetch it again.
- #113 (`de181eea8b5433e7d0c714ebca8358069188f31f`) added flushed stage markers. It proved activation, metadata seeding and record setup succeeded; crash occurred after `decompile-enter`. Failed log already fetched exactly once; never fetch it again.
- #114 (`22a27d2682b9a7cab7f50532a8b6aafafb03d1d2`) isolated an `ikProc` callee but exposed a probe seed-completeness mistake and exited 39 instead of crashing. Failed log already fetched exactly once; never fetch it again.
- #115 (`5d40cf0c076307b3de00deb5f8e18f3003d9a64e`) corrected that fixture mistake and proved direct procedure calls green.
- #116 (`872fd434d6cb8e9ea28922146d3527bdc5375d49`) restored `ikFunc(Integer)` and called `GetTypeKind("Integer")` explicitly before decompile. It crashed after `records-ready` and before the return-type marker, proving the access violation was inside `GetTypeKind(String,int*)`, before `Decompile()`. Failed log already fetched exactly once; never fetch it again.
- Source inspection showed `GetTypeKind` performs RTTI/KnowledgeBase lookup before its later hard-coded builtin Integer branch. In the portable headless path that lookup was not safe for this builtin.
- #117 (`0e69ef2ad308c75c2fce8753dc76eee213f83f80`) added a generated-only fast path for builtin integer types in `tests/prepare_portable_misc_full.ps1`. Original `Misc.cpp` remains untouched. The same `ikFunc(Integer)` direct-call probe is now green.

## Procedure-size invariant

Current rule:

`stored procSize -> session HeadlessProcedureSizeResolver -> unavailable/0`

Never infer size from next `ProcStart`, image remainder, or CFG `observedSpan`. Never synthesize `ProcEnd`. Resolver output remains read-time unless a deliberate persistence design is introduced.

## Compatibility strategy

Preserve original legacy source unless a deliberate structural port is required. Prefer:

- portable adapters;
- generated portable copies;
- narrow runtime-reached compatibility shims;
- explicit headless policy/service seams.

The generated Misc path already ports Borland `Pos`, `SubString`, `LastDelimiter`, `Length`, trimming and related APIs. Direct Borland 1-based indexing remains a known compatibility risk and should only be changed when runtime evidence reaches a specific site.

## Known risks

- Borland `String` 1-based indexing versus `std::string` 0-based indexing.
- Transitional `WideString`, `Variant`, `Currency`, `Comp` and formatting behavior.
- Exact legacy flag-range boundary fidelity.
- Indirect calls/jumps and jump-table/switch handling.
- Legacy decompiler mutation of locals/args/flags/`InfoRec`.
- Remaining `ManualInput()` families: import return-byte questions, `@DispInvoke`, unknown function types, indirect calls and some virtual/interface dispatch cases.
- Type resolution that genuinely requires RTTI/KnowledgeBase still needs evidence-driven hardening; #117 only bypasses KB for already-known builtin integer types.

## CI discipline

Primary push-run discovery:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

- Never push over an active relevant run.
- Green run: metadata/status only; no log.
- Red run: metadata -> jobs -> failed job log exactly once.
- Never fetch the same failed log twice.
- If branch push-run discovery is unavailable, capability-check connector tools and state the gap explicitly. Do not substitute PR-limited `fetch_commit_workflow_runs`.

## Git write discipline

All active-branch writes:

`create_blob -> create_tree -> create_commit -> update_ref`

Verify HEAD and relevant run state immediately before writing. Use non-forced fast-forward ref updates.

## Immediate next phase

The direct procedure/function call baseline is now green. Next frontier:

1. exercise the next smallest real call-family that introduces prototype/argument behavior without broad GUI emulation;
2. keep `manualInput` counting enabled and require zero calls where metadata is complete;
3. prefer a deterministic direct function with one explicit argument or another builtin return type before moving to imports/indirect dispatch;
4. stop at the first runtime-reached unresolved dependency and model only that family;
5. keep consuming output through neutral result/source envelopes;
6. later expose deterministic CLI decompiler output and publish `idr-cli.exe + dis.dll` artifacts.
