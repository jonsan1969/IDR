$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('DWord __fastcall TDecompiler::Decompile(')
$end = $src.IndexOf('DWord __fastcall TDecompiler::DecompileCaseEnum(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler engine slice markers not found' }
$body = $src.Substring($start, $end - $start)

# Preserve observed Embarcadero numeric String construction explicitly in the
# generated smoke copy. Original Decompiler.cpp remains unchanged.
$body = $body -replace 'String\(_div\)', 'std::to_string(_div)'
$body = $body -replace 'String\(_mod\)', 'std::to_string(_mod)'
$body = $body -replace 'String\(_item\.IntValue\)', 'std::to_string(_item.IntValue)'

# Embarcadero String::Pos() is 1-based and returns 0 when not found.
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'

# The original engine directly asks the VCL UI whether an embedded procedure
# should be decompiled and temporarily changes lbCode selection. Keep the
# analysis path in the smoke copy, but replace only that UI plumbing with a
# future-policy-shaped callback. Do not invent fake VCL form/application types.
$body = $body -replace 'int _savedIdx = FMain_11011981->lbCode->ItemIndex;', 'int _savedIdx = -1;'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = -1;', '// portable smoke: no GUI selection state'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = _savedIdx;', '// portable smoke: no GUI selection state'
$body = $body -replace '(?s)if \(Application->MessageBox\(\s*String\("Decompile embedded procedure at address " \+ _embAdr \+ "\?"\)\.c_str\(\), L"Confirmation",\s*MB_YESNO\) == IDYES\)', 'if (PortableConfirmEmbeddedProcedure(_embAdr))'

$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;
extern DWord CurProcAdr;
extern int DelphiVersion;
extern MKnowledgeBase KnowledgeBase;

int PortableStringPos(const String &text, const String &needle) {
    auto pos = text.find(needle);
    return pos == String::npos ? 0 : static_cast<int>(pos) + 1;
}
bool PortableConfirmEmbeddedProcedure(const String &address);

String __fastcall IntToStr(__int64 value);
String __fastcall IntToHex(__int64 value, int digits);
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
String __fastcall GetString(PITEM item, Byte precedence);
int __fastcall IsIntOver(DWord fromAdr);
bool __fastcall IsExit(DWord fromAdr);
bool __fastcall IsValidCodeAdr(DWord adr);
bool __fastcall SameText(const String &left, const String &right);
bool __fastcall IsInheritsByProcName(const String &name1, const String &name2);
String __fastcall ExtractProcName(const String &name);
String __fastcall ExtractClassName(const String &name);
String __fastcall GetDirectCondition(char c);
int __fastcall BranchGetPrevInstructionType(DWord fromAdr, DWord *jmpAdr, PLoopInfo loopInfo);
bool __fastcall IsXorMayBeSkipped(DWord fromAdr);
int __fastcall IsBoundErr(DWord fromAdr);
int __fastcall IsInlineLengthCmp(DWord fromAdr);
int __fastcall IsInlineLengthTest(DWord fromAdr);
DWord __fastcall IsGeneralCase(DWord fromAdr, int retAdr);
int __fastcall IsInlineDiv(DWord fromAdr, int *div);
int __fastcall IsInlineMod(DWord fromAdr, int *mod);
int __fastcall IsCopyDynArrayToStack(DWord fromAdr);
int __fastcall IsInt64Comparison(DWord fromAdr, int *skip1, int *skip2, bool *immVal, __int64 *val);
int __fastcall IsInt64ComparisonViaStack1(DWord fromAdr, int *skip1, DWord *simEnd);
int __fastcall IsInt64ComparisonViaStack2(DWord fromAdr, int *skip1, int *skip2, DWord *simEnd);
int __fastcall IsInt64Shl(DWord fromAdr);
int __fastcall IsInt64Shr(DWord fromAdr);
int __fastcall IsAbs(DWord fromAdr);
Byte __fastcall GetTypeKind(String name, int *size);
int __fastcall GetRecordSize(String name);
int __fastcall GetArraySize(String type);
int __fastcall GetArrayElementTypeSize(String type);
String __fastcall GetArrayElementType(String type);
String __fastcall ManualInput(DWord procAdr, DWord curAdr, String caption, String labelText);
int __fastcall GetProcRetBytes(MProcInfo *pInfo);
String __fastcall GetDefaultProcName(DWord adr);
DWord __fastcall GetClassAdr(const String &name);
String __fastcall GetDynaInfo(DWord adr, Word id, DWord *dynAdr);
void __fastcall AddPicode(int pos, Byte op, String name, int ofs);
void __fastcall FillArgInfo(int k, Byte callkind, PARGINFO argInfo, Byte **p, int *s);
String __fastcall GetTypeDeref(String typeName);
String __fastcall GetImmString(int val);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp" -Value ($prefix + $body) -Encoding utf8
