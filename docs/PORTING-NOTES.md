# IDR Portable Core - Technical Notes

Last updated: 2026-08-17

Technical journal for the MSVC/GitHub-hosted portable-core work.

## Repository / branches

- Modern Embarcadero baseline: `sarog/IDR`
- Working fork: `jonsan1969/IDR`
- Active integration branch: `agent/portable-core-integration`
- Frozen smoke/reference branch: `agent/portable-core-smoke`

The integration branch was reconstructed directly from clean `main`; it does not inherit smoke-branch generated artifacts or docs history by ancestry.

## Smoke phase summary

The smoke branch established complete 52/52 compile representation for `TDecompiler` methods under MSVC x86. That branch is frozen and should be treated as a regression/reference laboratory, not the place for further integration work.

Key smoke lessons carried forward:

- avoid giant Borland `String` wrappers without semantic evidence;
- preserve `__fastcall` ABI on x86;
- Borland 1-based `String` semantics are a real runtime hazard;
- method extractors must tolerate multiline declarations;
- GUI/policy seams should be abstracted, not satisfied by fake VCL.

## Integration chronology

### Clean start

Initial clean integration commit: `4fbe2c6f020ed0b0d2990081b3f37d09ea7e9ec7`.

This established the real-core workflow and neutral layers from a clean main-based branch.

### Analysis/core layers

The integration work introduced neutral core types/services, analysis helpers, analysis-state storage and instruction navigation. The new `AnalysisState` currently uses symmetric range validation; note that original legacy `SetFlags` and `ClearFlags` have an exact-end asymmetry that still requires deliberate behavioral review.

### Real decoder runtime

Run #23 proved the generated real `Disasm.cpp` translation unit links and executes against the repository's shipped x86 `dis.dll`.

The generated portable transform also corrects the apparent initialization ordering defect in legacy `MDisasm::Init()` for the portable build only: function pointers are resolved before `PdisNew(1)` is invoked. Original `Disasm.cpp` remains unchanged.

### Segment-aware image context

Run #24 extended `IdrImageContext` from a flat `imageBase + offset` model to segment-aware address mapping matching legacy `SegmentList` behavior. Unbacked segments (`0x80000`) are represented without pretending bytes exist in `Code`.

This is important because a flat mapping is insufficient for faithful `Adr2Pos` / `Pos2Adr` behavior.

### Decompiler model

Runs #25-#26 introduced neutral decompiler data types and runtime-tested primitive behavior.

Notable preserved quirk: legacy `AssignItem` copies most fields but does not copy `Offset`.

## Whole-Decompiler TU phase

The major integration transition after #26 was to stop reasoning from slices and generate **one complete portable translation unit from the real `Decompiler.cpp`**.

### Generator

`tests/prepare_portable_decompiler_full.ps1` now:

- generates `Decompiler.portable.h` from real `Decompiler.h` while replacing the `Main.h` dependency with the narrow transitional bridge;
- generates a filtered `Misc.portable.h` that excludes GUI-only canvas/form declarations;
- generates one full `Decompiler.portable.cpp` from real `Decompiler.cpp`;
- removes only the three presentation/orchestration definitions owned by the GUI path;
- transforms reached Borland/System semantics in the generated copy only.

Original `Decompiler.cpp`, `Decompiler.h`, `Misc.h` and `Main.h` remain unchanged.

### Presentation boundary

These definitions remain outside the headless core translation unit:

- `TDecompileEnv::OutputSourceCodeLine()`
- `TDecompileEnv::OutputSourceCode()`
- `TDecompileEnv::DecompileProc()`

This is deliberate. Their behavior belongs to presentation/orchestration and should later be replaced by a headless output/driver layer, not VCL shims.

### Full-TU red-to-green chronology

- #28: first complete `Decompiler.cpp` TU attempt; stopped on excessive GUI/Misc declaration surface and compatibility gaps.
- #29: GUI noise removed; failures narrowed to concrete Borland `String`/Variant seams deep in the real engine.
- #30: only one compile error remained in the entire TU: numeric `String(m + N)`.
- #31: green after adding that final explicit numeric construction mapping.

Run #31 head:

`7addf0cd6116de1b21cc7e224792992428f62106` — `Handle final numeric String construction`

Result: **the complete generated legacy Decompiler translation unit compiles under hosted MSVC x86 and produces `Decompiler.portable.obj`.**

## Transitional compatibility surface

`portable/core/IdrLegacyCompat.h` is a bridge, not the final portable runtime.

Current generated-TU compatibility includes:

- `String = std::string`
- explicit known numeric `String(expr)` conversions to `std::to_string(expr)`
- 1-based `PortableStringPos` and `PortableSubString`
- `.Length()` / `.SetLength()` / `.IsEmpty()` transforms
- narrow `Currency` conversion seam
- narrow String-valued enumeration/Variant seam
- `SameText`, `AnsiReplaceText`, `IntToStr`, `IntToHex`, `QuotedStr` declarations
- embedded-procedure confirmation seam
- form-owned `GetMethodInfo` seam

Do not generalize these into a broad Borland runtime unless runtime tests prove that is the best design.

## Important semantic hazards still open

### 1-based direct indexing

Legacy code contains direct expressions such as `name[1]`, `_name[1]`, etc. With `std::string`, those are 0-based. Compile success does not make them correct.

These sites need either generated translation, a purpose-built string abstraction, or direct source neutralization when runtime execution reaches them.

### Variant / WideString / Currency / Comp

Only narrow reached cases are represented. Their binary layout, conversions and formatting are not yet claimed equivalent to Embarcadero behavior.

### Legacy warnings

The full TU currently emits non-blocking warnings including signed/unsigned mismatches, old CRT functions and large/invalid shift-count warnings. Do not paper over them en masse; classify them when runtime paths become relevant.

### `DecompileTry()`

The known legacy not-all-paths-return behavior remains intentionally untouched pending semantic validation.

## Next frontier: link the whole Decompiler object

`Decompiler.portable.obj` is currently compile-only in the integration workflow.

The next technical move is to add it to the linker input and use unresolved externals as the dependency map.

Expected classes of unresolved symbols:

1. legacy globals/state (`DelphiVersion`, addresses, flags, Infos, KnowledgeBase, etc.);
2. neutral-analysis helpers still implemented only in legacy units;
3. type/metadata/KB services;
4. explicit UI/policy seams (`ManualInput`, confirmation, method lookup);
5. presentation methods intentionally excluded from the core TU.

Resolve them by coherent subsystem boundaries, not one-off fake stubs.

Once the object links, the next runtime milestone is to instantiate enough real Decompiler state to execute a minimal analysis/decompile path against controlled bytes while keeping the existing `dis.dll` runtime regression green.

## Longer path to first useful CLI

1. full `Decompiler.portable.obj` linked into core target;
2. runtime-safe String/RTL behavior for actually executed paths;
3. neutral Infos/type/KB context;
4. PE/image loader and segment population;
5. deterministic headless policies for interactive legacy seams;
6. minimal `idr-cli.exe <target.exe>`;
7. test against known Delphi Win32 samples and compare output with original IDR.

## Working rules

- Compiler/linker/runtime evidence over speculation.
- One coherent commit per logical change where practical.
- Do not disturb the frozen smoke branch.
- Do not push over an active integration run because concurrency cancels the previous run.
- Preserve original legacy files until a deliberate structural port warrants changing them.
- No upstream PR or merge to `main` yet.
