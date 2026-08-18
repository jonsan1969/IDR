# Portable Core Project State

Last updated: 2026-08-18

## Goal

Build a useful headless IDR core/CLI on stock GitHub-hosted Windows runners with MSVC x86, without requiring Embarcadero C++Builder, while preserving the original VCL GUI path.

Repository: `jonsan1969/IDR` (fork of `sarog/IDR`).

## Branches

Active product branch: `agent/portable-cli`

Frozen references — do not modify:

- `agent/portable-core-integration`
- `agent/portable-core-smoke`

`main` remains untouched at the modern Embarcadero baseline.

Current verified code milestone before this documentation-only update:

`0631b5d52bcf0a02079d70e5490653a4e4f09ab3` — `Exercise richer neutral control flow`

Run #79 was reported green. Its parent is the documentation commit `aca88f5708cff9895614cb83bceeee4442ae88ba`, whose parent/code milestone was `428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary` — with green run #78.

`docs/**` is excluded from push-triggered CI, so documentation-only commits do not start the normal integration workflow.

## CI

Workflow: `.github/workflows/portable-core-integration.yml`

The active branch uses:

- GitHub-hosted Windows runner;
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`;
- `actions/checkout@v6`;
- fail-fast compile commands;
- concurrency cancellation;
- real generated legacy translation units;
- deterministic MSVC x86 PE32 fixture;
- neutral control-flow probe;
- end-to-end `idr-cli.exe` execution against the real fixture.

x86 remains intentional because the real legacy decoder and shipped `dis.dll` are x86.

### Actions status/log workflow

Normal project discipline remains:

1. after a code push, inspect workflow metadata/status;
2. if green, do not fetch logs;
3. if failed, inspect jobs and fetch the failed job log exactly once;
4. never push another code commit while the current workflow run is still active.

The GitHub connector currently has a session-specific discovery limitation which future threads must know about:

- `GitHub.fetch` works for a specific known Actions run endpoint such as `/actions/runs/<run_id>` and returns `run_number`, `event`, `status`, `conclusion`, `head_branch`, `head_sha`, etc.;
- `GitHub.fetch_workflow_run_jobs`, `GitHub.fetch_workflow_job_steps`, `GitHub.fetch_workflow_job_logs`, artifact listing and artifact download remain available once a run/job id is known;
- however, in the current ChatGPT session the generic collection endpoint `/actions/runs?branch=agent%2Fportable-cli&per_page=10` is rejected by the connector allowlist with HTTP 400, even though that exact `GitHub.fetch` route worked in the previous thread;
- `/commits/<sha>/check-runs` collection discovery is similarly blocked;
- the wrapper `GitHub.fetch_commit_workflow_runs` is not a substitute because it is currently limited to PR-triggered runs;
- `GitHub.get_commit_combined_status` may return no useful Actions discovery metadata;
- GitHub plugin connection and permissions were verified healthy (`Allow all actions`), so this is not a repo permission problem.

Until discovery is restored, the minimal manual bridge is for the user to report the new run number/status (or run id if failed). Once the run id is known, continue with the established metadata -> jobs -> one failed log workflow.

## Clean active history

`agent/portable-cli` was rebuilt cleanly from `main`; the long linker/compiler archaeology remains on the frozen integration branch.

Important active commits:

- `7f3f3fe68db55c95a7928bbc86f3bf2580519113` — `Establish portable MSVC x86 core baseline`
- `aaf47e2382a3b6419a74230931a052f6481aa6d0` — `Introduce portable CLI host`
- `fcfc08db2b588036afc16a2e882b80a0d186dcb0` — `Decode loaded entry point in portable CLI`
- `ee83293a665d6eeaca11b2a6315de163713c01af` — `Clamp file-aligned section data to analysis span`
- `182277558d0a0f9837d965b3833bb9769741c535` — `Trace direct call procedure candidate`
- `8238a97c54941107ef125bd4b588601eb7893e25` — `Queue bounded procedure candidates`
- `df4a270933e7e0de6d77af69b1c8e3856fd6a6d4` — `Extract bounded control flow from CLI`
- `19a63ffed2770125a7af6f2e88b26e552bfb2189` — `Prove neutral control flow independently`
- `2e6200adf640d3f6d6561703bca905ba811d0c83` — `Trace bounded basic blocks in control flow`
- `4e62a4139f5e1de94d8864150413008ddb350405` — `Mark analyzed procedure starts`
- `c71b2f12ae56d0a1ed3ef2402a2b26ccc4b55412` — `Model explicit control flow edge kinds`
- `78a373b99da00a305ea3b823eb877c1e3af3b9c9` — `Track procedure ownership on control flow edges`
- `d0d08fc108716597b1032a889c99fef6b61690a6` — `Summarize analyzed procedures`
- `3903097f2d6d5a279164a7366300299c3cfad0af` — `Track procedure call xrefs`
- `47c48d12f03a1dfa32a05c7cac6aff4ad84bddcf` — `Count incoming procedure calls`
- `ebbde9b286526725b6cfd645a6335906c78479f0` — `Inject address mapping into control flow`
- `428995104a14bfabb484c13d091f4ee0a6ad58a5` — `Prove neutral control flow link boundary`
- `0631b5d52bcf0a02079d70e5490653a4e4f09ab3` — `Exercise richer neutral control flow`

## Verified architecture

Runtime evidence now covers:

`PE32 target -> IdrPeLoader -> authoritative loaded session -> real MDisasm/dis.dll decode -> bounded neutral CFG -> shared AnalysisState -> deterministic CLI output`

The portable layer contains neutral core services, segment-aware PE loading/image context, shared `AnalysisState`/legacy `Flags`, instruction navigation, legacy session bridge, generated real `Disasm`/`KnowledgeBase`/`Infos`/`Misc` and almost-complete `Decompiler` translation units, `idr-cli.exe`, and neutral `IdrControlFlow`.

The neutral control-flow probe links only:

`IdrAnalysisState.obj + idr_control_flow_probe.obj`

It does not link `IdrImageContext`, PE loader/session bridge, disassembler or generated legacy TUs. `AnalyzeBoundedControlFlow` receives an injected `AddressMapper`; the CLI supplies segment-aware image mapping while the neutral probe supplies a local mapper.

## Neutral control-flow engine

Current behavior includes:

- bounded deduplicated procedure-candidate worklist;
- bounded basic-block worklist per procedure;
- direct calls;
- conditional and unconditional direct branches;
- explicit `Call`, `BranchTaken`, `FallThrough` edges;
- `ret` block termination;
- instruction de-duplication within a procedure;
- procedure ownership on every edge;
- procedure summaries;
- call xrefs `{ caller, callSite, callee }`;
- incoming direct-call counts;
- `Instruction`, `Code`, `Call`, `Loc` flag propagation;
- `ProcStart` only for procedures actually analyzed, never ordinary branch/basic-block targets.

Call-sites are not deduplicated; procedure candidates are.

## Deterministic fixtures

The established neutral fixture still proves three procedures: Entry -> TargetA, TargetA -> TargetB twice, plus a conditional branch. Its contract remains 3 entry blocks, 2 candidates, 5 edges (3 Call, 1 BranchTaken, 1 FallThrough), ownership 3/2/0, 3 call xrefs and incoming call counts 0/1/2.

Commit `0631b5d...` added a second rich graph fixture without changing production `IdrControlFlow` semantics. Green run #79 proves the existing engine handles:

- unconditional direct jumps;
- loops/back-edges;
- cross-block joins;
- multiple incoming edges to a join;
- invalid/out-of-image direct call targets;
- invalid/out-of-image unconditional jump targets;
- no invalid edge/xref/candidate creation;
- branch/join/loop targets still not promoted to `ProcStart`.

## Procedure extent / ProcEnd evidence

Do not invent `ProcEnd` semantics.

Legacy `AnalyzeProc1` sets `cfProcStart | cfPass1` on an analyzed procedure. On accepted procedure-ending returns it writes:

`recN->procInfo->procSize = curAdr - fromAdr + instrLen`

while the adjacent `SetFlag(cfProcEnd, ...)` is commented out. A similar size assignment appears for a terminating near absolute indirect jump outside valid code, again with the `cfProcEnd` write commented out. Entry-point `@Halt0` handling follows the same size-metadata pattern.

`GetProcSize(fromAdr)` first returns `recN->procInfo->procSize` when present; if no stored size exists it falls back to `FMain_11011981->EstimateProcSize(fromAdr)`.

Therefore current evidence says procedure extent is primarily explicit `procInfo->procSize` metadata with an estimator fallback, not a simple `ProcEnd` bit derived from every `ret`. More legacy investigation is required before adding neutral extent semantics.

## Known risks

- Borland `String` direct indexing is 1-based versus `std::string` 0-based.
- `WideString`, `Variant`, `Currency`, `Comp` and formatting compatibility remain transitional.
- exact legacy `SetFlags`/`ClearFlags` end-boundary fidelity remains open.
- metadata ownership/lifetime matters as deeper `InfoRec`/type/procedure paths activate.
- indirect calls/jumps, switch/jump tables and richer x86 control flow remain unsupported by neutral CFG.
- procedure size/end semantics are not yet fully proved.
- imports/exports/resources and Delphi-specific discovery are not yet driven through CLI.
- deeper decompiler passes are not yet invoked by the CLI.

## Immediate next phase

1. finish deriving `EstimateProcSize` / initial procedure-size semantics from legacy code;
2. decide whether neutral procedure summaries should expose an evidence-backed extent field without creating `ProcEnd` semantics;
3. begin feeding discovered procedures into the next safe legacy analysis/decompiler stage;
4. inspect resulting `Flags`, `Infos`, procedure/type metadata and lifetimes;
5. audit runtime-reached Borland String/RTL paths;
6. later add controlled Delphi Win32 fixtures and comparison against original IDR;
7. when useful outside CI, publish both `idr-cli.exe` and required `dis.dll` as an Actions artifact.

## Working rules

- Compiler/linker/runtime evidence before speculation.
- One coherent architecture commit at a time.
- Do not disturb frozen branches or `main`.
- Do not push while the current integration run is active.
- Successful runs: metadata only, no log retrieval.
- Failed runs: metadata -> jobs -> failed job log exactly once.
- Preserve original legacy source unless a deliberate structural port requires a change.
- GitHub writes on the active branch use `create_blob -> create_tree -> create_commit -> update_ref`.
- No PR/merge to `main` yet.

## Success criterion

`windows-latest -> MSVC x86 -> portable IDR core -> real PE32 analysis -> headless idr-cli.exe -> GitHub Actions artifact`

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
