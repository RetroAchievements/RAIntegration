#ifndef RA_LEADERBOARD_ASSERTS_H
#define RA_LEADERBOARD_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data/models/LeaderboardModel.hh"

#include "util/TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::models::LeaderboardModel::LeaderboardParts>(const ra::data::models::LeaderboardModel::LeaderboardParts& t)
{
    switch (t)
    {
        case ra::data::models::LeaderboardModel::LeaderboardParts::None:
            return L"None";
        case ra::data::models::LeaderboardModel::LeaderboardParts::Start:
            return L"Start";
        case ra::data::models::LeaderboardModel::LeaderboardParts::Submit:
            return L"Submit";
        case ra::data::models::LeaderboardModel::LeaderboardParts::Cancel:
            return L"Cancel";
        case ra::data::models::LeaderboardModel::LeaderboardParts::Value:
            return L"Value";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_LEADERBOARD_ASSERTS_H */
