$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$name = $src.IndexOf('TDecompiler::GetCmpInfo(')
$start = if ($name -ge 0) { $src.LastIndexOf('int __fastcall ', $name) } else { -1 }
$end = if ($name -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $name) } else { -1 }
if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler cmp slice markers not found (name=$name start=$start end=$end)"
}
$body = $src.Substring($start, $end - $start)

# Reuse the compiler-neutral declaration surface already proven by the main
# engine. Keep GetCmpInfo isolated so the #62 call-simulation milestone stays
# independently green while comparison reconstruction is mapped.
& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

# Reuse only already-established Embarcadero String transforms.
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.cmp.slice.cpp" -Value ($prefix + $body) -Encoding utf8
