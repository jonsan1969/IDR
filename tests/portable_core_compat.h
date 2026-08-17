#pragma once

#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;

#ifndef __fastcall
#define __fastcall
#endif

// Core analysis flags and kinds normally declared in Main.h. Keep only the
// definitions needed by the portable Decompiler slices here so the test
// harness does not have to pull in the VCL-heavy Main.h.
#ifndef cfImport
#define cfImport 0x00000004
#endif
#ifndef cfPass
#define cfPass 0x00400000
#endif
#ifndef cfLoc
#define cfLoc 0x00800000
#endif
#ifndef cfSkip
#define cfSkip 0x40000000
#endif
#ifndef ikFloat
#define ikFloat 0x04
#endif
#ifndef ikLString
#define ikLString 0x0A
#endif
#ifndef ikRecord
#define ikRecord 0x0E
#endif
#ifndef ikFunc
#define ikFunc 0x29
#endif

class Exception : public std::runtime_error {
public:
    explicit Exception(const char *message) : std::runtime_error(message) {}
    explicit Exception(const String &message) : std::runtime_error(message) {}
};

class TList {
public:
    int Count = 0;
    std::vector<void *> Items;

    int Add(void *item) {
        Items.push_back(item);
        Count = static_cast<int>(Items.size());
        return Count - 1;
    }
};

class TStringList {
public:
    bool Sorted = false;
    int Count = 0;
    std::vector<String> Strings;

    int Add(const String &value) {
        Strings.push_back(value);
        if (Sorted) std::sort(Strings.begin(), Strings.end());
        Count = static_cast<int>(Strings.size());
        return IndexOf(value);
    }

    int IndexOf(const String &value) const {
        auto it = std::find(Strings.begin(), Strings.end(), value);
        return it == Strings.end() ? -1 : static_cast<int>(it - Strings.begin());
    }
};

#include "../KnowledgeBase.h"
#include "../Infos.h"
#include "../Disasm.h"
