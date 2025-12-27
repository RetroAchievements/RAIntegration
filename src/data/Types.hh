#ifndef RA_DATA_TYPES_H
#define RA_DATA_TYPES_H
#pragma once

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

} // namespace data
} // namespace ra

#endif RA_UI_TYPES_H
