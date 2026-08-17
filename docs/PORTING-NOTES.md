# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

This file is the technical scratchpad for the experimental MSVC/GitHub Actions portability work. Keep it factual and update it when a new dependency, workaround, or architectural boundary is discovered.

## Repository lineage

- Original project: `crypto2011/IDR`
- Modern C++Builder fork used as starting point: `sarog/IDR`
- Current working fork: `jonsan1969/IDR`
- Experimental branch: `agent/portable-core-smoke`

## Why this branch exists

`sarog/IDR` is much easier to work from than the historical C++Builder 6 source because it has already been updated for modern Embarcadero C++Builder and CMake/Ninja. However, the project still links VCL/RTL and therefore cannot simply be built for free on a stock GitHub-hosted runner.

The current experiment asks a narrower question:

> Can the analysis/decompiler core be made compiler-neutral enough to build with MSVC on `windows-latest`, while leaving the VCL GUI untouched?

So far the answer is yes for a meaningful amount of code.

## Existing CMake limitation

The upstream/sarog CMake path is Embarcadero-specific. It uses Embarcadero helper functions and links VCL/RTL libraries. Therefore the current MSVC experiment does not reuse that target directly yet.

A future portable build should probably introduce a separate target rather than trying to make one monolithic target support both VCL/C++Builder and MSVC immediately.

Suggested eventual layout:

```text
IDR
├── core/
│   ├── analysis
│   ├── decompiler
│   ├── disassembler
│   ├── knowledge-base
│   └── RTTI
├── compat/
│   ├── portable types/containers
│   └── compiler-specific glue
├── gui-vcl/
│   └── existing VCL application
└── cli/
    └── headless entry point
```

Do not perform this reorganization until the smoke-test experiment has mapped the real dependency surface.

## Current test harness files

- `.github/workflows/portable-core-smoke.yml`
- `tests/portable_core_compat.h`
- `tests/portable_disasm_header_smoke.cpp`
- `tests/portable_kb_header_smoke.cpp`
- `tests/portable_infos_header_smoke.cpp`
- `tests/portable_decompiler_header_smoke.cpp`
- `tests/prepare_portable_disasm.ps1`
- `tests/prepare_portable_decompiler.ps1`
- `tests/prepare_portable_decompiler_slice.ps1`

Generated files are placed under `tests/generated` during Actions runs and are not intended to replace the original source files.

## Current compiler/runtime substitutions

### Fundamental types

```cpp
using Byte  = std::uint8_t;
using Word  = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;
```

`String = std::string` is currently sufficient for the tested code, but this does not prove semantic equivalence for all IDR code. Borland/Embarcadero `String` indexing and Unicode behavior may require a more careful compatibility type later.

### `__fastcall`

For current compile smoke tests it is neutralized with a macro under MSVC. Calling convention compatibility matters only once independently built objects expose ABI boundaries or interact with external code expecting the original convention.

### `TList`

Current minimal functionality needed:

- `Count`
- `Items[index]`
- `Add(void *)`

Implemented using `std::vector<void *>` in the test compatibility layer.

### `TStringList`

Current minimal functionality needed:

- `Sorted`
- `Count`
- `Add(String)`
- `IndexOf(String)`

Implemented using `std::vector<std::string>` plus sorting in the test compatibility layer.

Potential future semantics to watch:

- duplicate handling
- case sensitivity
- sorted insertion behavior
- `Objects[]`
- ownership
- encoding

Do not assume the current shim is sufficient outside the code already tested.

### `Exception`

The decompiler stack code throws Borland `Exception("Stack!")`. The portable test layer replaces this with a standard C++ exception wrapper derived from `std::runtime_error`.

## `Disasm.cpp`

`Disasm.cpp` is surprisingly portable once the compiler-specific syntax is handled.

Known issues:

- Borland inline-assembly syntax has to be transformed for MSVC.
- MSVC x64 does not support inline asm, so current target is x86.
- MSVC warns that the code modifies `ebp` in inline assembly (`C4731`). This is currently a warning, not a compile blocker.
- legacy CRT string functions generate deprecation warnings.

Current policy: compile behavior first; modernize later.

## `Decompiler.cpp`

The source is large (~372 KB) and should not be attacked as one giant port. The current method expands an implementation slice progressively from a known beginning marker.

Early routines compiled without major changes once basic IDR types were available.

The first meaningful runtime dependencies encountered were `TStringList`, `TList`, and Borland `Exception`, all of which have so far been representable with small standard-C++ shims.

This is encouraging because these are compatibility issues, not deeply VCL-coupled algorithmic dependencies.

## `Main.h`

Major architectural smell: core records and application GUI state coexist in the same header.

Expected future action:

- identify structs/enums/records consumed by analysis code
- move/copy them into a neutral core header
- make `Main.h` consume that core header rather than making the core include `Main.h`

Do this only when enough of the core has been mapped to avoid repeatedly reshuffling definitions.

## `TypeInfo2`

`TypeInfo2` contains useful RTTI processing but places it inside/alongside a VCL form class.

Likely future split:

```text
RTTI core helpers
    Guid2String
    GetRTTI
    GetCppTypeInfo

VCL presentation
    ShowKbInfo
    ShowRTTI
    memo/form handlers
```

The exact split must be based on implementation dependencies, not only declarations.

## UI-only areas already identified

These should not block a headless core:

- `InputDlg.*`
- most/all form `.dfm` files
- `Resources.*` initially
- GUI dialogs and viewers

For any algorithm that currently asks the UI for data, prefer an interface/callback in the eventual core rather than pulling VCL into the portable target.

## CI details

The GitHub-hosted Windows environment observed in successful runs:

- Windows Server 2025
- image `windows-2025-vs2026`
- Visual Studio 2026 Developer Command Prompt 18.8.2
- MSVC x86 initialized through `vswhere.exe` and `vcvars32.bat`

Do not hardcode a Visual Studio installation path. Continue using `vswhere`.

### Node.js Actions policy

GitHub is deprecating Node.js 20 based actions. The portability workflow therefore uses:

- `actions/checkout@v6` (Node 24 generation)
- no `ilammy/msvc-dev-cmd@v1`, because that action declares Node 20

Prefer shell/compiler setup over adding old third-party actions when the runner already contains the necessary tooling.

## Run history milestones

Only milestone runs matter; intermediate runs may be cancelled by concurrency.

- Initial header test: `Disasm.h` compiled with MSVC.
- Later run: `KnowledgeBase.h` also compiled.
- Later run: real `Disasm.cpp` compiled with MSVC x86 after generated syntax/compatibility adjustment.
- Run #13: all tests green, including `Infos.h`, portable `Decompiler.h`, and real `Disasm.cpp`.
- Run #15: first real `Decompiler.cpp` implementation slice green.
- Run #17: substantially expanded decompiler slice green, including STL-backed list shims.
- Run #19: active/next milestone at time of writing; extends through register and stack handling and adds `Exception` shim.

Do not fetch old workflow logs repeatedly. Use the already documented result unless a later run changes the conclusion.

## Things not to do yet

- Do not port the VCL GUI to Qt/wxWidgets/Win32 merely to get CI green.
- Do not replace every Borland string/container use across the whole repository in one pass.
- Do not modernize CRT calls at the same time as portability changes unless required to compile.
- Do not attempt x64 before the x86 portable core works; the inline assembly makes that a separate project.
- Do not merge the experiment into `main` until there is a coherent buildable target or a clearly useful checkpoint.

## Definition of a useful first deliverable

A real portable milestone is reached when Actions can produce an executable or library from actual IDR core implementation code, not generated isolated compile-only slices.

Preferred first deliverable:

```text
idr-cli.exe <target.exe>
```

with enough functionality to load/analyze a Delphi Win32 executable and emit useful machine-readable/text output, even if the VCL GUI remains a separate Embarcadero build.
