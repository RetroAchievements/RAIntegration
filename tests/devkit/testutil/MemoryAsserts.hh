#ifndef RA_MEMORY_ASSERTS_H
#define RA_MEMORY_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data\Memory.hh"
#include "util\TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::Memory::Size>(const ra::data::Memory::Size& t)
{
    switch (t)
    {
        case ra::data::Memory::Size::Bit0:
            return L"Bit_0";
        case ra::data::Memory::Size::Bit1:
            return L"Bit_1";
        case ra::data::Memory::Size::Bit2:
            return L"Bit_2";
        case ra::data::Memory::Size::Bit3:
            return L"Bit_3";
        case ra::data::Memory::Size::Bit4:
            return L"Bit_4";
        case ra::data::Memory::Size::Bit5:
            return L"Bit_5";
        case ra::data::Memory::Size::Bit6:
            return L"Bit_6";
        case ra::data::Memory::Size::Bit7:
            return L"Bit_7";
        case ra::data::Memory::Size::NibbleLower:
            return L"Nibble_Lower";
        case ra::data::Memory::Size::NibbleUpper:
            return L"Nibble_Upper";
        case ra::data::Memory::Size::EightBit:
            return L"EightBit";
        case ra::data::Memory::Size::SixteenBit:
            return L"SixteenBit";
        case ra::data::Memory::Size::TwentyFourBit:
            return L"TwentyFourBit";
        case ra::data::Memory::Size::ThirtyTwoBit:
            return L"ThirtyTwoBit";
        case ra::data::Memory::Size::BitCount:
            return L"BitCount";
        case ra::data::Memory::Size::SixteenBitBigEndian:
            return L"SixteenBitBigEndian";
        case ra::data::Memory::Size::TwentyFourBitBigEndian:
            return L"TwentyFourBitBigEndian";
        case ra::data::Memory::Size::ThirtyTwoBitBigEndian:
            return L"ThirtyTwoBitBigEndian";
        case ra::data::Memory::Size::Float:
            return L"Float";
        case ra::data::Memory::Size::FloatBigEndian:
            return L"FloatBigEndian";
        case ra::data::Memory::Size::Double32:
            return L"Double32";
        case ra::data::Memory::Size::Double32BigEndian:
            return L"Double32BigEndian";
        case ra::data::Memory::Size::MBF32:
            return L"MBF32";
        case ra::data::Memory::Size::MBF32LE:
            return L"MBF32LE";
        case ra::data::Memory::Size::Text:
            return L"Text";
        case ra::data::Memory::Size::Unknown:
            return L"Unknown";
        case ra::data::Memory::Size::Array:
            return L"Array";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

template<>
std::wstring ToString<ra::data::Memory::Format>(const ra::data::Memory::Format& t)
{
    switch (t)
    {
        case ra::data::Memory::Format::Hex:
            return L"Hex";
        case ra::data::Memory::Format::Dec:
            return L"Dec";
        case ra::data::Memory::Format::Unknown:
            return L"Unknown";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_MEMORY_ASSERTS_H */
