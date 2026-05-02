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

} // namespace ra

#endif RA_UI_TYPES_H
