$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('String __fastcall GetString(PITEM item, Byte precedence)')
$end = $src.IndexOf('void __fastcall TDecompiler::InitFlags()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler slice markers not found' }
$body = $src.Substring($start, $end - $start)
$prefix = @'
#include <cstdio>
#include <cstring>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

String __fastcall GetImmString(String TypeName, int Val);
String __fastcall Val2Str0(DWord Val);
bool __fastcall IsValidImageAdr(DWord Adr);
PInfoRec __fastcall GetInfoRec(DWord adr);
String __fastcall MakeGvarName(DWord adr);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.slice.cpp" -Value ($prefix + $body) -Encoding utf8
