#pragma once

// Transitional MSVC x86 bridge for compiling the real legacy Decompiler TU.
// This is deliberately narrow and temporary: core-facing code should use the
// neutral Idr* types instead. Keep __fastcall intact so MSVC x86 ABI matches
// the original declarations.

#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;
using AnsiString = std::string;
using Variant = std::int64_t;
using Comp = std::int64_t;
using Char = char;
using TColor = unsigned long;
using TObject = void;

struct Currency {
    std::int64_t Val = 0;
};

class WideString : public std::wstring {
public:
    using std::wstring::wstring;
    WideString() = default;
    WideString(const std::wstring &value) : std::wstring(value) {}
    const wchar_t *c_bstr() const { return c_str(); }
};

class Exception : public std::runtime_error {
public:
    String Message;
    explicit Exception(const char *message) : std::runtime_error(message), Message(message) {}
    explicit Exception(const String &message) : std::runtime_error(message), Message(message) {}
};

class TCriticalSection {
public:
    void Enter() { mutex_.lock(); }
    void Leave() { mutex_.unlock(); }
private:
    std::recursive_mutex mutex_;
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
    void Insert(int index, void *item) {
        Items.insert(Items.begin() + index, item);
        Count = static_cast<int>(Items.size());
    }
    void Clear() { Items.clear(); Count = 0; }
    void Delete(int index) {
        Items.erase(Items.begin() + index);
        Count = static_cast<int>(Items.size());
    }
    template <typename Compare>
    void Sort(Compare compare) {
        std::sort(Items.begin(), Items.end(), [compare](void *left, void *right) {
            return compare(left, right) < 0;
        });
        Count = static_cast<int>(Items.size());
    }
};

class TStringList {
public:
    bool Sorted = false;
    int Count = 0;
    std::vector<String> Strings;
    std::vector<void *> Objects;

    int Add(const String &value) {
        int index = 0;
        if (Sorted) {
            auto it = std::lower_bound(Strings.begin(), Strings.end(), value);
            index = static_cast<int>(it - Strings.begin());
            Strings.insert(it, value);
            Objects.insert(Objects.begin() + index, nullptr);
        } else {
            Strings.push_back(value);
            Objects.push_back(nullptr);
            index = static_cast<int>(Strings.size()) - 1;
        }
        Count = static_cast<int>(Strings.size());
        return index;
    }
    int AddObject(const String &value, TObject *object) {
        const int index = Add(value);
        Objects[static_cast<std::size_t>(index)] = object;
        return index;
    }
    int IndexOf(const String &value) const {
        const auto it = std::find(Strings.begin(), Strings.end(), value);
        return it == Strings.end() ? -1 : static_cast<int>(it - Strings.begin());
    }
    int IndexOfName(const String &name) const {
        for (int i = 0; i < Count; ++i) {
            const auto sep = Strings[i].find('=');
            const String lhs = sep == String::npos ? Strings[i] : Strings[i].substr(0, sep);
            if (lhs == name) return i;
        }
        return -1;
    }
};

// Parse-only GUI placeholders. No GUI behavior is implemented here.
class TForm {};
class TStrings {};
class TCanvas {};
struct TRect { int Left = 0; int Top = 0; int Right = 0; int Bottom = 0; };

// Core analysis flags/kinds historically supplied by Main.h.
#define cfCode          0x00000001
#define cfData          0x00000002
#define cfImport        0x00000004
#define cfCall          0x00000008
#define cfProcStart     0x00000010
#define cfProcEnd       0x00000020
#define cfRTTI          0x00000040
#define cfEmbedded      0x00000080
#define cfPass0         0x00000100
#define cfFrame         0x00000200
#define cfSwitch        0x00000400
#define cfPass1         0x00000800
#define cfETable        0x00001000
#define cfPush          0x00002000
#define cfDSkip         0x00004000
#define cfPop           0x00008000
#define cfSetA          0x00010000
#define cfSetD          0x00020000
#define cfSetC          0x00040000
#define cfBracket       0x00080000
#define cfPass2         0x00100000
#define cfExport        0x00200000
#define cfPass          0x00400000
#define cfLoc           0x00800000
#define cfTry           0x01000000
#define cfFinally       0x02000000
#define cfExcept        0x04000000
#define cfLoop          0x08000000
#define cfFinallyExit   0x10000000
#define cfVTable        0x20000000
#define cfSkip          0x40000000
#define cfInstruction   0x80000000

#define ikUnknown       0x00
#define ikInteger       0x01
#define ikChar          0x02
#define ikEnumeration   0x03
#define ikFloat         0x04
#define ikString        0x05
#define ikSet           0x06
#define ikClass         0x07
#define ikMethod        0x08
#define ikWChar         0x09
#define ikLString       0x0A
#define ikWString       0x0B
#define ikVariant       0x0C
#define ikArray         0x0D
#define ikRecord        0x0E
#define ikInterface     0x0F
#define ikInt64         0x10
#define ikDynArray      0x11
#define ikUString       0x12
#define ikClassRef      0x13
#define ikPointer       0x14
#define ikProcedure     0x15
#define ikCString       0x20
#define ikWCString      0x21
#define ikResString     0x22
#define ikVMT           0x23
#define ikGUID          0x24
#define ikRefine        0x25
#define ikConstructor   0x26
#define ikDestructor    0x27
#define ikProc          0x28
#define ikFunc          0x29
#define ikLoc           0x2A
#define ikData          0x2B

#include "../../KnowledgeBase.h"
#include "../../Infos.h"
#include "../../Disasm.h"
