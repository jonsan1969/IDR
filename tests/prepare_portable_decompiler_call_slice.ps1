$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$startName = $src.IndexOf('TDecompiler::SimulateInherited(')
$callName = $src.IndexOf('TDecompiler::SimulateCall(', [Math]::Max($startName, 0))
$start = if ($startName -ge 0) { $src.LastIndexOf('void __fastcall ', $startName) } else { -1 }
$end = if ($callName -ge 0) { $src.IndexOf("`n//---------------------------------------------------------------------------", $callName) } else { -1 }
if ($start -lt 0 -or $callName -lt 0 -or $end -lt 0 -or $end -le $start) {
    throw "Decompiler call slice markers not found (startName=$startName start=$start callName=$callName end=$end)"
}
$body = $src.Substring($start, $end - $start)

# This slice contains SimulateInherited() plus complete SimulateCall().
# Use the normal IDR function separator after SimulateCall so the boundary
# does not depend on the name/order of the following implementation.
& "$PSScriptRoot\prepare_portable_decompiler_engine_slice.ps1"
$engine = Get-Content -Raw -LiteralPath "$PSScriptRoot\generated\Decompiler.engine.slice.cpp"
$prefixEnd = $engine.IndexOf('DWord __fastcall TDecompiler::Decompile(')
if ($prefixEnd -lt 0) { throw 'Portable engine prefix marker not found' }
$prefix = $engine.Substring(0, $prefixEnd)

# Preserve only numeric String constructions already seen in decompiler code.
$numericStringArgs = @(
    '_argsNum', '_retBytes', '_retBytesCalc', '_len', '_val', '_esp', '_idx', '_rn', '_ndx', '_size', '_recsize',
    '_item.IntValue', '_item1.IntValue', '_item2.IntValue', '_item3.IntValue', '_item4.IntValue',
    '_classAdr', '_adr', '_dynAdr', 'callAdr', 'curAdr'
)
foreach ($arg in $numericStringArgs) {
    $escaped = [regex]::Escape($arg)
    $body = $body -replace "(?<![A-Za-z0-9_])String\($escaped\)", "std::to_string($arg)"
}

# Reuse the Embarcadero String semantics already proven by earlier slices.
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.Pos\(([^\r\n\)]+)\)', 'PortableStringPos($1, $2)'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SubString\(([^,\r\n]+),\s*([^\)\r\n]+)\)', 'PortableSubString($1, $2, $3)'
$body = $body -replace '\.Length\(\)', '.size()'
$body = $body -replace '([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->)[A-Za-z_][A-Za-z0-9_]*)*)\.SetLength\(([^\)\r\n]+)\)', '$1.resize($2)'

# SimulateCall repeats the same embedded-procedure UI policy and form-owned
# virtual-method lookup already reached by the main Decompile() engine. Keep
# the generated smoke copy headless and reuse the same neutral boundaries.
$body = $body -replace 'int _savedIdx = FMain_11011981->lbCode->ItemIndex;', 'int _savedIdx = -1;'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = -1;', '// portable smoke: no GUI selection state'
$body = $body -replace 'FMain_11011981->lbCode->ItemIndex = _savedIdx;', '// portable smoke: no GUI selection state'
$body = $body -replace '(?s)if \(Application->MessageBox\(\s*String\("Decompile embedded procedure at address " \+ _embAdr \+ "\?"\)\.c_str\(\), L"Confirmation",\s*MB_YESNO\) == IDYES\)', 'if (PortableConfirmEmbeddedProcedure(_embAdr))'
$body = $body -replace 'FMain_11011981->GetMethodInfo\(', 'PortableGetMethodInfo('

New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.call.slice.cpp" -Value ($prefix + $body) -Encoding utf8
