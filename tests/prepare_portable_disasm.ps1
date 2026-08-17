$ErrorActionPreference = 'Stop'

$source = Get-Content -Raw -LiteralPath 'Disasm.cpp'

$source = $source -replace '(?m)^#include <vcl\.h>\r?\n', ''
$source = $source -replace '(?m)^#pragma hdrstop\r?\n', ''
$source = $source -replace '(?m)^#include <mem\.h>\r?\n', ''
$source = $source -replace '(?m)^#include <SyncObjs\.hpp>\r?\n', ''
$source = $source -replace '(?m)^(\s*)asm\s*$', '$1__asm'
$source = $source -replace '(?s)#ifdef __clang__\s*#define LABEL\(x\) x\s*#else\s*#define LABEL\(x\) @##x\s*#endif', '#define LABEL(x) x'
$source = $source -replace 'extern TCriticalSection\* CrtSection;', "static TCriticalSection PortableCrtSection;`r`nTCriticalSection* CrtSection = &PortableCrtSection;"

# The checked-in legacy Init() dereferences PdisNew before resolving the DLL
# exports. Keep the original source untouched, but make the generated portable
# translation unit initialize the backend in a valid order so CI can exercise
# real Disassemble() calls against the repository's x86 dis.dll.
$portableInit = @'
int __fastcall MDisasm::Init()
{
    hModule = LoadLibraryA("dis.dll");
    if (!hModule) return 0;

    PdisNew = (DWord * (__stdcall *) (int)) GetProcAddress(hModule, "?PdisNew@DIS@@SGPAV1@W4DIST@1@@Z");
    CchFormatInstr = (DWord(_stdcall *)(char *, DWord)) GetProcAddress(hModule, "?CchFormatInstr@DIS@@QBEIPADI@Z");
    Dist = (DWord(_stdcall *)()) GetProcAddress(hModule, "?Dist@DIS@@QBE?AW4DIST@1@XZ");
    if (!PdisNew || !CchFormatInstr || !Dist) return 0;

    DIS = PdisNew(1);
    return DIS ? 1 : 0;
}
'@
$source = [regex]::Replace(
    $source,
    '(?s)int __fastcall MDisasm::Init\(\)\s*\{.*?\n\}\s*//---------------------------------------------------------------------------',
    $portableInit + "`r`n//---------------------------------------------------------------------------",
    1)

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
