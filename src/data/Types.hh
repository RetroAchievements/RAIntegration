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

} // namespace data
} // namespace ra

#endif RA_UI_TYPES_H
