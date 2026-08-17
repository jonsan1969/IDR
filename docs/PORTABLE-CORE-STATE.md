# Portable Core Project State

Last updated: 2026-08-17

## Goal

Build a useful headless IDR core/CLI on free GitHub-hosted Windows Actions runners with MSVC, without requiring Embarcadero C++Builder, while leaving the original VCL GUI path intact.

Starting point: `sarog/IDR` modern fork. Current experimental fork: `jonsan1969/IDR`.

## Working branch

`agent/portable-core-smoke`

`main` remains untouched. Do not merge or open an upstream PR until the experiment has produced a coherent portable target.

## CI baseline

Workflow: `.github/workflows/portable-core-smoke.yml`

- `windows-latest` (currently Windows Server 2025 / VS2026 image)
- MSVC x86 via `vswhere.exe` + `vcvars32.bat`
- `actions/checkout@v6`
- `concurrency.cancel-in-progress: true`
- `docs/**` ignored for push-triggered CI

x86 is intentional because legacy `Disasm.cpp` contains inline x86 assembly that MSVC cannot compile in x64 mode.

## Proven portable so far

Successfully object-compiled on GitHub-hosted MSVC x86:

- `Disasm.h`
- `KnowledgeBase.h`
- `Infos.h`
- generated portable `Decompiler.h`
- real `Disasm.cpp` after generated syntax/compatibility transforms
- primary real `Decompiler.cpp` slice from `GetString()` through complete `TDecompiler::Init()`
- complete BJL/branch-analysis slice from `GetBJLRange()` through `PrintBJL()`
- complete main-engine slice containing all of `TDecompiler::Decompile()`
- complete `TDecompiler::DecompileCaseEnum()`
- complete syscall slice: `GetSysCallAlias()` + `SimulateSysCall()`
- complete call-simulation slice: `SimulateInherited()` + `SimulateCall()`
- complete comparison reconstruction: `GetCmpInfo()`
- complete one-operand simulator: `SimulateInstr1()`
- complete two-operand simulator family: `SimulateInstr2RegImm()`, `SimulateInstr2RegReg()`, `SimulateInstr2RegMem()`, `SimulateInstr2MemImm()`, `SimulateInstr2MemReg()` and dispatcher `SimulateInstr2()`
- complete three-operand simulator: `SimulateInstr3()`
- complete instruction-stack simulators: `SimulatePush()` + `SimulatePop()`

Original source remains unchanged; portability adaptations are generated under `tests/generated` or expressed as smoke-only compatibility declarations.

## Milestone history

### Primary / branch analysis

- #27 green: complete `TDecompiler::Init()`.
- #38 green: complete BJL slice through `PrintBJL()`.

### Main engine

- #40/#42/#44/#46/#48 progressively exposed hidden core declarations, RTL/String semantics and small GUI/policy seams.
- #49 green: **complete `TDecompiler::Decompile()`**.

The main engine is compiler-portable at object-compilation level once hidden core dependencies, limited UI-policy seams and observed Embarcadero RTL semantics are made explicit. This is not yet runtime semantic equivalence and not yet a linked CLI.

### Case enum / syscall

- #51 green: **complete `TDecompiler::DecompileCaseEnum()`**.
- #53-#56: harness-only syscall slicing failures; no portability finding.
- #57: first real syscall compile exposed only `GetTypeName`, `FT_EXTENDED` and known `String::Pos()` semantics.
- #58 green: **complete `GetSysCallAlias()` + `SimulateSysCall()`**.

### Call simulation

- #60: first real `SimulateInherited()` + `SimulateCall()` compile exposed direct VCL/form seams (`FMain_11011981`, `Application`) plus one numeric `String(DWord)` case.
- #61: converged to one remaining `String(DisInfo.Offset)` conversion.
- #62 green: **complete `SimulateInherited()` + `SimulateCall()`**.

The generated smoke copy maps embedded-procedure confirmation to `PortableConfirmEmbeddedProcedure(...)` and form-owned method lookup to `PortableGetMethodInfo(...)` instead of inventing VCL stubs.

### Comparison reconstruction

- #64 green: **complete `TDecompiler::GetCmpInfo()`**.

### Instruction simulation

- #66 green: **complete `TDecompiler::SimulateInstr1()`**.
- #68 red: first `SimulateInstr2RegImm()` compile exposed only two `CmpInfo.R = 0` String assignments.
- #69 green: **complete `SimulateInstr2RegImm()`**.
- #71 green: **complete `SimulateInstr2RegReg()`**.
- #73 red: `SimulateInstr2RegMem()` exposed one direct integer/String concatenation.
- #74 green: **complete `SimulateInstr2RegMem()`**.
- #76 green: **complete `SimulateInstr2MemImm()`**.
- #78 green: **complete `SimulateInstr2MemReg()`**.
- #79 was a generator-only green run and did not yet contain the dispatcher compile.
- #80 green: **complete `TDecompiler::SimulateInstr2()` dispatcher**, proving the full two-operand family compile-green.
- #82 red: first `SimulateInstr3()` compile exposed four instances of the already-known `String(_imm)` numeric construction and no new dependency class.
- #83 green: **complete `TDecompiler::SimulateInstr3()`**.
- #84 was a generator-only push/pop run.
- #85 green: **complete `TDecompiler::SimulatePush()` + `TDecompiler::SimulatePop()`**.
- #86 was a generator-only float run.
- #87 red: first integrated `SimulateFloatInstruction()` compile; this began the tail-helper/float compatibility phase rather than proving float support green.

At #85 the major integer instruction simulation plus instruction-stack simulation is compile-green under hosted MSVC x86. Float/tail-helper mapping is still being converged.

### Final Decompiler helper coverage audit

A source/header coverage audit was added before beginning structural extraction.

By #98 the prepare/audit step reports **52/52 declared `TDecompiler` methods accounted for by the smoke matrix**. This means the coverage inventory itself is complete; it does **not** mean all 52 compile simultaneously yet.

The final helper aggregation currently includes the previously uncovered helper group:

- `GetArrayFieldOffset()`
- `GetCycleFrom()`
- `GetCycleIdx()`
- `GetCycleTo()`
- `GetFloatItemFromStack()`
- `GetInt64ItemFromStack()`
- `GetMemItem()`
- `GetStringArgument()`

#98 reached the compile step but failed. The failure exposed two categories:

1. a **harness boundary bug**: `GetArrayFieldOffset()` is formatted with a line break between `__fastcall` and the qualified method name, so the old extractor started too early and duplicated already-proven methods such as `DecompileTry()`, `MarkCaseEnum()` and `GetCycle*()`;
2. genuine final compatibility dependencies around legacy Delphi/BCB float types/constants/functions, `ikInterface`, helper declarations and additional controlled String/Variant conversions.

The boundary bug was fixed rather than changing IDR source. Smoke compatibility was extended for the real dependency set (`Comp`, `Currency`, `FT_*`, `FloatToStr`, `ikInterface`, `GetArrayIndexes(...)`, and the observed String/Variant boundary).

- #99 is an intermediate compatibility commit/run and may be cancelled by concurrency.
- #100 (`Fix final Decompiler helper smoke boundaries`, commit `b9bfb56832779c82ddbda3a7c0bf779a5d267211`) is the current authoritative run and was still in progress when this document was updated.

## Engine GUI/interactivity boundary

Embedded-procedure handling directly reads/writes `FMain_11011981->lbCode->ItemIndex` and calls `Application->MessageBox()`. Generated smoke code replaces that plumbing with `PortableConfirmEmbeddedProcedure(...)`.

Virtual-method lookup is analysis logic but is currently owned by the form through `FMain_11011981->GetMethodInfo`; smoke code exposes it as `PortableGetMethodInfo(...)`.

`ManualInput(...)` remains an explicit dependency for unresolved return-byte/type cases and should eventually become a resolver callback or deterministic CLI policy.

## Compatibility layer status

`tests/portable_core_compat.h` supplies temporary compiler-neutral aliases/containers plus only constants reached by tested code.

Compiler-verified String/RTL differences currently mapped or exposed in generated smoke code:

- numeric `String(int)` -> controlled `std::to_string(...)`
- numeric zero assigned to `String` -> explicit textual `"0"` where observed
- direct integer/String concatenation -> explicit decimal conversion where observed
- `.Length()` -> `.size()`
- `String::Pos()` -> helper preserving 1-based/zero-not-found semantics
- `SubString()` -> helper preserving 1-based start semantics
- `SetLength()` -> `resize()` for observed smoke use
- `AnsiReplaceText()`, `IntToStr`, `IntToHex`, `QuotedStr` exposed as dependencies
- `AnsiString` temporarily mapped to `std::string`
- `WideString` has a minimal smoke wrapper
- `Variant` remains a targeted smoke abstraction; late helper mapping exposed another implicit String/Variant boundary
- legacy float compatibility now needs explicit smoke representations for `Comp`, `Currency`, `FT_*` and `FloatToStr`
- `ikInterface` (`0x0F` in original `Main.h`) is now part of the reached neutral kind surface

`String = std::string` remains a compile-smoke approximation, not runtime equivalence. Direct 1-based `String[index]` remains a known semantic hazard and must be addressed before calling the engine runtime-ready.

`TStringList` includes `Strings`, aligned `Objects`, sorted insertion, `IndexOf`, and `IndexOfName`. It is still a targeted compatibility shim, not full VCL semantics.

## Harness lessons

The final helper audit added an important extraction rule: method extraction must key from the qualified method name and locate the actual return-type boundary robustly, because original BCB formatting may split return type / `__fastcall` / method name across lines. Do not assume `"<return> __fastcall TDecompiler::..."` appears on one physical line.

Harness failures and compiler portability failures must remain distinct. #98 produced both, and the duplicate-method diagnostics were caused by extraction boundaries rather than by IDR itself.

## Architectural boundaries found

### `Main.h`

Mixes core records/constants with VCL GUI state. Current runs strongly support extracting reusable core definitions into a neutral header (`CoreTypes.h` / `IdrTypes.h`).

### `Misc.h`

Mixes pure analysis helpers with forms/canvas/dialog helpers. Tested engine code repeatedly uses the pure half without needing the UI half, supporting a separate neutral core-analysis API header.

### GUI boundary after `TDecompiler::Init()`

`OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly couple to form/presentation behavior and remain intentionally skipped by headless core mapping.

### GUI/interactivity inside `TDecompiler::Decompile()` / call simulation

The engine is overwhelmingly analysis code but contains small policy/UI decisions around embedded procedures, virtual-method lookup ownership, and unresolved-call input. These need injected interfaces/policies in a real headless core.

## Current direction after the 52/52 audit

The Decompiler method inventory is now complete at the coverage level. Immediate work is to get the final aggregated helper slice compiler-green after the #98 compatibility findings. Once that is green, stop expanding the slice matrix and transition to integration:

1. neutral core headers/API extraction,
2. linkable core translation units,
3. Delphi/Embarcadero String semantic cleanup (especially direct 1-based indexing),
4. headless binary loading / analysis entry path,
5. first linked `idr-cli.exe`,
6. runtime tests against known Delphi Win32 binaries.

Do not interpret 52/52 coverage or the current green matrix as proof of runtime semantic equivalence.

## Working rules

- Preserve original source during dependency mapping.
- Add compatibility semantics only when actual code requires them.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- Do not pull GUI code into core just to keep source slices contiguous.
- Fetch a failed Actions log once per run; successful runs need status/job verification only.
- Keep these docs updated at meaningful milestones.

## Success criterion

`windows-latest` -> MSVC x86 -> portable IDR core -> headless `idr-cli.exe` -> GitHub Actions artifact

without Embarcadero C++Builder, paid CI tooling or a self-hosted runner.
