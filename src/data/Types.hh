#ifndef RA_DATA_TYPES_H
#define RA_DATA_TYPES_H
#pragma once

enum class MemSize : uint8_t
{
    Bit_0,
    Bit_1,
    Bit_2,
    Bit_3,
    Bit_4,
    Bit_5,
    Bit_6,
    Bit_7,
    Nibble_Lower,
    Nibble_Upper,
    EightBit,
    SixteenBit,
    TwentyFourBit,
    ThirtyTwoBit,
    BitCount,
    SixteenBitBigEndian,
    TwentyFourBitBigEndian,
    ThirtyTwoBitBigEndian,
    Float,
    MBF32,
    MBF32LE,
    FloatBigEndian,
    Double32,
    Double32BigEndian,

    // extended sizes not supported by rcheevos
    Unknown,
    Text,
    Array
};

enum class ComparisonType : uint8_t
{
    Equals,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    NotEqualTo
};

namespace ra {

enum class MemFormat : uint8_t
{
    Hex,
    Dec,
};

using ByteAddress = uint32_t;
using AchievementID = uint32_t;
using LeaderboardID = uint32_t;

namespace data {

enum class ValueFormat : uint8_t
{
    Frames = RC_FORMAT_FRAMES,
    Seconds = RC_FORMAT_SECONDS,
    Centiseconds = RC_FORMAT_CENTISECS,
    Score = RC_FORMAT_SCORE,
    Value = RC_FORMAT_VALUE,
    Minutes = RC_FORMAT_MINUTES,
    SecondsAsMinutes = RC_FORMAT_SECONDS_AS_MINUTES,
    Float1 = RC_FORMAT_FLOAT1,
    Float2 = RC_FORMAT_FLOAT2,
    Float3 = RC_FORMAT_FLOAT3,
    Float4 = RC_FORMAT_FLOAT4,
    Float5 = RC_FORMAT_FLOAT5,
    Float6 = RC_FORMAT_FLOAT6,
    Fixed1 = RC_FORMAT_FIXED1,
    Fixed2 = RC_FORMAT_FIXED2,
    Fixed3 = RC_FORMAT_FIXED3,
    Tens = RC_FORMAT_TENS,
    Hundreds = RC_FORMAT_HUNDREDS,
    Thousands = RC_FORMAT_THOUSANDS,
    UnsignedValue = RC_FORMAT_UNSIGNED_VALUE,
};

const char* ValueFormatToString(ValueFormat nFormat) noexcept;
ValueFormat ValueFormatFromString(const std::string& sFormat) noexcept;

constexpr bool MemSizeIsFloat(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
        case MemSize::Double32:
        case MemSize::Double32BigEndian:
            return true;

        default:
            return false;
    }
}

constexpr bool IsBigEndian(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::SixteenBitBigEndian:
        case MemSize::TwentyFourBitBigEndian:
        case MemSize::ThirtyTwoBitBigEndian:
        case MemSize::FloatBigEndian:
        case MemSize::Double32BigEndian:
        case MemSize::MBF32:
            return true;

        default:
            return false;
    }
}

constexpr uint32_t ReverseBytes(uint32_t nValue) noexcept
{
    return ((nValue & 0xFF000000) >> 24) |
           ((nValue & 0x00FF0000) >> 8) |
           ((nValue & 0x0000FF00) << 8) |
           ((nValue & 0x000000FF) << 24);
}

constexpr unsigned int MemSizeBits(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::ThirtyTwoBit:
        case MemSize::ThirtyTwoBitBigEndian:
        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
        case MemSize::Double32:
        case MemSize::Double32BigEndian:
            return 32;

        case MemSize::EightBit:
            return 8;

        case MemSize::SixteenBit:
        case MemSize::SixteenBitBigEndian:
            return 16;

        case MemSize::TwentyFourBit:
        case MemSize::TwentyFourBitBigEndian:
            return 24;

        case MemSize::BitCount: // value will be 0-8
        case MemSize::Nibble_Lower:
        case MemSize::Nibble_Upper:
            return 4;

        case MemSize::Bit_0:
        case MemSize::Bit_1:
        case MemSize::Bit_2:
        case MemSize::Bit_3:
        case MemSize::Bit_4:
        case MemSize::Bit_5:
        case MemSize::Bit_6:
        case MemSize::Bit_7:
            return 1;

        default:
            return 0;
    }
}

constexpr unsigned int MemSizeMax(MemSize nSize)
{
    const auto nBits = ra::data::MemSizeBits(nSize);
    if (nBits >= 32)
        return 0xFFFFFFFF;

    return (1 << nBits) - 1;
}

constexpr unsigned int MemSizeBytes(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::ThirtyTwoBit:
        case MemSize::ThirtyTwoBitBigEndian:
        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
            return 4;

        case MemSize::SixteenBit:
        case MemSize::SixteenBitBigEndian:
            return 2;

        case MemSize::TwentyFourBit:
        case MemSize::TwentyFourBitBigEndian:
            return 3;

        case MemSize::Text:
            return 0;

        default:
            return 1;
    }
}

std::wstring MemSizeFormat(unsigned nValue, MemSize nSize, MemFormat nFormat);

constexpr const wchar_t* MemSizeString(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::Bit_0:
            return L"Bit0";
        case MemSize::Bit_1:
            return L"Bit1";
        case MemSize::Bit_2:
            return L"Bit2";
        case MemSize::Bit_3:
            return L"Bit3";
        case MemSize::Bit_4:
            return L"Bit4";
        case MemSize::Bit_5:
            return L"Bit5";
        case MemSize::Bit_6:
            return L"Bit6";
        case MemSize::Bit_7:
            return L"Bit7";
        case MemSize::Nibble_Lower:
            return L"Lower4";
        case MemSize::Nibble_Upper:
            return L"Upper4";
        case MemSize::EightBit:
            return L"8-bit";
        case MemSize::SixteenBit:
            return L"16-bit";
        case MemSize::TwentyFourBit:
            return L"24-bit";
        case MemSize::ThirtyTwoBit:
            return L"32-bit";
        case MemSize::BitCount:
            return L"BitCount";
        case MemSize::SixteenBitBigEndian:
            return L"16-bit BE";
        case MemSize::TwentyFourBitBigEndian:
            return L"24-bit BE";
        case MemSize::ThirtyTwoBitBigEndian:
            return L"32-bit BE";
        case MemSize::Float:
            return L"Float";
        case MemSize::MBF32:
            return L"MBF32";
        case MemSize::MBF32LE:
            return L"MBF32 LE";
        case MemSize::FloatBigEndian:
            return L"Float BE";
        case MemSize::Double32:
            return L"Double32";
        case MemSize::Double32BigEndian:
            return L"Double32 BE";
        case MemSize::Text:
            return L"ASCII";
        case MemSize::Array:
            return L"Array";
        default:
        case MemSize::Unknown:
            return L"Unknown";
    }
}

unsigned FloatToU32(float fValue, MemSize nFloatType) noexcept;
float U32ToFloat(unsigned nValue, MemSize nFloatType) noexcept;

} // namespace data
} // namespace ra

#endif RA_UI_TYPES_H
