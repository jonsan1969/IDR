$root = Join-Path $PSScriptRoot '..'
$generated = Join-Path $PSScriptRoot 'generated'
New-Item -ItemType Directory -Force -Path $generated | Out-Null

function Remove-CppFunction {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Signature
    )

    $start = $Text.IndexOf($Signature, [System.StringComparison]::Ordinal)
    if ($start -lt 0) { return $Text }
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

$src = Get-Content -Raw -LiteralPath (Join-Path $root 'Misc.cpp')

# Strip platform/VCL includes. The generated declaration surface from the full
# Decompiler preparation already supplies the legacy core declarations.
$src = $src -replace '(?m)^\s*#include\s*<vcl\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<minwindef\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<dbghelp\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<Vcl\.Clipbrd\.hpp>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Misc\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"InputDlg\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+hdrstop\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+package\([^\r\n]+\)\s*\r?\n', ''

# These functions are already supplied by the neutral bridge. Remove the real
# legacy definitions from this generated TU so one link target has one owner.
$bridgeOwned = @(
    'int __fastcall Adr2Pos(DWord adr)',
    'DWord __fastcall Pos2Adr(int Pos)',
    'String __fastcall GetDefaultProcName(DWord adr)',
    'String __fastcall ExtractClassName(const String &AName)',
    'String __fastcall ExtractProcName(const String &AName)',
    'String __fastcall ExtractName(const String &AName)',
    'String __fastcall ExtractType(const String &AName)',
    'String __fastcall TrimTypeName(const String &TypeName)',
    'String __fastcall MakeGvarName(DWord adr)',
    'bool __fastcall IsFlagSet(DWord flag, int pos)',
    'void __fastcall SetFlag(DWord flag, int pos)',
    'void __fastcall SetFlags(DWord flag, int pos, int num)',
    'void __fastcall ClearFlag(DWord flag, int pos)',
    'int __fastcall GetNearestUpInstruction(int fromPos)',
    'int __fastcall GetNearestUpInstruction(int fromPos, int toPos)'
)
foreach ($signature in $bridgeOwned) {
    $src = Remove-CppFunction $src $signature
}

# Presentation/UI-only functions stay outside the portable core.
$guiOwned = @(
    'void __fastcall ScaleForm(TForm *AForm)',
    'void __fastcall Copy2Clipboard(',
    'String __fastcall InputDialogExec(',
    'String __fastcall ManualInput(',
    'void __fastcall SaveCanvas(',
    'void __fastcall RestoreCanvas(',
    'void __fastcall DrawOneItem('
)
foreach ($signature in $guiOwned) {
    $src = Remove-CppFunction $src $signature
}

# Narrow Embarcadero String compatibility transforms for the generated copy.
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Trim\(\)', 'PortableTrim($1)'
$src = $src -replace '\.Length\(\)', '.size()'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.IsEmpty\(\)', '$1.empty()'
$src = $src -replace '\bTrue\b', 'true'
$src = $src -replace '\bFalse\b', 'false'

$prefix = @'
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include "../../portable/core/IdrLegacyCompat.h"
#include "Decompiler.portable.h"
#include "Misc.portable.h"

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
String PortableTrim(const String &text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    if (first == text.end()) return "";
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return String(first, last);
}

'@

Set-Content -LiteralPath (Join-Path $generated 'Misc.portable.cpp') -Value ($prefix + $src) -Encoding utf8
