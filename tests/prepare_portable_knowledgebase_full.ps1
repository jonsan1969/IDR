$root = Join-Path $PSScriptRoot '..'
$generated = Join-Path $PSScriptRoot 'generated'
New-Item -ItemType Directory -Force -Path $generated | Out-Null

$src = Get-Content -Raw -LiteralPath (Join-Path $root 'KnowledgeBase.cpp')

# Keep the real KnowledgeBase implementation while removing only VCL/project
# include plumbing. IdrLegacyCompat supplies the data declarations required by
# KnowledgeBase.h/Infos.h without importing Main/TForm.
$src = $src -replace '(?m)^\s*#include\s*<vcl\.h>\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"KnowledgeBase\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Main\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#include\s*"Misc\.h"\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+hdrstop\s*\r?\n', ''
$src = $src -replace '(?m)^\s*#pragma\s+package\([^\r\n]+\)\s*\r?\n', ''

# Narrow Borland String transforms used by KnowledgeBase.cpp. Keep these local
# to this generated TU so we do not create another cross-object helper owner.
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*)\.LastDelimiter\(([^\)\r\n]+)\)', 'PortableKBLastDelimiter($1, $2)'
$src = $src -replace '([A-Za-z_][A-Za-z0-9_]*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableKBSubString($1, $2, $3)'
$src = $src -replace '\.Length\(\)', '.size()'

$prefix = @'
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include "../../portable/core/IdrLegacyCompat.h"

// CleanupList historically came from Main.h. The semantics needed here are
// just ownership cleanup for pointer elements followed by deletion of TList.
template <typename T>
static void PortableKBCleanupList(TList *&list) {
    if (!list) return;
    for (int i = 0; i < list->Count; ++i) delete static_cast<T *>(list->Items[static_cast<std::size_t>(i)]);
    delete list;
    list = nullptr;
}
#define CleanupList PortableKBCleanupList

static int PortableKBLastDelimiter(const String &text, const String &delimiters) {
    const auto pos = text.find_last_of(delimiters);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}

static String PortableKBSubString(const String &text, int start, int count) {
    if (start <= 0 || count <= 0) return "";
    const auto offset = static_cast<std::size_t>(start - 1);
    if (offset >= text.size()) return "";
    return text.substr(offset, static_cast<std::size_t>(count));
}

'@

Set-Content -LiteralPath (Join-Path $generated 'KnowledgeBase.portable.cpp') -Value ($prefix + $src) -Encoding utf8
