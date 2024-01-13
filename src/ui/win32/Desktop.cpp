#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/drawing/gdi/GDIBitmapSurface.hh"

#include "ui/viewmodels/WindowManager.hh"

#include "ui/win32/AssetListDialog.hh"
#include "ui/win32/AssetEditorDialog.hh"
#include "ui/win32/BrokenAchievementsDialog.hh"
#include "ui/win32/CodeNotesDialog.hh"
#include "ui/win32/FileDialog.hh"
#include "ui/win32/GameChecksumDialog.hh"
#include "ui/win32/LoginDialog.hh"
#include "ui/win32/MessageBoxDialog.hh"
#include "ui/win32/MemoryBookmarksDialog.hh"
#include "ui/win32/MemoryInspectorDialog.hh"
#include "ui/win32/NewAssetDialog.hh"
#include "ui/win32/OverlaySettingsDialog.hh"
#include "ui/win32/PointerFinderDialog.hh"
#include "ui/win32/ProgressDialog.hh"
#include "ui/win32/RichPresenceDialog.hh"
#include "ui/win32/UnknownGameDialog.hh"

#include "ui/win32/bindings/ControlBinding.hh"
#include "ui/win32/bindings/MemoryViewerControlBinding.hh"

#include "RA_Log.h"

#include <TlHelp32.h>

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
    m_vDialogPresenters.emplace_back(new (std::nothrow) MemoryInspectorDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) MemoryBookmarksDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) CodeNotesDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) AssetListDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) AssetEditorDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) PointerFinderDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) FileDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) OverlaySettingsDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) NewAssetDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) BrokenAchievementsDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) UnknownGameDialog::Presenter);
    m_vDialogPresenters.emplace_back(new (std::nothrow) ProgressDialog::Presenter);

    ra::ui::win32::bindings::MemoryViewerControlBinding::RegisterControlClass();
}

void Desktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    if (m_pWindowBinding)
        m_pWindowBinding->EnableInvokeOnUIThread();

    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        ra::ui::win32::bindings::WindowBinding::InvokeOnUIThread(
            [this, &vmViewModel, pPresenter]() {
                pPresenter->ShowWindow(vmViewModel);
            });
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
    if (m_pWindowBinding)
        m_pWindowBinding->EnableInvokeOnUIThread();

    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        const auto* pBinding = ui::win32::bindings::WindowBinding::GetBindingFor(vmParentViewModel);
        const HWND hParentWnd = pBinding ? pBinding->GetHWnd() : nullptr;

        ra::ui::win32::bindings::WindowBinding::InvokeOnUIThreadAndWait(
            [this, &vmViewModel, pPresenter, hParentWnd]() {
                pPresenter->ShowModal(vmViewModel, hParentWnd);
            });
    }
    else
    {
        assert("!No presenter for view model");
    }

    return vmViewModel.GetDialogResult();
}

void Desktop::CloseWindow(WindowViewModelBase& vmViewModel) const noexcept
{
    const auto* const pBinding = ra::ui::win32::bindings::WindowBinding::GetBindingFor(vmViewModel);
    if (pBinding != nullptr)
        ::SendMessage(pBinding->GetHWnd(), WM_COMMAND, IDCANCEL, 0);
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

HWND Desktop::GetMainHWnd() const noexcept
{
    if (m_pWindowBinding != nullptr)
        return m_pWindowBinding->GetHWnd();

    return nullptr;
}

void Desktop::SetMainHWnd(HWND hWnd)
{
    if (m_pWindowBinding == nullptr)
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        m_pWindowBinding = std::make_unique<ra::ui::win32::bindings::WindowBinding>(pWindowManager.Emulator);
    }

    m_pWindowBinding->SetHWND(nullptr, hWnd);
    m_pWindowBinding->SetUIThread(::GetWindowThreadProcessId(hWnd, nullptr));
    m_pWindowBinding->EnableInvokeOnUIThread();

    g_RAMainWnd = hWnd;
}

std::wstring Desktop::GetRunningExecutable() const
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, sizeof(buffer)/sizeof(buffer[0]));
    return std::wstring(buffer);
}

std::string Desktop::GetWindowsVersionString()
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

void Desktop::OpenUrl(const std::string& sUrl) const
{
    const auto sNativeUrl = NativeStr(sUrl);
    ShellExecute(nullptr, TEXT("open"), sNativeUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::unique_ptr<ra::ui::drawing::ISurface> Desktop::CaptureClientArea(const WindowViewModelBase& vmViewModel) const
{
    std::unique_ptr<ra::ui::drawing::ISurface> pSurface;

    const auto* const pBinding = ra::ui::win32::bindings::WindowBinding::GetBindingFor(vmViewModel);
    if (pBinding != nullptr)
    {
        RECT rcClientArea;
        ::GetClientRect(pBinding->GetHWnd(), &rcClientArea);

        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        pSurface = pSurfaceFactory.CreateSurface(rcClientArea.right - rcClientArea.left, rcClientArea.bottom - rcClientArea.top);

        auto* pGDISurface = dynamic_cast<ra::ui::drawing::gdi::GDIBitmapSurface*>(pSurface.get());
        if (pGDISurface == nullptr)
        {
            pSurface.reset();
        }
        else
        {
            HDC hDC = GetDC(pBinding->GetHWnd());
            pGDISurface->CopyFromWindow(hDC, 0, 0, pSurface->GetWidth(), pSurface->GetHeight(), 0, 0);
            ReleaseDC(pBinding->GetHWnd(), hDC);
        }
    }

    return pSurface;
}

static bool IsSuspiciousProcessRunning()
{
    bool bFound = false;

    const HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot)
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (tcslen_s(pe32.szExeFile) >= 15)
                {
                    std::string sFilename = ra::Narrow(pe32.szExeFile);
                    ra::StringMakeLowercase(sFilename);

                    // cannot reliably detect injection without ObRegisterCallbacks, which requires Vista,
                    // instead just look for the default process names (it's better than nothing)
                    // [Cheat Engine.exe, cheatengine-i386.exe, cheatengine-x86_64.exe]
                    if (sFilename.find("cheat") != std::string::npos &&
                        sFilename.find("engine") != std::string::npos)
                    {
                        RA_LOG_WARN("Cheat Engine detected");
                        bFound = true;
                        break;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    return bFound;
}

bool Desktop::IsDebuggerPresent() const
{
#ifdef NDEBUG // allow debugger-limited functionality when using a DEBUG build
    if (::IsDebuggerPresent())
    {
        RA_LOG_WARN("Debugger detected");
        return true;
    }

    BOOL bDebuggerPresent;
    if (::CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebuggerPresent) && bDebuggerPresent)
    {
        RA_LOG_WARN("Remote debugger detected");
        return true;
    }
#endif

    // real cheating tools don't register as debuggers (or intercept and override the call)
    // also check for known bad agents and assume that the player is using them to cheat
    return IsSuspiciousProcessRunning();
}

bool Desktop::IsOnUIThread() const noexcept
{
    return ra::ui::win32::bindings::WindowBinding::IsOnUIThread();
}

void Desktop::InvokeOnUIThread(std::function<void()> fAction) const
{
    ra::ui::win32::bindings::WindowBinding::InvokeOnUIThread(fAction);
}

void Desktop::Shutdown() noexcept
{
    // must destroy binding before viewmodel
    m_pWindowBinding.reset();
}

} // namespace win32
} // namespace ui
} // namespace ra
