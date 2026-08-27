# IDR Portable Core - Technical Notes

Last updated: 2026-08-27

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`

Repository state is authoritative. Verify `agent/portable-cli` HEAD before every write.

## Current verified milestone

Current code HEAD before this docs-only update:

`0e69ef2ad308c75c2fce8753dc76eee213f83f80` — `Bypass KB for builtin integer types`

Run #117 is verified green. No green-run logs were fetched.

Recent relevant sequence:

- #109 `c49c327e58057a8cfd061ec3412bb625cffbb597` — stack-frame decompile — green.
- #110 `e1b0c43677eb288444706a0ea14976d58ca304be` — attempted legacy `DecompileProc()` source capture — link failure; failed log already fetched once.
- #111 `050753b9f6ec1e23339d71cd1e8da118f674b570` — neutral source wrapper over low-level legacy execution — green.
- #112 `0938c4b602c41fc865e04a2c3f125584a5d69fa9` — first direct function call — runtime AV; failed log already fetched once.
- #113 `de181eea8b5433e7d0c714ebca8358069188f31f` — direct-call stage tracing — runtime AV after `decompile-enter`; failed log already fetched once.
- #114 `22a27d2682b9a7cab7f50532a8b6aafafb03d1d2` — procedure isolation with incorrect seed completeness argument — controlled exit 39; failed log already fetched once.
- #115 `5d40cf0c076307b3de00deb5f8e18f3003d9a64e` — direct `ikProc` call — green.
- #116 `872fd434d6cb8e9ea28922146d3527bdc5375d49` — explicit `GetTypeKind("Integer")` diagnostic — runtime AV before decompile; failed log already fetched once.
- #117 `0e69ef2ad308c75c2fce8753dc76eee213f83f80` — generated builtin integer fast path — direct `ikFunc(Integer)` call green.

Never re-fetch failed logs for #106, #110, #112, #113, #114, or #116.

## Architecture milestone

The portable path is runtime-proven through:

`PE32 -> loaded legacy session -> real MDisasm/dis.dll -> neutral CFG -> Infos[] reconciliation -> neutral prototype/size policy -> TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result -> neutral source envelope`

The neutral source wrapper deliberately avoids GUI/presentation-owned `TDecompileEnv::DecompileProc()`. `DecompileActiveLegacyProcedureSource()` wraps the low-level result with portable `begin`/`end` lines and keeps legacy engine ownership private.

## Direct call evidence

The fixture uses caller at `0x00403000`:

`E8 0B 00 00 00; C3`

and callee at `+0x10`:

`B8 07 00 00 00; C3`

The rel32 target is correct: current + 5 + 0x0B = `+0x10`.

Both caller and callee records are explicitly seeded into the active `Infos[]` session, both have explicit `procSize = 6`, and both have explicit `ProcStart` flags.

Run #115 proves this path for `ikProc`.

Run #117 proves the same path for `ikFunc` returning `Integer`.

## #116 localization and #117 fix

#116 inserted an explicit `GetTypeKind("Integer", &size)` call immediately before decompile. Runtime markers reached `records-ready` but not the marker after `GetTypeKind`, and the process exited with Windows `0xC0000005`.

That proves the direct function crash was not in the E8 target, caller/callee record setup, `Decompile()`, or EAX return-result handling. It was inside legacy `GetTypeKind(String,int*)`.

Source inspection showed `GetTypeKind` checks RTTI and KnowledgeBase before reaching its later hard-coded builtin Integer branch. In the portable headless session, that lookup path is not a safe prerequisite for a type already known to be builtin.

The fix is confined to `tests/prepare_portable_misc_full.ps1`: generated `Misc.portable.cpp` receives an early builtin-integer classification path. Original `Misc.cpp` remains untouched. The existing `ikFunc(Integer)` probe was retained unchanged as the regression test and #117 went green.

## String compatibility

Generated legacy TUs currently translate several Embarcadero String APIs to portable helpers, including `Pos`, `SubString`, `LastDelimiter`, `Length`, `SetLength`, `IsEmpty`, trim/case helpers, and selected numeric String construction.

Direct `String[index]` remains dangerous because Borland String indexing is 1-based while `std::string` is 0-based. Do not globally rewrite it without evidence; patch generated copies only when a runtime-reached site proves necessary.

## Procedure-size invariant

`stored procSize -> session HeadlessProcedureSizeResolver -> unavailable/0`

No inference from next procedure, image remainder, or CFG `observedSpan`; no synthetic `ProcEnd`.

## Prototype / interaction policy

Headless metadata resolution stays explicit: `Resolved`, `Unavailable`, or `Rejected`. Do not fabricate return or argument types.

Known unresolved interactive families remain imports/return-byte questions, `@DispInvoke`, unknown function types, indirect calls, and some virtual/interface dispatch paths. Add only narrow policies when a real fixture reaches them.

## CI / Git discipline

Push-run discovery:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

Green: metadata only. Red: metadata -> jobs -> failed job log once. Never push over an active relevant run.

All branch writes use:

`create_blob -> create_tree -> create_commit -> update_ref`

Use current HEAD as parent and a non-forced fast-forward ref update.

## Immediate technical frontier

The basic direct-call family is now proven for both procedure and Integer-returning function. Advance incrementally into one more dimension at a time:

1. add one deterministic explicit argument to a direct callee, or exercise a different safe builtin return type;
2. keep `manualInput` counting at zero when complete metadata is supplied;
3. inspect results only through neutral execution/source envelopes;
4. stop at the first new runtime-reached dependency;
5. preserve original legacy source and prefer generated adapters;
6. after the next stable call/prototype milestone, resume controlled real Delphi Win32 fixture comparison and deterministic CLI output work.
