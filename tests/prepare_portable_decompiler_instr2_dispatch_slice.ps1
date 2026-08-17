$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$name = $src.IndexOf('TDecompiler::SimulateInstr2(')
$start = if ($name -ge 0) { $src.LastIndexOf('void __fastcall ', $name) } else { -1 }
$end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler instr2-dispatch slice markers not found (name=$name start=$start end=$end)"
}
$body = $src.Substring($start, $end - $start)

& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.instr2.dispatch.slice.cpp" -Value ($prefix + $body) -Encoding utf8
