#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/viewmodels/WindowManager.hh"

#include "ui/win32/BrokenAchievementsDialog.hh"
#include "ui/win32/GameChecksumDialog.hh"
#include "ui/win32/LoginDialog.hh"
#include "ui/win32/MessageBoxDialog.hh"
#include "ui/win32/MemoryBookmarksDialog.hh"
#include "ui/win32/RichPresenceDialog.hh"
#include "ui/win32/UnknownGameDialog.hh"

#include "RA_Log.h"

namespace ra {
namespace ui {
namespace win32 {

Desktop::Desktop() noexcept
{
    // most common first
    m_vDialogPresenters.emplace_back(new (std::nothrow) MessageBoxDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) RichPresenceDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) GameChecksumDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) LoginDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) MemoryBookmarksDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) BrokenAchievementsDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) UnknownGameDialog::Presenter);
}

void Desktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        pPresenter->ShowWindow(vmViewModel);
    }
    else
    {
        assert("!No presenter for view model");
    }
}

ra::ui::DialogResult Desktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    const auto& vmEmulator = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().Emulator;
    return ShowModal(vmViewModel, vmEmulator);
}

ra::ui::DialogResult Desktop::ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentViewModel) const
{
    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        const auto* pBinding = ui::win32::bindings::WindowBinding::GetBindingFor(vmParentViewModel);
        const HWND hParentWnd = pBinding ? pBinding->GetHWnd() : nullptr;

        pPresenter->ShowModal(vmViewModel, hParentWnd);
    }
    else
    {
        assert("!No presenter for view model");
    }

    return vmViewModel.GetDialogResult();
}

void Desktop::GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const
{
    if (m_oWorkAreaSize.Width == 0)
    {
        RECT rcWorkArea;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0))
        {
            m_oWorkAreaPosition = { rcWorkArea.left, rcWorkArea.top };
            m_oWorkAreaSize = { rcWorkArea.right - rcWorkArea.left, rcWorkArea.bottom - rcWorkArea.top };

            RA_LOG_INFO("Desktop work area: %dx%d", m_oWorkAreaSize.Width, m_oWorkAreaSize.Height);
        }
        else
        {
            RA_LOG_ERR("Unable to determine work area");

            m_oWorkAreaPosition = { 0, 0 };
            m_oWorkAreaSize = { 800, 600 };
        }
    }

    oUpperLeftCorner = m_oWorkAreaPosition;
    oSize = m_oWorkAreaSize;
}

ra::ui::Size Desktop::GetClientSize(const WindowViewModelBase& vmViewModel) const noexcept
{
    const auto* const pBinding = ra::ui::win32::bindings::WindowBinding::GetBindingFor(vmViewModel);
    if (pBinding == nullptr)
        return {};

    RECT rcClientArea;
    ::GetClientRect(pBinding->GetHWnd(), &rcClientArea);
    return { rcClientArea.right - rcClientArea.left, rcClientArea.bottom - rcClientArea.top };
}

_Use_decl_annotations_
IDialogPresenter* Desktop::GetDialogPresenter(const WindowViewModelBase& oViewModel) const
{
    for (const auto& pPresenter : m_vDialogPresenters)
    {
        if (pPresenter->IsSupported(oViewModel))
            return pPresenter.get();
    }

    return nullptr;
}

void Desktop::SetMainHWnd(HWND hWnd)
{
    if (m_pWindowBinding == nullptr)
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        m_pWindowBinding = std::make_unique<ra::ui::win32::bindings::WindowBinding>(pWindowManager.Emulator);
    }

    m_pWindowBinding->SetHWND(hWnd);
}

std::string Desktop::GetRunningExecutable() const
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string Desktop::GetOSVersionString() const
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
    // https://github.com/DarthTon/Blackbone/blob/master/contrib/VersionHelpers.h
#ifndef NTSTATUS
    using NTSTATUS = __success(return >= 0) LONG;
#endif
    if (const auto ntModule{ ::GetModuleHandleW(L"ntdll.dll") }; ntModule)
    {
        RTL_OSVERSIONINFOEXW osVersion{ sizeof(RTL_OSVERSIONINFOEXW) };
        using fnRtlGetVersion = NTSTATUS(NTAPI*)(PRTL_OSVERSIONINFOEXW);

        fnRtlGetVersion RtlGetVersion{};
        GSL_SUPPRESS_TYPE1 RtlGetVersion =
            reinterpret_cast<fnRtlGetVersion>(::GetProcAddress(ntModule, "RtlGetVersion"));

        if (RtlGetVersion)
        {
            RtlGetVersion(&osVersion);
            if (osVersion.dwMajorVersion > 0UL)
                return ra::StringPrintf("WindowsNT %lu.%lu", osVersion.dwMajorVersion, osVersion.dwMinorVersion);
        }
    }

    return "Windows";
}


void Desktop::OpenUrl(const std::string& sUrl) const noexcept
{
    ShellExecute(nullptr, TEXT("open"), sUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Desktop::Shutdown() noexcept
{
    // must destroy binding before viewmodel
    m_pWindowBinding.reset();
}

} // namespace win32
} // namespace ui
} // namespace ra
