$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"

$matches = [regex]::Matches(
    $src,
    '(?m)^[^\r\n]*TDecompiler::(?<name>~?[A-Za-z_][A-Za-z0-9_]*)\s*\('
)
$sourceMethods = @(
    $matches |
        ForEach-Object { $_.Groups['name'].Value } |
        Sort-Object -Unique
)

$compiledSlices = @(
    'Decompiler.slice.cpp',
    'Decompiler.branch.slice.cpp',
    'Decompiler.engine.slice.cpp',
    'Decompiler.case.slice.cpp',
    'Decompiler.syscall.slice.cpp',
    'Decompiler.call.slice.cpp',
    'Decompiler.cmp.slice.cpp',
    'Decompiler.instr1.slice.cpp',
    'Decompiler.instr2.regimm.slice.cpp',
    'Decompiler.instr2.regreg.slice.cpp',
    'Decompiler.instr2.regmem.slice.cpp',
    'Decompiler.instr2.memimm.slice.cpp',
    'Decompiler.instr2.memreg.slice.cpp',
    'Decompiler.instr2.dispatch.slice.cpp',
    'Decompiler.pushpop.slice.cpp'
)

$compiledText = ''
foreach ($file in $compiledSlices) {
    $path = Join-Path "$PSScriptRoot\generated" $file
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Compiled Decompiler smoke slice is missing: $file"
    }
    $compiledText += "`n" + (Get-Content -Raw -LiteralPath $path)
}

$missing = @()
foreach ($name in $sourceMethods) {
    if (-not $compiledText.Contains("TDecompiler::$name(")) {
        $missing += $name
    }
}

Write-Host "TDecompiler source methods found: $($sourceMethods.Count)"
Write-Host "TDecompiler methods represented in compiled smoke slices: $($sourceMethods.Count - $missing.Count)"

if ($missing.Count -gt 0) {
    Write-Host 'Missing from compiled smoke coverage:'
    $missing | ForEach-Object { Write-Host "  - $_" }
    throw "Decompiler smoke coverage incomplete: $($missing.Count) method(s) missing"
}

Write-Host 'Decompiler smoke coverage audit: COMPLETE'
