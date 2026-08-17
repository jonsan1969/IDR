$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$startName = $src.IndexOf('TDecompiler::GetSysCallAlias(')
$endName = $src.IndexOf('TDecompiler::SimulateCall(', [Math]::Max($startName, 0))
$start = if ($startName -ge 0) { $src.LastIndexOf('String __fastcall ', $startName) } else { -1 }
$end = if ($endName -ge 0) { $src.LastIndexOf('bool __fastcall ', $endName) } else { -1 }
if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler syscall slice markers not found (startName=$startName start=$start endName=$endName end=$end)"
}
$body = $src.Substring($start, $end - $start)

# Reuse the already-proven engine prefix so dependency declarations stay in
# sync instead of being copied into every new implementation slice.
& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

# Preserve observed Embarcadero numeric String construction narrowly.
$numericStringArgs = @(
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_item4.IntValue',
    '_cnt', '_esp', '_cmpRes', '_size'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}

# Map String methods already proven by earlier slices.
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.syscall.slice.cpp" -Value ($prefix + $body) -Encoding utf8
