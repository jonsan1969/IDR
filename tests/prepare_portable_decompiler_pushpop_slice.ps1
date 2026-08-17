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
$body = $bodyPush + $bodyPop + $bodyFloat + $bodyFormat + $bodyMarkCase + $bodyMarkGeneral + $bodyGeneralCase

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)
$prefix += "`nString __fastcall GetGvarName(DWord adr);`n"

$numericStringArgs = @(
    '_offset', '_foffset', '_pow2', '_mod', '_size', '_sz', '_ofs', '_idx', '_idx1', '_classSize', '_ap', '_adr', '_ea', '_cmpRes', '_len', '_N', '_N1',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_itemBase.IntValue', '_itemSrc.IntValue', '_itemDst.IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}
$body = $body -replace 'String\(_N1\s*-\s*_N2\)', 'std::to_string(_N1 - _N2)'
$body = $body -replace 'String\(_N1\s*-\s*1\)', 'std::to_string(_N1 - 1)'
$body = $body -replace '(?<![A-Za-z0-9_])True(?![A-Za-z0-9_])', 'true'
$body = $body -replace '(?<![A-Za-z0-9_])False(?![A-Za-z0-9_])', 'false'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$body = $body -replace 'CmpInfo\.R\s*=\s*0\s*;', 'CmpInfo.R = "0";'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.pushpop.slice.cpp" -Value ($prefix + $body) -Encoding utf8
