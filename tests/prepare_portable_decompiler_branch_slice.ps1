$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('bool __fastcall TDecompileEnv::GetBJLRange(')
$end = $src.IndexOf('void __fastcall TDecompileEnv::UpdateBJLList()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler branch slice markers not found' }
$body = $src.Substring($start, $end - $start)
$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;
int __fastcall Adr2Pos(DWord adr);
bool __fastcall IsFlagSet(DWord flag, int pos);
int __fastcall BranchGetPrevInstructionType(DWord fromAdr, DWord *jmpAdr, PLoopInfo loopInfo);
String __fastcall GetDirectCondition(char c);
String __fastcall GetInvertCondition(char c);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.branch.slice.cpp" -Value ($prefix + $body) -Encoding utf8
