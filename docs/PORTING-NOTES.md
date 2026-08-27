# IDR Portable Core - Technical Notes

Last updated: 2026-08-27

Technical journal for the MSVC/GitHub-hosted portable-core and CLI work.

## Repository / branches

- Working fork: `jonsan1969/IDR`
- Active product branch: `agent/portable-cli`
- Frozen: `main`, `agent/portable-core-integration`, `agent/portable-core-smoke`

Repository state is authoritative. Verify `agent/portable-cli` HEAD before every write.

## Current verified milestone

Current code HEAD before this docs update:

`c49c327e58057a8cfd061ec3412bb625cffbb597` — `Exercise legacy stack frame decompile`

Run #109 is verified green. No green-run logs were fetched.

Recent sequence:

- #106 `77ceedc17fa238317489a19f9a01f962c865ba43` — first minimal full `Decompile()` attempt — runtime failure.
- #107 `d479411c1e45ce13d2293de64ecfeb8ab207e3d4` — initialize real legacy disassembler in focused probe — green.
- #108 `1539163f8c90432699076a5ad0ff7eca1504390e` — neutralize full decompiler execution result — green.
- #109 `c49c327e58057a8cfd061ec3412bb625cffbb597` — exercise classic stack-frame procedure — green.

## #106 failure evidence

Run #106 failed only when the newly linked `idr-legacy-decompiler-runner-probe.exe` executed. Compiler and linker stages had succeeded. The process exited with `-1073741819`, Windows exception `0xC0000005` (access violation).

The #106 failed job log was fetched exactly once and must not be fetched again.

Source inspection then established the runtime prerequisite: `TDecompiler::Decompile()` immediately reaches `MDisasm::Disassemble()`, whose backend pointer is established by `Disasm.Init()`. The real CLI initialized that chain; the isolated probe did not.

The fix was intentionally placed in the portable probe rather than original legacy source. Run #107 proved the diagnosis correct.

## Full legacy Decompile() path now proven

The real MSVC x86 path is runtime-proven through:

`TDecompileEnv -> TDecompiler -> Init(address) -> InitFlags() -> SetStop(address + size) -> Decompile(address, 0, ...)`

The one-byte `RET` fixture completes without `ManualInput()` calls and preserves the explicit procedure-size policy.

This moves the project beyond preflight into actual legacy decompiler execution.

## Neutral full-decompile result boundary

Run #108 adds a portable result boundary over the legacy execution.

The adapter captures ordinary portable values such as:

- procedure address;
- resolved procedure size and `ProcedureSizeSource`;
- end address;
- RET/completion status;
- decompiler body lines as standard strings.

Legacy ownership (`TDecompileEnv`, `TDecompiler`, `TStringList`) stays inside the adapter implementation. The public portable header does not expose legacy engine classes.

This boundary is where future CLI/output work should consume decompiler results.

## Richer no-interaction fixture — #109

The next fixture is the canonical five-byte x86 frame:

`55 8B EC 5D C3`

`PUSH EBP; MOV EBP,ESP; POP EBP; RET`

It exercises multiple decoder passes plus stack/register state transitions while deliberately avoiding direct/indirect calls, imports and prototype completion questions.

Run #109 is green, so this richer path is runtime evidence rather than a source-only expectation.

## PE32 / decoder / CFG path

`IdrPeLoader` validates PE32/x86, maps sections into analysis-facing bytes, preserves absolute entry-point semantics and binds the image to the active legacy session.

The real CLI initializes `MDisasm` against the shipped x86 `dis.dll` and adapts `DISINFO` into neutral decoded instruction facts.

Neutral CFG remains decoder/address-mapper injected and independent of PE globals. It supports bounded worklists, direct calls, direct conditional/unconditional branches, explicit edge kinds, call xrefs, procedure ownership/summaries and observed spans.

`observedSpan` is observational only. It is never a substitute for legacy `procInfo->procSize`, and no synthetic `ProcEnd` is created.

## Procedure metadata / Infos[] reconciliation

Neutral prototype metadata covers kind, return type, flags, BP base, ret bytes, stack size, arguments and locals; `procSize` is deliberately excluded.

The bridge supports neutral -> legacy seeding and read-only legacy -> neutral capture. CFG procedures reconcile into active `Infos[]` idempotently: valid procedure records are reused, empty slots can be materialized, occupied non-procedure slots fail, writes are preflighted/rolled back, and no size is inferred from CFG span.

## Prototype policy

`IdrDecompilerInput.h` defines the neutral decompiler-facing read model. `IdrLegacyDecompilerInput.h` is a thin active-session lookup adapter.

Headless prototype resolution remains explicit: `Resolved`, `Unavailable`, or `Rejected`. No return type or argument type is fabricated automatically.

Only runtime-reached unresolved families should gain new policies.

## Procedure-size invariant

Current rule:

`stored procSize -> session HeadlessProcedureSizeResolver -> unavailable/0`

Forbidden implicit fallbacks:

- next `ProcStart`;
- image remainder;
- CFG `observedSpan`.

Resolver output is read-time only and is not persisted into `InfoRec`.

## ManualInput inventory

Known remaining interactive families include:

- import return-byte questions;
- `@DispInvoke`;
- unknown function types;
- indirect calls;
- some virtual/interface dispatch cases.

Do not implement a generic fake dialog system. When a fixture actually reaches one of these paths, add only the narrow explicit headless policy required by that evidence.

## GitHub Actions discovery

Preferred push-run discovery:

`GitHub.fetch("https://api.github.com/repos/jonsan1969/IDR/actions/runs?branch=agent%2Fportable-cli&event=push&per_page=20")`

This route is working in the current session.

Discipline:

1. identify run number and verify `head_sha`;
2. if green, metadata only;
3. if red, fetch jobs;
4. fetch the failed job log exactly once;
5. never re-fetch the same failed log.

If the collection endpoint becomes unavailable, capability-check the GitHub connector and explicitly report a connector gap. Do not substitute PR-only `fetch_commit_workflow_runs`.

## Git writes

All branch-changing writes use:

`create_blob -> create_tree -> create_commit -> update_ref`

Never push over an active relevant workflow. Verify HEAD immediately before constructing the commit.

## Compatibility risks

- Borland `String` 1-based indexing versus `std::string` 0-based indexing.
- Transitional `WideString`, `Variant`, `Currency`, `Comp`, formatting behavior.
- Exact legacy flag range boundary fidelity.
- Indirect control flow and jump-table/switch support.
- Legacy mutation of locals/args/flags/`InfoRec`; neutral mutation/ownership semantics must remain explicit.

## Immediate technical frontier

1. Starting from the green #109 stack-frame fixture, introduce the smallest deterministic instruction sequence that causes useful decompiler body output.
2. Keep a counting `manualInput` service installed and require zero calls.
3. Verify output only through the neutral execution result, not through legacy UI/list objects.
4. Stop at the first runtime-reached interactive dependency and model that family explicitly.
5. Continue controlled Delphi Win32 comparisons against original IDR.
6. Later make `idr-cli.exe` emit deterministic decompiler output and publish `idr-cli.exe + dis.dll` artifacts.

## Working rules

- Runtime/compiler/linker evidence over speculation.
- One coherent architecture commit per step.
- Preserve original legacy source unless making a deliberate structural port.
- Prefer adapters/generated portable copies.
- Frozen branches remain frozen; `main` remains untouched.
- Keep this journal synchronized at meaningful runtime milestones.
