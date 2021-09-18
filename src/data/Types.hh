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
    Text
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
    Value = RC_FORMAT_VALUE,
    Score = RC_FORMAT_SCORE,
    Frames = RC_FORMAT_FRAMES,
    Centiseconds = RC_FORMAT_CENTISECS,
    Seconds = RC_FORMAT_SECONDS,
    Minutes = RC_FORMAT_MINUTES,
    SecondsAsMinutes = RC_FORMAT_SECONDS_AS_MINUTES,
};

constexpr unsigned int MemSizeBits(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::ThirtyTwoBit:
        case MemSize::ThirtyTwoBitBigEndian:
            return 32;

        case MemSize::EightBit:
            return 8;

        case MemSize::SixteenBit:
        case MemSize::SixteenBitBigEndian:
            return 16;

        case MemSize::TwentyFourBit:
        case MemSize::TwentyFourBitBigEndian:
            return 24;

        case MemSize::BitCount:
            return 3;

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

constexpr unsigned int MemSizeBytes(MemSize nSize)
{
    switch (nSize)
    {
        case MemSize::ThirtyTwoBit:
        case MemSize::ThirtyTwoBitBigEndian:
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

} // namespace data
} // namespace ra

#endif RA_UI_TYPES_H
