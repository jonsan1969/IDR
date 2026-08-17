$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"

function Get-MethodBody([string]$signature, [string]$prefix) {
    $name = $src.IndexOf($signature)
    $start = if ($name -ge 0) { $src.LastIndexOf($prefix, $name) } else { -1 }
    $end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
    if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
        throw "Decompiler special slice markers not found for $signature (name=$name start=$start end=$end)"
    }
    return $src.Substring($start, $end - $start)
}

$bodyPush = Get-MethodBody 'TDecompiler::SimulatePush(' 'void __fastcall '
$bodyPop = Get-MethodBody 'TDecompiler::SimulatePop(' 'void __fastcall '
$bodyFloat = Get-MethodBody 'TDecompiler::SimulateFloatInstruction(' 'void __fastcall '
$bodyFormat = Get-MethodBody 'TDecompiler::SimulateFormatCall(' 'void __fastcall '
$bodyMarkCase = Get-MethodBody 'TDecompiler::MarkCaseEnum(' 'void __fastcall '
$bodyMarkGeneral = Get-MethodBody 'TDecompiler::MarkGeneralCase(' 'void __fastcall '
$bodyGeneralCase = Get-MethodBody 'TDecompiler::DecompileGeneralCase(' 'DWord __fastcall '
$bodyTry = Get-MethodBody 'TDecompiler::DecompileTry(' 'DWord __fastcall '
$bodyConditions = Get-MethodBody 'TDecompiler::AnalyzeConditions(' 'int __fastcall '

# Coverage audit #97 identified these eight remaining TDecompiler helpers.
# GetArrayFieldOffset has a source line break after __fastcall, so deliberately
# omit the trailing space from its return-type marker.
$bodyArrayField = Get-MethodBody 'TDecompiler::GetArrayFieldOffset(' 'int __fastcall'
$bodyCycleFrom = Get-MethodBody 'TDecompiler::GetCycleFrom(' 'String __fastcall '
$bodyCycleIdx = Get-MethodBody 'TDecompiler::GetCycleIdx(' 'void __fastcall '
$bodyCycleTo = Get-MethodBody 'TDecompiler::GetCycleTo(' 'String __fastcall '
$bodyFloatStack = Get-MethodBody 'TDecompiler::GetFloatItemFromStack(' 'void __fastcall '
$bodyInt64Stack = Get-MethodBody 'TDecompiler::GetInt64ItemFromStack(' 'void __fastcall '
$bodyMemItem = Get-MethodBody 'TDecompiler::GetMemItem(' 'void __fastcall '
$bodyStringArgument = Get-MethodBody 'TDecompiler::GetStringArgument(' 'String __fastcall '

$body = $bodyPush + $bodyPop + $bodyFloat + $bodyFormat + $bodyMarkCase + $bodyMarkGeneral + $bodyGeneralCase + $bodyTry + $bodyConditions +
        $bodyArrayField + $bodyCycleFrom + $bodyCycleIdx + $bodyCycleTo + $bodyFloatStack + $bodyInt64Stack + $bodyMemItem + $bodyStringArgument

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)
$prefix += @'

String __fastcall GetGvarName(DWord adr);
String __fastcall GetInvertCondition(char c);
bool __fastcall GetArrayIndexes(String typeName, int dim, int *lowIdx, int *highIdx);
int __fastcall GetClassSize(DWord adr);
String __fastcall FloatToStr(float value);
String __fastcall FloatToStr(double value);
String __fastcall FloatToStr(long double value);
String __fastcall FloatToStr(Comp value);
String __fastcall FloatToStr(Currency value);
// C++Builder Variant accepted textual values implicitly. Keep that conversion
// as an explicit smoke overload until the portable Variant boundary is real.
String __fastcall GetEnumerationString(String typeName, String value);

'@

$numericStringArgs = @(
    '_offset', '_foffset', '_fofs', '_pow2', '_mod', '_size', '_sz', '_ofs', '_pos', '_idx', '_idx1', '_lIdx', '_hIdx', '_cnt', '_classSize', '_ap', '_adr', '_ea', '_cmpRes', '_len', '_N', '_N1', '_k',
    'item.IntValue', '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_itemBase.IntValue', '_itemSrc.IntValue', '_itemDst.IntValue',
    'Env->Stack[varIdxInfo.IdxValue].IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale', 'ADisInfo->Immediate', 'ADisInfo->Offset', 'ADisInfo->Scale'
)
foreach ($arg in $numericStringArgs) {
    $body = $body.Replace("String($arg)", "std::to_string($arg)")
}

# Exact arithmetic forms used by the covered helpers. Keep these narrow so
# pointer/character String constructors remain untouched.
$body = $body.Replace('String(_N1 - _N2)', 'std::to_string(_N1 - _N2)')
$body = $body.Replace('String(_N1 - 1)', 'std::to_string(_N1 - 1)')
$body = $body.Replace('String(intTo - 1)', 'std::to_string(intTo - 1)')
$body = $body.Replace('String(intTo + 1)', 'std::to_string(intTo + 1)')
$body = $body.Replace('String(-_offset)', 'std::to_string(-_offset)')
$body = $body.Replace('String(_offset + 1)', 'std::to_string(_offset + 1)')
$body = $body.Replace('String(-_k)', 'std::to_string(-_k)')
$body = $body.Replace('String(_offset - _foffset)', 'std::to_string(_offset - _foffset)')

# Currency in C++Builder exposes a textual conversion operator. Keep this as a
# compile-smoke-only numeric rendering until the real portable Currency type exists.
$body = $body.Replace('_currVal.operator String()', 'std::to_string(_currVal.Val)')

$body = $body -replace '(?<![A-Za-z0-9_])True(?![A-Za-z0-9_])', 'true'
$body = $body -replace '(?<![A-Za-z0-9_])False(?![A-Za-z0-9_])', 'false'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$body = $body -replace 'CmpInfo\.R\s*=\s*0\s*;', 'CmpInfo.R = "0";'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.pushpop.slice.cpp" -Value ($prefix + $body) -Encoding utf8

& "$PSScriptRoot\audit_portable_decompiler_coverage.ps1"
