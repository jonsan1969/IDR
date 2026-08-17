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

$src = $src -replace '(?m)^\s*#include\s*<vcl\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<minwindef\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<dbghelp\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*<Vcl\.Clipbrd\.hpp>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Misc\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"InputDlg\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+hdrstop\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+package\([^\r\n]+\)\s*\r?\n', ''

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
foreach ($signature in $bridgeOwned) { $src = Remove-CppFunction $src $signature }

$guiOwned = @(
    'void __fastcall ScaleForm(TForm *AForm)',
    'void __fastcall Copy2Clipboard(',
    'String __fastcall InputDialogExec(',
    'String __fastcall ManualInput(',
    'void __fastcall SaveCanvas(',
    'void __fastcall RestoreCanvas(',
    'void __fastcall DrawOneItem(',
    'void __fastcall OutputDecompilerHeader(',
    'String __fastcall GetModuleVersion(',
    'bool __fastcall IsBplByExport('
)
foreach ($signature in $guiOwned) { $src = Remove-CppFunction $src $signature }

$src = $src -replace '(?m)^\s*TColor\s+SavedPenColor;\s*\r?\n', ''
$src = $src -replace '(?m)^\s*TColor\s+SavedBrushColor;\s*\r?\n', ''
$src = $src -replace '(?m)^\s*TColor\s+SavedFontColor;\s*\r?\n', ''
$src = $src -replace '(?m)^\s*TBrushStyle\s+SavedBrushStyle;\s*\r?\n', ''

$src = $src -replace 'FMain_11011981->EstimateProcSize\(([^\)]+)\)', 'PortableEstimateProcSize($1)'
$src = $src -replace 'FMain_11011981->WrkDir', 'PortableWorkDir()'

$src = $src -creplace 'String\(b\)\.Trim\(\)', 'PortableTrim(String(b))'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Trim\(\)', 'PortableTrim($1)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*)\.LastDelimiter\(([^\)\r\n]+)\)', 'PortableLastDelimiter($1, $2)'
$src = $src -replace '\.Length\(\)', '.size()'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.IsEmpty\(\)', '$1.empty()'
$src = $src -replace '\bTrue\b', 'true'
$src = $src -replace '\bFalse\b', 'false'
$src = $src -replace '\bStrToInt\(', 'PortableStrToInt('
$src = $src -replace '\bTryStrToInt\(', 'PortableTryStrToInt('
$src = $src -replace '\bCompareText\(', 'PortableCompareText('
$src = $src -replace '(?<!Portable)\bTrim\(', 'PortableTrim('
$src = $src -replace 'UpperCase\(Reg32Tab\[([^\]]+)\]\)', 'PortableUpperCase(String(Reg32Tab[$1]))'
$src = $src -creplace 'String\(Idx\s*-\s*31\)', 'std::to_string(Idx - 31)'
$src = $src -creplace 'String\(\(int\)\s*Val\)', 'std::to_string(static_cast<int>(Val))'
$src = $src -creplace 'String\(static_cast<int>\(([^\)]+)\)\)', 'std::to_string(static_cast<int>($1))'
$src = $src -creplace 'String\(b\)\.PortableTrim\(\)', 'PortableTrim(String(b))'

$src = $src -replace 'if \(Val\.Type\(\) == varString\) return VarToStr\(Val\);', ''
$src = $src -replace 'return VarToStr\(Val\);', 'return std::to_string(static_cast<long long>(Val));'
$src = $src -replace 'Format\("''%s''", ARRAYOFCONST\(\(\(Char\)Val\)\)\)', 'PortableQuotedChar(Val)'
$src = $src -replace 'p = AnsiString\(tInfo\.Decl\)\.c_str\(\);', 'String _portableEnumDecl = tInfo.Decl; p = _portableEnumDecl.data();'
$src = $src -replace 'pDecl = AnsiString\(tInfo\.Decl\)\.c_str\(\);', 'String _portableSetDecl = tInfo.Decl; pDecl = _portableSetDecl.data();'

$prefix = @'
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <string>
#include "../../portable/core/IdrLegacyCompat.h"
#include "Decompiler.portable.h"
#include "Misc.portable.h"

#ifndef MAXSTRBUFFER
#define MAXSTRBUFFER 10000
#endif

struct SegmentInfo { DWord Start; DWord Size; DWord Flags; String Name; };
using PSegmentInfo = SegmentInfo *;
struct TypeRec { Byte kind; DWord adr; String name; };
using PTypeRec = TypeRec *;
struct CaseInfo { int caseno; int count; };
struct VmtListRec { int height; DWord vmtAdr; String vmtName; };
using PVmtListRec = VmtListRec *;
struct ExportNameRec { String name; DWord address; Word ord; };
using PExportNameRec = ExportNameRec *;
struct ImportNameRec { String module; String name; DWord address; };
using PImportNameRec = ImportNameRec *;
struct UnitRec {
    bool trivial = false;
    bool trivialIni = false;
    bool trivialFin = false;
    bool kb = false;
    int fromAdr = 0;
    int toAdr = 0;
    int finadr = 0;
    int finSize = 0;
    int iniadr = 0;
    int iniSize = 0;
    float matchedPercent = 0;
    int iniOrder = 0;
    TStringList *names = nullptr;
};
using PUnitRec = UnitRec *;

PTypeRec __fastcall GetOwnTypeByName(String AName);
String __fastcall IntToHex(__int64 value, int digits);
bool __fastcall SameText(const String &left, const String &right);
String __fastcall QuotedStr(const String &value);
int PortableEstimateProcSize(DWord address);
String PortableWorkDir();

static int PortableStringPos(const String &text, const String &needle) {
    const auto pos = text.find(needle);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}
static String PortableSubString(const String &text, int start, int count) {
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
int PortableLastDelimiter(const String &text, const String &delimiters) {
    const auto pos = text.find_last_of(delimiters);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}
String PortableUpperCase(String text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return text;
}
int PortableStrToInt(const String &text) {
    if (text.empty()) return 0;
    const int base = text[0] == '$' ? 16 : 10;
    const char *start = text.c_str() + (base == 16 ? 1 : 0);
    return static_cast<int>(std::strtol(start, nullptr, base));
}
bool PortableTryStrToInt(const String &text, int &value) {
    if (text.empty()) return false;
    char *end = nullptr;
    const int base = text[0] == '$' ? 16 : 10;
    const char *start = text.c_str() + (base == 16 ? 1 : 0);
    const long parsed = std::strtol(start, &end, base);
    if (end == start || *end != '\0') return false;
    value = static_cast<int>(parsed);
    return true;
}
int PortableCompareText(const String &left, const String &right) {
    String a = left;
    String b = right;
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}
String PortableQuotedChar(int value) {
    String result = "'";
    result.push_back(static_cast<char>(value));
    result.push_back('\'');
    return result;
}

'@

Set-Content -LiteralPath (Join-Path $generated 'Misc.portable.cpp') -Value ($prefix + $src) -Encoding utf8
