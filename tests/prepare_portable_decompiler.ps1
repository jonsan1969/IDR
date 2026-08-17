$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.h"
$src = $src -replace '#include "Main.h"', '#include "../portable_core_compat.h"'
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.portable.h" -Value $src -Encoding utf8
