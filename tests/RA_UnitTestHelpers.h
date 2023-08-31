#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "ra_utility.h"

#include "RA_StringUtils.h"

#include "data\Types.hh"

// The rcheevos nameless struct warning is only affecting the test project, for now we have
// to disable the warning in the project or pragmatically in rcheevos. Careful not to use nameless structs here.

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.
template<>
std::wstring ToString<std::chrono::milliseconds>(const std::chrono::milliseconds& t)
{
    return std::to_wstring(t.count()) + L"ms";
}

#ifdef RA_V141_XP
// this has to be defined when compiling against v141_xp, but not when compiling against v142.
// since it's impractical to detect the selected platform toolset, use a compilation flag

template<>
std::wstring ToString<__int64>(const __int64& t)
{
    RETURN_WIDE_STRING(t);
}
#endif

template<>
std::wstring ToString<MemSize>(const MemSize& t)
{
    switch (t)
    {
        case MemSize::Bit_0:
            return L"Bit_0";
        case MemSize::Bit_1:
            return L"Bit_1";
        case MemSize::Bit_2:
            return L"Bit_2";
        case MemSize::Bit_3:
            return L"Bit_3";
        case MemSize::Bit_4:
            return L"Bit_4";
        case MemSize::Bit_5:
            return L"Bit_5";
        case MemSize::Bit_6:
            return L"Bit_6";
        case MemSize::Bit_7:
            return L"Bit_7";
        case MemSize::Nibble_Lower:
            return L"Nibble_Lower";
        case MemSize::Nibble_Upper:
            return L"Nibble_Upper";
        case MemSize::EightBit:
            return L"EightBit";
        case MemSize::SixteenBit:
            return L"SixteenBit";
        case MemSize::TwentyFourBit:
            return L"TwentyFourBit";
        case MemSize::ThirtyTwoBit:
            return L"ThirtyTwoBit";
        case MemSize::BitCount:
            return L"BitCount";
        case MemSize::SixteenBitBigEndian:
            return L"SixteenBitBigEndian";
        case MemSize::TwentyFourBitBigEndian:
            return L"TwentyFourBitBigEndian";
        case MemSize::ThirtyTwoBitBigEndian:
            return L"ThirtyTwoBitBigEndian";
        case MemSize::Float:
            return L"Float";
        case MemSize::FloatBigEndian:
            return L"FloatBE";
        case MemSize::MBF32:
            return L"MBF32";
        case MemSize::MBF32LE:
            return L"MBF32LE";
        case MemSize::Text:
            return L"Text";
        case MemSize::Unknown:
            return L"Unknown";
        case MemSize::Array:
            return L"Array";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

template<>
std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    switch (t)
    {
        case ComparisonType::Equals:
            return L"Equals";
        case ComparisonType::LessThan:
            return L"LessThan";
        case ComparisonType::LessThanOrEqual:
            return L"LessThanOrEqual";
        case ComparisonType::GreaterThan:
            return L"GreaterThan";
        case ComparisonType::GreaterThanOrEqual:
            return L"GreaterThanOrEqual";
        case ComparisonType::NotEqualTo:
            return L"NotEqualTo";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);
void AssertDoesNotContain(const std::string& sHaystack, const std::string& sNeedle);

#endif /* !RA_UNITTESTHELPERS_H */
