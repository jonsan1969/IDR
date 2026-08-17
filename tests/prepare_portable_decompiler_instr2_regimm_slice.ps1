$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$name = $src.IndexOf('TDecompiler::SimulateInstr2RegImm(')
$start = if ($name -ge 0) { $src.LastIndexOf('void __fastcall ', $name) } else { -1 }
$end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler instr2-regimm slice markers not found (name=$name start=$start end=$end)"
}
$body = $src.Substring($start, $end - $start)

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

# Keep numeric String conversions controlled to expressions actually common in
# the instruction simulator. Compiler feedback will drive any additions.
$numericStringArgs = @(
    '_offset', '_foffset', '_pow2', '_mod', '_size', '_idx', '_classSize', '_dotpos', '_len', '_ap',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_itemSrc.IntValue', '_itemDst.IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}

# Embarcadero String accepts numeric zero directly; std::string does not. In
# this slice both cases are comparison RHS values and mean the text "0".
$body = $body -replace 'CmpInfo\.R\s*=\s*0;', 'CmpInfo.R = "0";'

$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.instr2.regimm.slice.cpp" -Value ($prefix + $body) -Encoding utf8
