$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"

function Get-MethodBody([string]$signature, [string]$prefix) {
    $name = $src.IndexOf($signature)
    $start = if ($name -ge 0) { $src.LastIndexOf($prefix, $name) } else { -1 }
    $end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
    if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
        throw "Decompiler push/pop slice markers not found for $signature (name=$name start=$start end=$end)"
    }
    return $src.Substring($start, $end - $start)
}

$bodyPush = Get-MethodBody 'TDecompiler::SimulatePush(' 'void __fastcall '
$bodyPop = Get-MethodBody 'TDecompiler::SimulatePop(' 'void __fastcall '
$body = $bodyPush + $bodyPop

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

$numericStringArgs = @(
    '_offset', '_idx', '_item.IntValue', '_item1.IntValue', 'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.pushpop.slice.cpp" -Value ($prefix + $body) -Encoding utf8
