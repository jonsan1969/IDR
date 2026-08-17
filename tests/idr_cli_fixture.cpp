#pragma comment(linker, "/entry:_FixtureEntry")
#pragma comment(linker, "/nodefaultlib")

extern "C" __declspec(noinline) int __cdecl FixtureTarget() {
    return 7;
}

extern "C" __declspec(naked) void __cdecl FixtureEntry() {
    __asm {
        call FixtureTarget
        test eax, eax
        jz zero_result
        xor eax, eax
    zero_result:
        ret
    }
}
