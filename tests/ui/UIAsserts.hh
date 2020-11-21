#ifndef RA_UNITTEST_UIHELPERS_H
#define RA_UNITTEST_UIHELPERS_H
#pragma once

#include "ui\ImageReference.hh"
#include "ui\WindowViewModelBase.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

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

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_UNITTEST_UIHELPERS_H */
