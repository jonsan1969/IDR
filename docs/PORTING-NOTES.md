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

Core compatibility / earlier slices:

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

Generated transformed files live under `tests/generated` during CI. Original IDR source remains unchanged.

Later slices reuse the generated prefix from `prepare_portable_decompiler_engine_slice.ps1` so the large neutral declaration surface proven by #49 does not drift between harnesses.

## Compatibility layer

Temporary aliases include `Byte`, `Word`, `DWord`, `String = std::string`, `AnsiString = std::string`, a minimal `WideString` wrapper, and a temporary integer-compatible `Variant` alias for compile mapping.

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

`Pos` and `SubString` helpers preserve Embarcadero's 1-based semantics in generated smoke code. Do not blindly substitute raw `std::string` indexing/find semantics.

Direct 1-based `String[index]` usage remains a known runtime-semantic risk even when code compiles. A thin compatibility String type may eventually be cleaner than an indefinitely growing transform set, but defer that decision until the direct-indexing surface is mapped.

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

At #83 the major instruction-simulation family (`SimulateInstr1`, full `SimulateInstr2`, `SimulateInstr3`) is compiler-mapped.

## Mixed-responsibility headers

### `Main.h`

Core flags/kinds and GUI form state coexist. The tested engine surface strongly justifies neutral `CoreTypes.h` / `IdrTypes.h` extraction.

### `Misc.h`

Pure analysis APIs coexist with forms/canvas/dialog helpers. A separate neutral analysis API header is increasingly justified.

### Form-owned analysis helpers

At least `GetMethodInfo` is analysis logic currently owned by `TFMain_11011981`. It should move behind a core service/context rather than remain on a form class.

## Next technical phase after #83

Before starting broad source reorganization, enumerate any remaining independent `Decompiler.cpp` core helpers not covered by the current smoke matrix. Once those are mapped, shift from isolated object compilation to integration:

1. extract neutral core declarations/types from `Main.h` / `Misc.h`,
2. make real core translation units compile together without generated source surgery where practical,
3. link the portable objects and resolve duplicate/missing extern surfaces,
4. handle Delphi String semantics that compile smoke cannot validate, especially direct 1-based indexing,
5. expose a headless binary-loading/analysis entry point,
6. build first `idr-cli.exe`,
7. add runtime tests using known Delphi Win32 binaries.

Compile-green is not runtime semantic equivalence.

## Working rules

- Preserve original source during dependency mapping.
- Let concrete compiler errors drive shims/transforms.
- Avoid wholesale String/container rewrites until semantics are mapped.
- Do not fake VCL to make smoke compile; identify policy/callback boundaries.
- Keep functional portability separate from cleanup/security modernization.
- Stay x86 first.
- No merge to `main` or upstream PR yet.

## First useful deliverable

```text
idr-cli.exe <target.exe>
```

built on stock GitHub-hosted Windows from real IDR core implementation code, capable of loading/analyzing a Delphi Win32 executable and emitting useful textual or machine-readable output.
