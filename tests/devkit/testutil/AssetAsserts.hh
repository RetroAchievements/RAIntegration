#ifndef RA_ASSET_ASSERTS_H
#define RA_ASSET_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data\models\AssetModelBase.hh"
#include "util\TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::models::AssetType>(const ra::data::models::AssetType& t)
{
    switch (t)
    {
        case ra::data::models::AssetType::None:
            return L"None";
        case ra::data::models::AssetType::Achievement:
            return L"Achievement";
        case ra::data::models::AssetType::Leaderboard:
            return L"Leaderboard";
        case ra::data::models::AssetType::RichPresence:
            return L"RichPresence";
        case ra::data::models::AssetType::LocalBadges:
            return L"LocalBadges";
        case ra::data::models::AssetType::CodeNotes:
            return L"CodeNotes";
        case ra::data::models::AssetType::MemoryRegions:
            return L"MemoryRegions";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetCategory>(const ra::data::models::AssetCategory& t)
{
    switch (t)
    {
        case ra::data::models::AssetCategory::None:
            return L"None";
        case ra::data::models::AssetCategory::Local:
            return L"Local";
        case ra::data::models::AssetCategory::Core:
            return L"Core";
        case ra::data::models::AssetCategory::Unofficial:
            return L"Unofficial";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetState>(const ra::data::models::AssetState& t)
{
    switch (t)
    {
        case ra::data::models::AssetState::Inactive:
            return L"Inactive";
        case ra::data::models::AssetState::Active:
            return L"Active";
        case ra::data::models::AssetState::Triggered:
            return L"Triggered";
        case ra::data::models::AssetState::Waiting:
            return L"Waiting";
        case ra::data::models::AssetState::Paused:
            return L"Paused";
        case ra::data::models::AssetState::Disabled:
            return L"Disabled";
        case ra::data::models::AssetState::Primed:
            return L"Primed";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetChanges>(const ra::data::models::AssetChanges& t)
{
    switch (t)
    {
        case ra::data::models::AssetChanges::None:
            return L"None";
        case ra::data::models::AssetChanges::Modified:
            return L"Modified";
        case ra::data::models::AssetChanges::Unpublished:
            return L"Unpublished";
        case ra::data::models::AssetChanges::New:
            return L"New";
        case ra::data::models::AssetChanges::Deleted:
            return L"Deleted";
        default:
            return std::to_wstring(static_cast<int>(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_MEMORY_ASSERTS_H */
