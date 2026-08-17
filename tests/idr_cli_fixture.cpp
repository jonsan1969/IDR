#pragma comment(linker, "/entry:FixtureEntry")
#pragma comment(linker, "/nodefaultlib")

extern "C" __declspec(naked) int __cdecl FixtureLeaf() {
    __asm {
        mov eax, 7
        ret
    }
}

extern "C" __declspec(naked) int __cdecl FixtureTarget() {
    __asm {
        call FixtureLeaf
        call FixtureLeaf
        ret
    }
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
