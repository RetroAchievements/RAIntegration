#ifndef RA_ACHIEVEMENT_ASSERTS_H
#define RA_ACHIEVEMENT_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data/models/AchievementModel.hh"

#include "util/TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::models::AchievementType>(const ra::data::models::AchievementType& t)
{
    switch (t)
    {
        case ra::data::models::AchievementType::None:
            return L"None";
        case ra::data::models::AchievementType::Missable:
            return L"Missable";
        case ra::data::models::AchievementType::Progression:
            return L"Progression";
        case ra::data::models::AchievementType::Win:
            return L"Win";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_ACHIEVEMENT_ASSERTS_H */
