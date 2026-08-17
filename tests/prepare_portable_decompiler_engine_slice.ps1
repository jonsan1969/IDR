$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('DWord __fastcall TDecompiler::Decompile(')
$end = $src.IndexOf('DWord __fastcall TDecompiler::DecompileCaseEnum(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler engine slice markers not found' }
$body = $src.Substring($start, $end - $start)

$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;

int __fastcall Adr2Pos(DWord adr);
DWord __fastcall Pos2Adr(int pos);
String __fastcall Val2Str8(DWord val);
bool __fastcall IsFlagSet(DWord flag, int pos);
void __fastcall SetFlag(DWord flag, int pos);
void __fastcall SetFlags(DWord flag, int pos, int num);
void __fastcall ClearFlag(DWord flag, int pos);
int __fastcall GetProcSize(DWord fromAdr);
PInfoRec __fastcall GetInfoRec(DWord adr);
int __fastcall GetNearestUpInstruction(int fromPos);
String __fastcall GetDecompilerRegisterName(int idx);
void __fastcall InitItem(PITEM item);
int __fastcall IsIntOver(DWord fromAdr);
bool __fastcall IsExit(DWord fromAdr);
bool __fastcall IsValidCodeAdr(DWord adr);
bool __fastcall SameText(const String &left, const String &right);
bool __fastcall IsInheritsByProcName(const String &name1, const String &name2);
String __fastcall ExtractProcName(const String &name);
String __fastcall GetDirectCondition(char c);
int __fastcall BranchGetPrevInstructionType(DWord fromAdr, DWord *jmpAdr, PLoopInfo loopInfo);
int __fastcall IsInt64ComparisonViaStack1(DWord fromAdr, int *skip1, DWord *simEnd);
int __fastcall IsInt64ComparisonViaStack2(DWord fromAdr, int *skip1, int *skip2, DWord *simEnd);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp" -Value ($prefix + $body) -Encoding utf8
