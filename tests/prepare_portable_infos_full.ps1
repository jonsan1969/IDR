$root = Join-Path $PSScriptRoot '..'
$generated = Join-Path $PSScriptRoot 'generated'
New-Item -ItemType Directory -Force -Path $generated | Out-Null

$src = Get-Content -Raw -LiteralPath (Join-Path $root 'Infos.cpp')

# Keep the real Infos implementation; remove only VCL/project include plumbing.
$src = $src -replace '(?m)^\s*#include\s*<vcl\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Infos\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Main\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Misc\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+hdrstop\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+package\([^\r\n]+\)\s*\r?\n', ''

# Replace the one remaining Main-form data dependency with the same explicit
# headless session seam used by the full Misc TU.
$src = $src -replace 'FMain_11011981->WrkDir', 'PortableWorkDir()'

# Narrow Borland String transforms used by Infos.cpp.
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableInfosStringPos($1, $2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableInfosSubString($1, $2, $3)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Trim\(\)', 'PortableInfosTrim($1)'
$src = $src -creplace 'String\(([^\r\n\)]*)\)\.Trim\(\)', 'PortableInfosTrim(String($1))'
$src = $src -creplace 'String\(argInfo->Size\)', 'std::to_string(argInfo->Size)'
$src = $src -replace '\.Length\(\)', '.size()'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.IsEmpty\(\)', '$1.empty()'
$src = $src -replace '\bTrue\b', 'true'
$src = $src -replace '\bFalse\b', 'false'

$prefix = @'
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <string>
#include "../../portable/core/IdrLegacyCompat.h"

#ifndef MAXSTRBUFFER
#define MAXSTRBUFFER 10000
#endif

// Misc helpers historically reached through Misc.h. Their real definitions
// are supplied by the generated Misc TU / legacy bridge at link time.
bool __fastcall SameText(const String &left, const String &right);
String __fastcall TrimTypeName(const String &typeName);
String __fastcall ExtractClassName(const String &name);
String __fastcall ExtractProcName(const String &name);
String __fastcall ExtractName(const String &name);
String __fastcall ExtractType(const String &name);
DWord __fastcall Pos2Adr(int pos);
bool __fastcall IsFlagSet(DWord flag, int pos);
String __fastcall IntToHex(__int64 value, int digits);
String __fastcall QuotedStr(const String &value);
int __fastcall FieldsCmpFunction(void *item1, void *item2);
Byte __fastcall GetTypeKind(String typeName, int *size);
int __fastcall StrGetRecordFieldOffset(String str);
String __fastcall StrGetRecordFieldName(String str);
String __fastcall StrGetRecordFieldType(String str);
String __fastcall GetDefaultProcName(DWord adr);
String __fastcall Val2Str0(DWord value);
String __fastcall SanitizeName(String name);
String PortableWorkDir();

template <typename T>
static void PortableInfosCleanupList(TList *&list) {
    if (!list) return;
    for (int i = 0; i < list->Count; ++i)
        delete static_cast<T *>(list->Items[static_cast<std::size_t>(i)]);
    delete list;
    list = nullptr;
}
#define CleanupList PortableInfosCleanupList

static int PortableInfosStringPos(const String &text, const String &needle) {
    const auto pos = text.find(needle);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}

static String PortableInfosSubString(const String &text, int start, int count) {
    if (start <= 0 || count <= 0) return "";
    const auto offset = static_cast<std::size_t>(start - 1);
    if (offset >= text.size()) return "";
    return text.substr(offset, static_cast<std::size_t>(count));
}

static String PortableInfosTrim(const String &text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    if (first == text.end()) return "";
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return String(first, last);
}

'@

Set-Content -LiteralPath (Join-Path $generated 'Infos.portable.cpp') -Value ($prefix + $src) -Encoding utf8
