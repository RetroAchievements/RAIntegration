#ifndef RA_UNITTEST_DATAHELPERS_H
#define RA_UNITTEST_DATAHELPERS_H
#pragma once

#include "data\context\GameContext.hh"
#include "data\models\AssetModelBase.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::context::GameContext::Mode>(const ra::data::context::GameContext::Mode& nMode)
{
    switch (nMode)
    {
        case ra::data::context::GameContext::Mode::Normal:
            return L"Normal";
        case ra::data::context::GameContext::Mode::CompatibilityTest:
            return L"CompatibilityTest";
        default:
            return std::to_wstring(static_cast<int>(nMode));
    }
}

template<>
std::wstring ToString<Achievement::Category>(const Achievement::Category& category)
{
    switch (category)
    {
        case Achievement::Category::Local:
            return L"Local";
        case Achievement::Category::Core:
            return L"Core";
        case Achievement::Category::Unofficial:
            return L"Unofficial";
        default:
            return std::to_wstring(ra::etoi(category));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetType>(
    const ra::data::models::AssetType& nAssetType)
{
    switch (nAssetType)
    {
        case ra::data::models::AssetType::None:
            return L"None";
        case ra::data::models::AssetType::Achievement:
            return L"Achievement";
        case ra::data::models::AssetType::Leaderboard:
            return L"Leaderboard";
        default:
            return std::to_wstring(static_cast<int>(nAssetType));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetCategory>(
    const ra::data::models::AssetCategory& nAssetCategory)
{
    switch (nAssetCategory)
    {
        case ra::data::models::AssetCategory::Local:
            return L"Local";
        case ra::data::models::AssetCategory::Core:
            return L"Core";
        case ra::data::models::AssetCategory::Unofficial:
            return L"Unofficial";
        default:
            return std::to_wstring(static_cast<int>(nAssetCategory));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetState>(
    const ra::data::models::AssetState& nAssetState)
{
    switch (nAssetState)
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
        default:
            return std::to_wstring(static_cast<int>(nAssetState));
    }
}

template<>
std::wstring ToString<ra::data::models::AssetChanges>(
    const ra::data::models::AssetChanges& nAssetChanges)
{
    switch (nAssetChanges)
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
            return std::to_wstring(static_cast<int>(nAssetChanges));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_UNITTEST_DATAHELPERS_H */
