#ifndef RA_ACHIEVEMENTSET_ASSERTS_H
#define RA_ACHIEVEMENTSET_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data/models/AchievementSetModel.hh"

#include "util/TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::models::AchievementSetType>(const ra::data::models::AchievementSetType& t)
{
    switch (t)
    {
        case ra::data::models::AchievementSetType::Core:
            return L"Core";
        case ra::data::models::AchievementSetType::Bonus:
            return L"Bonus";
        case ra::data::models::AchievementSetType::Specialty:
            return L"Specialty";
        case ra::data::models::AchievementSetType::Exclusive:
            return L"Exclusive";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_ACHIEVEMENTSET_ASSERTS_H */
