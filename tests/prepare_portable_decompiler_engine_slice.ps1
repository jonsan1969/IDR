$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('DWord __fastcall TDecompiler::Decompile(')
$end = $src.IndexOf('DWord __fastcall TDecompiler::DecompileCaseEnum(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler engine slice markers not found' }
$body = $src.Substring($start, $end - $start)

$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp" -Value ($prefix + $body) -Encoding utf8
