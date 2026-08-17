$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('DWord __fastcall TDecompiler::Decompile(')
$end = $src.IndexOf('DWord __fastcall TDecompiler::DecompileCaseEnum(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler engine slice markers not found' }
$body = $src.Substring($start, $end - $start)

# Preserve observed Embarcadero numeric String construction explicitly in the
# generated smoke copy. The negative lookbehind is important: an earlier
# literal replacement also matched the tail of GetImmString(...).
$numericStringArgs = @(
    '_div', '_mod', '_pow2', '_offset', '_argsNum', '_retBytes',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}

# Map the Embarcadero String methods actually reached by the engine while
# preserving their 1-based semantics where std::string differs.
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'

# Embedded-procedure confirmation is policy, not GUI. Remove only the VCL
# plumbing in the generated copy and expose a future-core-shaped callback.
$body = $body -replace 'int _savedIdx = FMain_11011981->lbCode->ItemIndex;', 'int _savedIdx = -1;'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = -1;', '// portable smoke: no GUI selection state'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = _savedIdx;', '// portable smoke: no GUI selection state'
$body = $body -replace '(?s)if \(Application->MessageBox\(\s*String\("Decompile embedded procedure at address " \+ _embAdr \+ "\?"\)\.c_str\(\), L"Confirmation",\s*MB_YESNO\) == IDYES\)', 'if (PortableConfirmEmbeddedProcedure(_embAdr))'

# Virtual-method lookup also lives on the VCL main form even though the lookup
# itself is analysis logic. Expose it as a neutral dependency for this smoke.
$body = $body -replace 'FMain_11011981->GetMethodInfo\(', 'PortableGetMethodInfo('

$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;
extern DWord CurProcAdr;
extern int DelphiVersion;
extern int cVmtSelfPtr;
extern MKnowledgeBase KnowledgeBase;
extern TStringList *BSSInfos;

int PortableStringPos(const String &text, const String &needle) {
    auto pos = text.find(needle);
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

String __fastcall IntToStr(__int64 value);
String __fastcall IntToHex(__int64 value, int digits);
String __fastcall QuotedStr(const String &value);
int __fastcall Adr2Pos(DWord adr);
DWord __fastcall Pos2Adr(int pos);
String __fastcall Val2Str0(DWord val);
String __fastcall Val2Str8(DWord val);
bool __fastcall IsFlagSet(DWord flag, int pos);
void __fastcall SetFlag(DWord flag, int pos);
void __fastcall SetFlags(DWord flag, int pos, int num);
void __fastcall ClearFlag(DWord flag, int pos);
int __fastcall GetProcSize(DWord fromAdr);
PInfoRec __fastcall GetInfoRec(DWord adr);
int __fastcall GetNearestUpInstruction(int fromPos);
int __fastcall GetNearestUpInstruction(int fromPos, int toPos);
String __fastcall GetDecompilerRegisterName(int idx);
void __fastcall InitItem(PITEM item);
String __fastcall GetString(PITEM item, Byte precedence);
int __fastcall IsIntOver(DWord fromAdr);
bool __fastcall IsExit(DWord fromAdr);
bool __fastcall IsValidCodeAdr(DWord adr);
bool __fastcall IsValidImageAdr(DWord adr);
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
bool __fastcall IsSameRegister(int idx1, int idx2);
Byte __fastcall GetTypeKind(String name, int *size);
int __fastcall GetRecordSize(String name);
int __fastcall GetArraySize(String type);
int __fastcall GetArrayElementTypeSize(String type);
String __fastcall GetArrayElementType(String type);
String __fastcall ManualInput(DWord procAdr, DWord curAdr, String caption, String labelText);
int __fastcall GetProcRetBytes(MProcInfo *pInfo);
String __fastcall GetDefaultProcName(DWord adr);
DWord __fastcall GetClassAdr(const String &name);
DWord __fastcall GetChildAdr(DWord adr);
String __fastcall GetDynaInfo(DWord adr, Word id, DWord *dynAdr);
void __fastcall AddPicode(int pos, Byte op, String name, int ofs);
void __fastcall FillArgInfo(int k, Byte callkind, PARGINFO argInfo, Byte **p, int *s);
String __fastcall GetTypeDeref(String typeName);
String __fastcall TrimTypeName(const String &typeName);
String __fastcall GetImmString(int val);
String __fastcall GetImmString(String typeName, int val);
String __fastcall TransformString(char *str, int len);
String __fastcall TransformUString(Word codePage, const wchar_t *data, int len);
String __fastcall GetEnumerationString(String typeName, DWord val);
String __fastcall GetSetString(String typeName, Byte *valAdr);
int __fastcall FloatNameToFloatType(String name);
String __fastcall MakeGvarName(DWord adr);
PInfoRec __fastcall AddToBSSInfos(DWord adr, String name, String typeName);
int __fastcall GetField(String typeName, int offset, String &name, String &type);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp" -Value ($prefix + $body) -Encoding utf8
