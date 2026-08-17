# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical scratchpad for the experimental MSVC/GitHub-hosted portability work.

## Repository / branch

- Original: `crypto2011/IDR`
- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Experimental branch: `agent/portable-core-smoke`

The experiment asks whether the analysis/decompiler core can become compiler-neutral enough for stock GitHub-hosted MSVC while the existing VCL application remains intact.

## CI

Workflow: `.github/workflows/portable-core-smoke.yml`

Observed hosted environment: Windows Server 2025 / `windows-2025-vs2026`, VS 2026, MSVC x86 through `vswhere.exe` + `vcvars32.bat`.

Use `actions/checkout@v6`. `docs/**` does not trigger push CI. Stale runs are cancelled through concurrency.

For a failing run, fetch the job log once and analyze that returned result. Successful runs are verified from run/job/step metadata without fetching logs.

## Harness files

Core compatibility / slices:

- `tests/portable_core_compat.h`
- `tests/prepare_portable_disasm.ps1`
- `tests/prepare_portable_decompiler.ps1`
- `tests/prepare_portable_decompiler_slice.ps1`
- `tests/prepare_portable_decompiler_branch_slice.ps1`
- `tests/prepare_portable_decompiler_engine_slice.ps1`
- `tests/prepare_portable_decompiler_case_slice.ps1`
- `tests/prepare_portable_decompiler_syscall_slice.ps1`
- `tests/prepare_portable_decompiler_call_slice.ps1`
- `tests/prepare_portable_decompiler_cmp_slice.ps1`
- `tests/prepare_portable_decompiler_instr1_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_regimm_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_regreg_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_regmem_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_memimm_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_memreg_slice.ps1`
- `tests/prepare_portable_decompiler_instr2_dispatch_slice.ps1`
- `tests/prepare_portable_decompiler_instr3_slice.ps1`
- `tests/prepare_portable_decompiler_pushpop_slice.ps1`
- `tests/prepare_portable_decompiler_float_slice.ps1`
- `tests/audit_portable_decompiler_coverage.ps1`

Generated transformed files live under `tests/generated` during CI. Original IDR source remains unchanged.

Later slices reuse the generated prefix from `prepare_portable_decompiler_engine_slice.ps1` so the large neutral declaration surface proven by #49 does not drift between harnesses.

`prepare_portable_decompiler_instr3_slice.ps1` and `prepare_portable_decompiler_float_slice.ps1` exist as standalone generators, but the actual workflow compile proof is obtained through the extended dispatcher and push/pop translation units respectively.

## Compatibility layer

Temporary aliases include `Byte`, `Word`, `DWord`, `String = std::string`, `AnsiString = std::string`, a minimal `WideString` wrapper, and targeted smoke representations for reached Delphi/BCB types.

### Proven String / RTL mismatches

- #33: numeric `String(int)` differs; generated smoke code uses controlled `std::to_string(...)` transforms.
- #36: `String::Length()` maps to `.size()` for observed cases.
- #38: `AnsiReplaceText(...)` exposed with a narrow compile-time signature.
- #42 onward: engine reaches `String::Pos()`, `SubString()`, `SetLength()`, `IntToStr`, `IntToHex`, `QuotedStr`, `AnsiString`, `WideString` and additional controlled numeric `String(...)` forms.
- #48: final main-engine failures were only numeric/String semantic mismatches.
- #57: syscall reused the known 1-based `String::Pos()` mapping.
- #60/#61: call simulation needed controlled numeric conversions including `DisInfo.Offset`.
- #68: `CmpInfo.R = 0` demonstrated Embarcadero numeric-to-String assignment semantics in `SimulateInstr2RegImm()`; smoke copy uses textual `"0"`.
- #73: direct `_offset` concatenation into a String expression needed explicit decimal conversion.
- #82: `SimulateInstr3()` reused the already-known `String(_imm)` numeric construction; no new semantic category appeared.
- #98: final helper aggregation exposed additional numeric `String(...)` uses and an implicit String/Variant boundary; continue using narrow transforms rather than global constructor rewrites.

`Pos` and `SubString` helpers preserve Embarcadero's 1-based semantics in generated smoke code. Do not blindly substitute raw `std::string` indexing/find semantics.

Direct 1-based `String[index]` usage remains a known runtime-semantic risk even when code compiles. A thin compatibility String type may eventually be cleaner than an indefinitely growing transform set, but defer that decision until the direct-indexing surface is mapped.

### Legacy float surface

The final helper group reaches BCB/Delphi float support that earlier slices only referenced indirectly:

- `Comp`
- `Currency`
- `FT_SINGLE`
- `FT_REAL`
- `FT_DOUBLE`
- `FT_COMP`
- `FT_CURRENCY`
- `FT_EXTENDED`
- `FloatToStr(...)`

These are being represented in the smoke compatibility layer only far enough to preserve compile mapping. Runtime representation/formatting equivalence remains future work.

### Core kinds / helper declarations

Late helper mapping also reaches `ikInterface` (`0x0F` in original `Main.h`) and `GetArrayIndexes(...)`. These belong to the neutral analysis surface, not the VCL UI surface.

### Containers

`TList` remains a non-owning vector-backed smoke shim.

`TStringList` supports `Strings`, aligned `Objects`, sorted insertion preserving alignment, `IndexOf`, and `IndexOfName` with `=` as the current name/value separator. This is targeted compatibility, not full VCL semantics.

### `Exception`

Run #40 exposed Borland `Exception::Message`; the shim stores that public string alongside `std::runtime_error`.

## `Disasm.cpp`

Real implementation compiles with MSVC x86 after generated portability transforms. Legacy CRT and inline-x86 warnings remain separate from portability work.

## `Decompiler.cpp` mapping

### Primary / presentation boundary

`GetString()` through complete `TDecompiler::Init()` is green at #27.

Immediately after `Init()`, `OutputSourceCodeLine()`, `OutputSourceCode()`, and `DecompileProc()` directly use VCL/form presentation behavior and are intentionally excluded from core mapping.

### BJL / branch analysis

`GetBJLRange()` through complete `PrintBJL()` is stable green at #38.

### Main engine

Complete `TDecompiler::Decompile()` is green at #49 after runs #40/#42/#44/#46/#48 progressively mapped hidden declarations/constants, String semantics and small GUI/policy seams.

Important harness lesson from #44: textual transforms must use guarded/controlled argument matching. An earlier broad replacement corrupted `GetImmString(...)` into `GetImmstd::to_string`; the generator was fixed rather than changing source.

### Case enum

Complete `TDecompiler::DecompileCaseEnum()` is green at #51. Numeric case-label construction is mapped narrowly to `std::to_string(...)`.

### Syscall

`GetSysCallAlias()` + complete `SimulateSysCall()` are green at #58.

#53-#56 were harness-only boundary/marker failures. #57 was the first actual compile and exposed only `GetTypeName(DWord)`, `FT_EXTENDED`, and known `String::Pos()` behavior.

### Call simulation

The slice covers `SimulateInherited()` plus complete `SimulateCall()`.

- #60: first compiler result exposed direct form/VCL seams and one numeric String case.
- #61: only `String(DisInfo.Offset)` remained.
- #62: complete call-simulation slice green.

Embedded procedure UI is mapped in smoke code to `PortableConfirmEmbeddedProcedure(...)`; form-owned `GetMethodInfo` is mapped to `PortableGetMethodInfo(...)`. `ManualInput(...)` is still a future CLI resolver/policy seam.

### Comparison reconstruction

Complete `TDecompiler::GetCmpInfo()` is green at #64.

### Instruction simulation

#### One operand

- #66: complete `SimulateInstr1()` green.

#### Two operands

- #68: first `SimulateInstr2RegImm()` compile exposed only two numeric-zero String assignments.
- #69: `SimulateInstr2RegImm()` green.
- #71: `SimulateInstr2RegReg()` green.
- #73: `SimulateInstr2RegMem()` exposed one direct integer/String concatenation.
- #74: `SimulateInstr2RegMem()` green.
- #76: `SimulateInstr2MemImm()` green.
- #78: `SimulateInstr2MemReg()` green.
- #79: generator-only green run; dispatcher not yet present in workflow steps.
- #80: complete `SimulateInstr2()` dispatcher green.

Therefore the full two-operand family is object-compile green under hosted MSVC x86.

#### Three operands

- #82: first `SimulateInstr3()` compile failed on four occurrences of `String(_imm)` only; no new core or GUI dependency class.
- #83: complete `SimulateInstr3()` green.

#### Push / pop

- #84: generator-only intermediate run.
- #85: complete `SimulatePush()` + `SimulatePop()` green.

This is distinct from the earlier generic `TDecompiler::Push(PITEM)` / `Pop()` stack helpers already present in the primary slice.

#### Float instruction path

- #86: standalone float generator intermediate run.
- #87: first integrated `SimulateFloatInstruction()` compile was red. This showed that the next phase was no longer the basic integer instruction family but the float/tail-helper compatibility surface.

Do not record #87 as a float-success milestone.

## Final method coverage audit

Before reorganizing source, a method-level audit was added to compare declared `TDecompiler` methods against the generated smoke coverage.

By #98, the prepare/audit step reports **52/52 methods accounted for**. Coverage accounting is therefore complete at the declaration/mapping level.

The final previously uncovered helper group is:

- `GetArrayFieldOffset()`
- `GetCycleFrom()`
- `GetCycleIdx()`
- `GetCycleTo()`
- `GetFloatItemFromStack()`
- `GetInt64ItemFromStack()`
- `GetMemItem()`
- `GetStringArgument()`

The extended push/pop slice is currently used as the aggregate translation unit for this final helper proof. This avoids adding a long sequence of nearly identical workflow prepare/compile pairs while the mapping is converging.

### #98 findings

#98 passed `Prepare Decompiler push-pop slice`, including the 52/52 coverage audit, then failed `Compile Decompiler push-pop slice`.

A major portion of the diagnostics was **not an IDR portability problem**. `GetArrayFieldOffset()` is formatted as:

```cpp
int __fastcall
TDecompiler::GetArrayFieldOffset(...)
```

The extractor expected `int __fastcall ` immediately before the qualified name. Its `LastIndexOf(...)` therefore selected an earlier function boundary and appended a large extra source range. That caused false duplicate-body errors for already-proven methods such as `DecompileTry()`, `MarkCaseEnum()` and `GetCycle*()`.

Lesson: method slicing must tolerate BCB formatting where return type, calling convention and qualified method name span physical lines. Locate the method name first and derive a robust declaration boundary; do not assume one-line signatures.

After separating the harness artifacts, the real #98 compatibility classes were:

- numeric `String(...)` constructions in newly reached code;
- legacy float types/constants/functions (`Comp`, `Currency`, `FT_*`, `FloatToStr`);
- `ikInterface`;
- `GetArrayIndexes(...)`;
- an observed implicit String-to-Variant call boundary.

The fix strategy remains the same as earlier phases: extend smoke declarations/transforms narrowly and leave original `Decompiler.cpp` untouched.

### #99 / #100

- #99 (`Add legacy float and interface smoke types`, commit `a66fab73466e85cfc46af236811b5579f3c6b6cc`) is an intermediate run and is subject to concurrency cancellation.
- #100 (`Fix final Decompiler helper smoke boundaries`, commit `b9bfb56832779c82ddbda3a7c0bf779a5d267211`) combines the corrected extraction boundary with the current final-helper shims and is the authoritative run. It was still in progress when these notes were written.

## Mixed-responsibility headers

### `Main.h`

Core flags/kinds and GUI form state coexist. The tested engine surface strongly justifies neutral `CoreTypes.h` / `IdrTypes.h` extraction.

### `Misc.h`

Pure analysis APIs coexist with forms/canvas/dialog helpers. A separate neutral analysis API header is increasingly justified.

### Form-owned analysis helpers

At least `GetMethodInfo` is analysis logic currently owned by `TFMain_11011981`. It should move behind a core service/context rather than remain on a form class.

## Next technical phase after the 52/52 audit

First finish compiler convergence of the final aggregated helper slice. Once all accounted-for `TDecompiler` methods compile under the smoke environment, stop broadening method coverage and move to integration:

1. extract neutral core declarations/types from `Main.h` / `Misc.h`,
2. make real core translation units compile together without generated source surgery where practical,
3. link the portable objects and resolve duplicate/missing extern surfaces,
4. handle Delphi String semantics that compile smoke cannot validate, especially direct 1-based indexing,
5. expose a headless binary-loading/analysis entry point,
6. build first `idr-cli.exe`,
7. add runtime tests using known Delphi Win32 binaries.

Compile-green and 52/52 mapped coverage are not runtime semantic equivalence.

## Working rules

- Preserve original source during dependency mapping.
- Let concrete compiler errors drive shims/transforms.
- Avoid wholesale String/container rewrites until semantics are mapped.
- Do not fake VCL to make smoke compile; identify policy/callback boundaries.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- No merge to `main` or upstream PR yet.
- Treat harness/extraction defects separately from source portability defects.

## First useful deliverable

```text
idr-cli.exe <target.exe>
```

built on stock GitHub-hosted Windows from real IDR core implementation code, capable of loading/analyzing a Delphi Win32 executable and emitting useful textual or machine-readable output.
