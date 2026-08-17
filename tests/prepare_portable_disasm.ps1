$ErrorActionPreference = 'Stop'

$source = Get-Content -Raw -LiteralPath 'Disasm.cpp'

# Strip C++Builder/VCL-only includes. Windows headers stay in the source.
$source = $source -replace '(?m)^#include <vcl\.h>\r?\n', ''
$source = $source -replace '(?m)^#pragma hdrstop\r?\n', ''
$source = $source -replace '(?m)^#include <mem\.h>\r?\n', ''
$source = $source -replace '(?m)^#include <SyncObjs\.hpp>\r?\n', ''

# Borland uses asm { ... }; 32-bit MSVC uses __asm { ... }.
$source = $source -replace '(?m)^(\s*)asm\s*$', '$1__asm'

# The Borland-specific @label spelling is not needed by this translation unit.
$source = $source -replace '(?s)#ifdef __clang__\s*#define LABEL\(x\) x\s*#else\s*#define LABEL\(x\) @##x\s*#endif', '#define LABEL(x) x'

$shim = @'
#include <Windows.h>
#include <cstdint>
#include <mutex>
#include <cctype>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;

#ifndef stricmp
#define stricmp _stricmp
#endif

class TCriticalSection {
public:
    void Enter() { mutex_.lock(); }
    void Leave() { mutex_.unlock(); }
private:
    std::recursive_mutex mutex_;
};
'@

New-Item -ItemType Directory -Force -Path 'tests/generated' | Out-Null
Set-Content -LiteralPath 'tests/generated/Disasm.portable.cpp' -Value ($shim + "`r`n" + $source) -Encoding utf8
