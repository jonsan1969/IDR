$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$name = $src.IndexOf('TDecompiler::SimulateInstr2(')
$start = if ($name -ge 0) { $src.LastIndexOf('void __fastcall ', $name) } else { -1 }
$end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler instr2-dispatch slice markers not found (name=$name start=$start end=$end)"
}
$body = $src.Substring($start, $end - $start)

$name3 = $src.IndexOf('TDecompiler::SimulateInstr3(')
$start3 = if ($name3 -ge 0) { $src.LastIndexOf('void __fastcall ', $name3) } else { -1 }
$end3 = if ($name3 -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name3) } else { -1 }
if ($start3 -lt 0 -or $end3 -lt 0 -or $end3 -le $start3) {
    throw "Decompiler instr3 slice markers not found (name=$name3 start=$start3 end=$end3)"
}
$body3 = $src.Substring($start3, $end3 - $start3)

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

$numericStringArgs = @(
    '_offset', '_foffset', '_pow2', '_mod', '_size', '_idx', '_idx1', '_classSize', '_ap', '_adr', '_imm',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_itemSrc.IntValue', '_itemDst.IntValue',
    'DisInfo.Immediate', 'DisInfo.Offset', 'DisInfo.Scale'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body3 = $body3 -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}
$body3 = $body3 -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body3 = $body3 -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body3 = $body3 -replace '\.Length\(\)', '.size()'
$body3 = $body3 -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'
$body3 = $body3 -replace 'CmpInfo\.R\s*=\s*0\s*;', 'CmpInfo.R = "0";'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.instr2.dispatch.slice.cpp" -Value ($prefix + $body + $body3) -Encoding utf8
