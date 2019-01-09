#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "ra_utility.h"

#include "RA_Condition.h"
#include "RA_StringUtils.h"

#include "api\ApiCall.hh"

#include "services\Http.hh"

#include "ui\ImageReference.hh"
#include "ui\WindowViewModelBase.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

// The rcheevos nameless struct warning is only affecting the test project, for now we have
// to disable the warning in the project or pragmatically in rcheevos. Careful not to use nameless structs here.

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.
template<>
std::wstring ToString(const MemSize& t)
{
    return MEMSIZE_STR.at(ra::etoi(t));
}

template<>
std::wstring ToString(const CompVariable::Type& t)
{
    return ra::Widen(CompVariable::TYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString(const ComparisonType& t)
{
    return ra::Widen(COMPARISONTYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString(const Condition::Type& t)
{
    return (Condition::TYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString(const ra::services::Http::StatusCode& t)
{
    return ra::ToWString(t);
}

template<>
std::wstring ToString(const ra::ui::DialogResult& result)
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
            return ra::ToWString(result);
    }
}

template<>
std::wstring ToString(const ra::ui::viewmodels::MessageBoxViewModel::Icon& icon)
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
            return ra::ToWString(icon);
    }
}

template<>
std::wstring ToString(const ra::ui::viewmodels::MessageBoxViewModel::Buttons& buttons)
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
            return ra::ToWString(buttons);
    }
}

template<>
std::wstring ToString(const ra::api::ApiResult& result)
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
        default:
            return ra::ToWString(result);
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
        default: return ra::ToWString(type);
    }
}

template<>
std::wstring ToString(const std::chrono::seconds& type)
{
    return std::to_wstring(type.count());
}

template<>
std::wstring ToString(const long long& type)
{
    return std::to_wstring(type);
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

// Loads memory into the MemoryManager

// Size of the arrays used are not always the same so a span is used
void InitializeMemory(gsl::span<unsigned char> pMemory);
void InitializeMemory(std::unique_ptr<unsigned char[]> pMemory, size_t szMemorySize);
void AssertContains(const std::string& sHaystack, const std::string& sNeedle);

#endif /* !RA_UNITTESTHELPERS_H */
