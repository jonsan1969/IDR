$src = Get-Content -Raw -LiteralPath "$PSScriptRoot\..\Decompiler.cpp"
$start = $src.IndexOf('bool __fastcall TDecompileEnv::GetBJLRange(')
$end = $src.IndexOf('String __fastcall TDecompileEnv::PrintBJL()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Decompiler branch slice markers not found' }
$body = $src.Substring($start, $end - $start)

# Embarcadero String(int) formats an integer as text. std::string has no
# equivalent one-argument integer constructor, so preserve the intended
# semantics explicitly in the generated portability smoke copy.
$body = $body -replace 'String\(m \+ 1\)', 'std::to_string(m + 1)'
$body = $body -replace 'String\(m\)', 'std::to_string(m)'

# Embarcadero String::Length() maps to std::string::size() for the ANSI text
# semantics exercised by this compile-only BJL slice.
$body = $body -replace '\.Length\(\)', '.size()'

$prefix = @'
#include <cassert>
#include "portable_core_compat.h"
#include "generated/Decompiler.portable.h"

extern MDisasm Disasm;
extern Byte *Code;
int __fastcall Adr2Pos(DWord adr);
bool __fastcall IsFlagSet(DWord flag, int pos);
int __fastcall BranchGetPrevInstructionType(DWord fromAdr, DWord *jmpAdr, PLoopInfo loopInfo);
String __fastcall GetDirectCondition(char c);
String __fastcall GetInvertCondition(char c);

'@
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\generated" | Out-Null
Set-Content -LiteralPath "$PSScriptRoot\generated\Decompiler.branch.slice.cpp" -Value ($prefix + $body) -Encoding utf8
