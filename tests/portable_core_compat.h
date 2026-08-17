#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using String = std::string;

#ifndef __fastcall
#define __fastcall
#endif

class TList {};
class TStringList {};

#include "../KnowledgeBase.h"
#include "../Infos.h"
#include "../Disasm.h"
