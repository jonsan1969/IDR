$root = Join-Path $PSScriptRoot '..'
$generated = Join-Path $PSScriptRoot 'generated'
New-Item -ItemType Directory -Force -Path $generated | Out-Null

function Remove-CppFunction {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Signature
    )

    $start = $Text.IndexOf($Signature, [System.StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Function marker not found: $Signature" }
    $open = $Text.IndexOf('{', $start)
    if ($open -lt 0) { throw "Opening brace not found: $Signature" }

    $depth = 0
    for ($i = $open; $i -lt $Text.Length; $i++) {
        if ($Text[$i] -eq '{') { $depth++ }
        elseif ($Text[$i] -eq '}') {
            $depth--
            if ($depth -eq 0) {
                return $Text.Remove($start, $i - $start + 1)
            }
        }
    }
    throw "Closing brace not found: $Signature"
}

# Generate a Decompiler header that keeps the real class/method declarations
# but replaces Main.h with the narrow transitional compatibility bridge.
$header = Get-Content -Raw -LiteralPath (Join-Path $root 'Decompiler.h')
$header = $header -replace '#include "Main.h"', '#include "../../portable/core/IdrLegacyCompat.h"'
Set-Content -LiteralPath (Join-Path $generated 'Decompiler.portable.h') -Value $header -Encoding utf8

# Generate only the non-GUI declaration surface from Misc.h. The decompiler
# must not acquire VCL canvas/form dependencies merely because Misc.h mixes
# analysis helpers and presentation helpers in the legacy project.
$misc = Get-Content -Raw -LiteralPath (Join-Path $root 'Misc.h')
$misc = $misc -replace '#include "Decompiler.h"', '#include "Decompiler.portable.h"'
$miscLines = $misc -split "`r?`n"
$misc = ($miscLines | Where-Object {
    $_ -notmatch 'TCanvas|TRect|TColor' -and
    $_ -notmatch 'GetOwnTypeByName\s*\('
}) -join "`r`n"
Set-Content -LiteralPath (Join-Path $generated 'Misc.portable.h') -Value $misc -Encoding utf8

$src = Get-Content -Raw -LiteralPath (Join-Path $root 'Decompiler.cpp')

# Presentation/orchestration belongs outside the portable core. Keep the class
# declarations for legacy compatibility, but do not compile these GUI-owned
# definitions into the core translation unit.
$src = Remove-CppFunction $src 'void __fastcall TDecompileEnv::OutputSourceCodeLine('
$src = Remove-CppFunction $src 'void __fastcall TDecompileEnv::OutputSourceCode()'
$src = Remove-CppFunction $src 'void __fastcall TDecompileEnv::DecompileProc()'

# Remove VCL/System and legacy project includes from the generated copy. The
# portable header + declaration surface below replace them for this build only.
$src = $src -replace '(?m)^\s*#include\s*<vcl\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<System\.StrUtils\.hpp>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Decompiler\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Main\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Misc\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"TypeInfo2\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"InputDlg\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+hdrstop\s*\r?\n', ''

# Preserve Embarcadero String semantics only where the old code relies on
# methods std::string does not have. These transforms are confined to the
# generated integration TU; original sources remain untouched.
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$src = $src -replace '\.Length\(\)', '.size()'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.IsEmpty\(\)', '$1.empty()'
$src = $src -replace '\bTrue\b', 'true'
$src = $src -replace '\bFalse\b', 'false'

# Known numeric String(...) construction sites reached by the legacy engine.
# Keep this explicit: String(char*) must remain text construction.
$numericStringArgs = @(
    '_div', '_mod', '_pow2', '_offset', '-_offset', '_imm', '_argsNum', '_retBytes',
    '_cnt', '_k', '-_k', '_N', '_N1', '_N2', '_N1 - _N2', '_N1 - 1',
    'm', 'm + 1', 'n + N', 'n + N + 1',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_item4.IntValue',
    'item.IntValue', 'item1.IntValue', 'item2.IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale',
    '_disInfo.Immediate', '_disInfo.Offset', '_disInfo.Scale',
    'intTo - 1', 'intTo + 1', '_offset + 1',
    'Env->Stack[varIdxInfo.IdxValue].IntValue',
    'Env->Stack[cntIdxInfo.IdxValue].IntValue'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $src = $src -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}
$src = $src -replace 'CmpInfo\.R = 0;', 'CmpInfo.R = "0";'
$src = $src -replace 'GetDecompilerRegisterName\(_reg1Idx\) \+ " := " \+ _offset \+ ";"', 'GetDecompilerRegisterName(_reg1Idx) + " := " + std::to_string(_offset) + ";"'
$src = $src -replace '_line \+= _item2\.IntValue;', '_line += std::to_string(_item2.IntValue);'
$src = $src -replace '_currVal\.operator String\(\)', 'PortableCurrencyToString(_currVal)'

# GUI-owned analysis hooks become narrow compile-time seams in the generated TU.
$src = $src -replace 'FMain_11011981->GetMethodInfo\(', 'PortableGetMethodInfo('
$src = $src -replace 'int _savedIdx = FMain_11011981->lbCode->ItemIndex;', 'int _savedIdx = -1;'
$src = $src -replace 'FMain_11011981->lbCode->ItemIndex = -1;', '/* portable: no GUI selection state */'
$src = $src -replace 'FMain_11011981->lbCode->ItemIndex = _savedIdx;', '/* portable: no GUI selection state */'
$src = $src -replace '(?s)if \(Application->MessageBox\(\s*String\("Decompile embedded procedure at address " \+ _embAdr \+ "\?"\)\.c_str\(\), L"Confirmation",\s*MB_YESNO\) == IDYES\)', 'if (PortableConfirmEmbeddedProcedure(_embAdr))'

$prefix = @'
#include <cassert>
#include <string>
#include "../../portable/core/IdrLegacyCompat.h"
#include "Decompiler.portable.h"
#include "Misc.portable.h"

#ifndef MAXSTRBUFFER
#define MAXSTRBUFFER 10000
#endif

int PortableStringPos(const String &text, const String &needle) {
    const auto pos = text.find(needle);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}
String PortableSubString(const String &text, int start, int count) {
    if (start <= 0 || count <= 0) return "";
    const auto offset = static_cast<std::size_t>(start - 1);
    if (offset >= text.size()) return "";
    return text.substr(offset, static_cast<std::size_t>(count));
}
bool PortableConfirmEmbeddedProcedure(const String &address);
PMethodRec PortableGetMethodInfo(DWord classAdr, char kind, int offset);
String PortableCurrencyToString(const Currency &value);

String __fastcall IntToStr(__int64 value);
String __fastcall IntToHex(__int64 value, int digits);
String __fastcall QuotedStr(const String &value);
String __fastcall AnsiReplaceText(const String &text, const String &from, const String &to);
bool __fastcall SameText(const String &left, const String &right);
template <typename T> String FloatToStr(T value);

'@

Set-Content -LiteralPath (Join-Path $generated 'Decompiler.portable.cpp') -Value ($prefix + $src) -Encoding utf8
