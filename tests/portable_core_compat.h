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
using AnsiString = std::string;

class WideString : public std::wstring {
public:
    using std::wstring::wstring;
    WideString() = default;
    WideString(const std::wstring &value) : std::wstring(value) {}
    const wchar_t *c_bstr() const { return c_str(); }
};

#ifndef __fastcall
#define __fastcall
#endif

// Core analysis flags and kinds normally declared in Main.h. Keep only the
// definitions needed by the portable Decompiler slices here so the test
// harness does not have to pull in the VCL-heavy Main.h.
#ifndef cfImport
#define cfImport 0x00000004
#endif
#ifndef cfProcStart
#define cfProcStart 0x00000010
#endif
#ifndef cfFrame
#define cfFrame 0x00000200
#endif
#ifndef cfSwitch
#define cfSwitch 0x00000400
#endif
#ifndef cfDSkip
#define cfDSkip 0x00004000
#endif
#ifndef cfPass
#define cfPass 0x00400000
#endif
#ifndef cfLoc
#define cfLoc 0x00800000
#endif
#ifndef cfTry
#define cfTry 0x01000000
#endif
#ifndef cfLoop
#define cfLoop 0x08000000
#endif
#ifndef cfFinallyExit
#define cfFinallyExit 0x10000000
#endif
#ifndef cfSkip
#define cfSkip 0x40000000
#endif
#ifndef ikUnknown
#define ikUnknown 0x00
#endif
#ifndef ikInteger
#define ikInteger 0x01
#endif
#ifndef ikChar
#define ikChar 0x02
#endif
#ifndef ikEnumeration
#define ikEnumeration 0x03
#endif
#ifndef ikFloat
#define ikFloat 0x04
#endif
#ifndef ikString
#define ikString 0x05
#endif
#ifndef ikSet
#define ikSet 0x06
#endif
#ifndef ikClass
#define ikClass 0x07
#endif
#ifndef ikLString
#define ikLString 0x0A
#endif
#ifndef ikWString
#define ikWString 0x0B
#endif
#ifndef ikVariant
#define ikVariant 0x0C
#endif
#ifndef ikArray
#define ikArray 0x0D
#endif
#ifndef ikRecord
#define ikRecord 0x0E
#endif
#ifndef ikInt64
#define ikInt64 0x10
#endif
#ifndef ikDynArray
#define ikDynArray 0x11
#endif
#ifndef ikUString
#define ikUString 0x12
#endif
#ifndef ikClassRef
#define ikClassRef 0x13
#endif
#ifndef ikPointer
#define ikPointer 0x14
#endif
#ifndef ikCString
#define ikCString 0x20
#endif
#ifndef ikWCString
#define ikWCString 0x21
#endif
#ifndef ikResString
#define ikResString 0x22
#endif
#ifndef ikVMT
#define ikVMT 0x23
#endif
#ifndef ikConstructor
#define ikConstructor 0x26
#endif
#ifndef ikDestructor
#define ikDestructor 0x27
#endif
#ifndef ikProc
#define ikProc 0x28
#endif
#ifndef ikFunc
#define ikFunc 0x29
#endif
#ifndef ikData
#define ikData 0x2B
#endif

class Exception : public std::runtime_error {
public:
    String Message;

    explicit Exception(const char *message)
        : std::runtime_error(message), Message(message) {}

    explicit Exception(const String &message)
        : std::runtime_error(message), Message(message) {}
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

    void Clear() {
        Items.clear();
        Count = 0;
    }

    void Delete(int index) {
        Items.erase(Items.begin() + index);
        Count = static_cast<int>(Items.size());
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
