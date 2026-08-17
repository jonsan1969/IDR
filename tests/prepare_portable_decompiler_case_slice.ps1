$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('DWord __fastcall TDecompiler::DecompileCaseEnum(')
$end = $src.IndexOf('String __fastcall TDecompiler::GetSysCallAlias(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler case-enum slice markers not found' }
$body = $src.Substring($start, $end - $start)

# DecompileCaseEnum uses Embarcadero String(int) for emitted numeric case
# labels. Preserve that meaning explicitly in the generated smoke copy.
$body = $body -replace [regex]::Escape('String(n + N)'), 'std::to_string(n + N)'
$body = $body -replace [regex]::Escape('String(m + N)'), 'std::to_string(m + N)'

$prefix = @'
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;

int __fastcall Adr2Pos(DWord adr);
bool __fastcall IsFlagSet(DWord flag, int pos);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.case.slice.cpp" -Value ($prefix + $body) -Encoding utf8
