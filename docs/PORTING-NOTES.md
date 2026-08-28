# IDR Portable Core - Technical Notes

Last updated: 2026-08-28

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`

Repository state is authoritative. Verify `agent/portable-cli` HEAD before every write.

## Current verified milestone

Current code HEAD before this docs-only update:

`8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a` — `Exercise two register arguments`

Run #120 is verified green. No green-run jobs or logs were fetched.

Recent relevant sequence:

- #115 `5d40cf0c076307b3de00deb5f8e18f3003d9a64e` — direct `ikProc` call — green.
- #116 `872fd434d6cb8e9ea28922146d3527bdc5375d49` — explicit `GetTypeKind("Integer")` diagnostic — runtime AV before decompile; failed log already fetched once.
- #117 `0e69ef2ad308c75c2fce8753dc76eee213f83f80` — generated builtin integer fast path — direct `ikFunc(Integer)` call green.
- #119 `55dde09d002fbc08ba06d9bc5b8d131feab85146` — one explicit value argument in EAX with complete neutral/legacy prototype round-trip — green.
- #120 `8bd65445cd69a4b73dee73e6fbf7711bb7b3d18a` — two explicit register arguments in EAX + ECX, retaining the one-argument regression — green.

Never re-fetch failed logs for #106, #110, #112, #113, #114, or #116.

## Architecture milestone

The portable path is runtime-proven through:

`PE32 -> loaded legacy session -> real MDisasm/dis.dll -> neutral CFG -> Infos[] reconciliation -> neutral prototype/size policy -> TDecompileEnv/TDecompiler -> full Decompile() -> neutral execution result -> neutral source envelope`

The neutral source wrapper deliberately avoids GUI/presentation-owned `TDecompileEnv::DecompileProc()`. `DecompileActiveLegacyProcedureSource()` wraps the low-level result with portable `begin`/`end` lines and keeps legacy engine ownership private.

## Direct-call / argument evidence

The original direct-call fixture proved procedure and Integer-returning function calls. #119 then added a deterministic caller that loads EAX before a direct E8 call and supplies one complete argument descriptor:

- tag `0x21` (`val`);
- register argument;
- register index `0` (EAX);
- size `4`;
- type `Integer`.

Metadata is captured back through `CaptureLegacyProcedurePrototypeMetadata()` and checked exactly before decompile. #119 went green with zero `ManualInput()`.

#120 keeps that regression and adds the next register argument, index `1` (ECX), with the same explicit Integer metadata. The full legacy decompiler path remains green. This proves that the portable metadata bridge is not limited to a single register argument.

No stack-argument or new calling-convention behavior has been introduced yet.

## #116 localization and #117 fix

#116 proved the AV was inside legacy `GetTypeKind(String,int*)`, before `Decompile()`. Source inspection showed RTTI/KnowledgeBase lookup happens before the later hard-coded builtin Integer branch.

The fix remains confined to `tests/prepare_portable_misc_full.ps1`: generated `Misc.portable.cpp` receives an early builtin-integer classification path. Original `Misc.cpp` remains untouched.

## Current CLI gap

The real `idr-cli.exe` already compiles, links and runs against `tests/idr_cli_fixture.cpp`. It performs PE32 loading, real disassembly, bounded CFG analysis, procedure discovery/reconciliation and neutral `ProcedureDecompileInput` construction.

The main CLI still does not link/use `IdrLegacyDecompilerRunner` to emit real decompiler source. The focused runner probe already proves that legacy full `Decompile()` can be consumed through the neutral source envelope.

The remaining integration must not use CFG `observedSpan` as authoritative procedure size.

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

The synthetic register-argument family has reached a stable milestone. Resume controlled real PE32/CLI work:

1. establish an explicit authoritative procedure-size source for one procedure in the existing real fixture, without promoting `observedSpan` to `procSize`;
2. link `IdrLegacyDecompilerRunner` into `idr-cli.exe` only when that size contract is satisfied;
3. decompile exactly one real analyzed procedure and print deterministic neutral source output;
4. require zero unexpected `ManualInput()` and fail explicitly when metadata is unavailable;
5. compare the resulting output against the controlled fixture;
6. then broaden the CLI one call/prototype family at a time and prepare publishable artifacts.
