#include <Windows.h>
#include <cstdint>

using Byte = std::uint8_t;
using DWord = std::uint32_t;

#include "../Disasm.h"

// First portability probe: compile the existing IDR disassembler interface with MSVC.
int main() {
    DISINFO info{};
    (void)info;
    return 0;
}
