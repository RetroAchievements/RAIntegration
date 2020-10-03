#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "ra_utility.h"

#include "RA_Condition.h"
#include "RA_StringUtils.h"

#include "api\ApiCall.hh"

#include "data\GameContext.hh"

#include "services\Http.hh"

#include "ui\ImageReference.hh"
#include "ui\WindowViewModelBase.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\AssetViewModelBase.hh"

// The rcheevos nameless struct warning is only affecting the test project, for now we have
// to disable the warning in the project or pragmatically in rcheevos. Careful not to use nameless structs here.

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.
template<>
std::wstring ToString<std::chrono::milliseconds>(const std::chrono::milliseconds& t)
{
    return std::to_wstring(t.count()) + L"ms";
}

#ifndef RA_ANALYSIS
// this has to be defined when compiling against v141_xp, but not when compiling against v142.
// since it's impractical to detect the selected platform toolset, just key off the fact that
// the Analysis builds have to use the newer toolset.

template<>
std::wstring ToString<__int64>(const __int64& t)
{
    RETURN_WIDE_STRING(t);
}
#endif

template<>
std::wstring ToString<MemSize>(const MemSize& t)
{
    switch (t)
    {
        case MemSize::Text:
            return L"Text";
        default:
            if (ra::etoi(t) < MEMSIZE_STR.size())
                return MEMSIZE_STR.at(ra::etoi(t));
            return std::to_wstring(ra::etoi(t));
    }

}

template<>
std::wstring ToString<CompVariable::Type>(const CompVariable::Type& t)
{
    return ra::Widen(CompVariable::TYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    return ra::Widen(COMPARISONTYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString<Condition::Type>(const Condition::Type& t)
{
    return (Condition::TYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString<ra::services::Http::StatusCode>(const ra::services::Http::StatusCode& t)
{
    return std::to_wstring(ra::etoi(t));
}

template<>
std::wstring ToString<ra::ui::DialogResult>(const ra::ui::DialogResult& result)
{
    switch (result)
    {
        case ra::ui::DialogResult::None:
            return L"None";
        case ra::ui::DialogResult::OK:
            return L"OK";
        case ra::ui::DialogResult::Cancel:
            return L"Cancel";
        case ra::ui::DialogResult::Yes:
            return L"Yes";
        case ra::ui::DialogResult::No:
            return L"No";
        case ra::ui::DialogResult::Retry:
            return L"Retry";
        default:
            return std::to_wstring(ra::etoi(result));
    }
}

template<>
std::wstring
    ToString<ra::ui::viewmodels::MessageBoxViewModel::Icon>(const ra::ui::viewmodels::MessageBoxViewModel::Icon& icon)
{
    switch (icon)
    {
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::None:
            return L"None";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Info:
            return L"Info";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning:
            return L"Warning";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Error:
            return L"Error";
        default:
            return std::to_wstring(static_cast<int>(icon));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::MessageBoxViewModel::Buttons>(
    const ra::ui::viewmodels::MessageBoxViewModel::Buttons& buttons)
{
    switch (buttons)
    {
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::OK:
            return L"OK";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::OKCancel:
            return L"OKCancel";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo:
            return L"YesNo";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNoCancel:
            return L"YesNoCancel";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel:
            return L"RetryCancel";
        default:
            return std::to_wstring(static_cast<int>(buttons));
    }
}

template<>
std::wstring ToString<ra::api::ApiResult>(const ra::api::ApiResult& result)
{
    switch (result)
    {
        case ra::api::ApiResult::None:
            return L"None";
        case ra::api::ApiResult::Success:
            return L"Success";
        case ra::api::ApiResult::Error:
            return L"Error";
        case ra::api::ApiResult::Failed:
            return L"Failed";
        case ra::api::ApiResult::Unsupported:
            return L"Unsupported";
        case ra::api::ApiResult::Incomplete:
            return L"Incomplete";
        default:
            return std::to_wstring(ra::etoi(result));
    }
}

template<> std::wstring ToString<ra::ui::ImageType>(const ra::ui::ImageType& type)
{
    switch (type)
    {
        case ra::ui::ImageType::None: return L"None";
        case ra::ui::ImageType::Badge: return L"Badge";
        case ra::ui::ImageType::UserPic: return L"UserPic";
        case ra::ui::ImageType::Local: return L"Local";
        case ra::ui::ImageType::Icon: return L"Icon";
        default: return std::to_wstring(ra::etoi(type));
    }
}

template<>
std::wstring ToString<ra::data::GameContext::Mode>(const ra::data::GameContext::Mode& nMode)
{
    switch (nMode)
    {
        case ra::data::GameContext::Mode::Normal:
            return L"Normal";
        case ra::data::GameContext::Mode::CompatibilityTest:
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
std::wstring ToString<ra::ui::viewmodels::AssetCategory>(
    const ra::ui::viewmodels::AssetCategory& nAssetCategory)
{
    switch (nAssetCategory)
    {
        case ra::ui::viewmodels::AssetCategory::Local:
            return L"Local";
        case ra::ui::viewmodels::AssetCategory::Core:
            return L"Core";
        case ra::ui::viewmodels::AssetCategory::Unofficial:
            return L"Unofficial";
        default:
            return std::to_wstring(static_cast<int>(nAssetCategory));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetState>(
    const ra::ui::viewmodels::AssetState& nAssetState)
{
    switch (nAssetState)
    {
        case ra::ui::viewmodels::AssetState::Inactive:
            return L"Inactive";
        case ra::ui::viewmodels::AssetState::Active:
            return L"Active";
        case ra::ui::viewmodels::AssetState::Triggered:
            return L"Triggered";
        case ra::ui::viewmodels::AssetState::Waiting:
            return L"Waiting";
        case ra::ui::viewmodels::AssetState::Paused:
            return L"Paused";
        default:
            return std::to_wstring(static_cast<int>(nAssetState));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetChanges>(
    const ra::ui::viewmodels::AssetChanges& nAssetChanges)
{
    switch (nAssetChanges)
    {
        case ra::ui::viewmodels::AssetChanges::None:
            return L"None";
        case ra::ui::viewmodels::AssetChanges::Modified:
            return L"Modified";
        case ra::ui::viewmodels::AssetChanges::Unpublished:
            return L"Unpublished";
        default:
            return std::to_wstring(static_cast<int>(nAssetChanges));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);

#endif /* !RA_UNITTESTHELPERS_H */
