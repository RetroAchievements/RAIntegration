#include "MessageBoxDialog.hh"

#include "RA_StringUtils.h"

#include "ui/viewmodels/MessageBoxViewModel.hh"

extern HWND g_RAMainWnd;

namespace ra {
namespace ui {
namespace win32 {

using ra::ui::viewmodels::MessageBoxViewModel;

using fnTaskDialog = std::add_pointer_t<HRESULT WINAPI(HWND, HINSTANCE, PCWSTR, PCWSTR, PCWSTR,
                                                       TASKDIALOG_COMMON_BUTTON_FLAGS, PCWSTR, int*)>;
static fnTaskDialog pTaskDialog = nullptr;

MessageBoxDialog::Presenter::Presenter() noexcept
{
    // TaskDialog isn't supported on WinXP, so we have to dynamically find it.
    auto hDll = LoadLibraryA("comctl32");
    if (hDll)
        GSL_SUPPRESS_TYPE1 pTaskDialog = reinterpret_cast<fnTaskDialog>(GetProcAddress(hDll, "TaskDialog"));
}

bool MessageBoxDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& oViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::MessageBoxViewModel*>(&oViewModel) != nullptr);
}

void MessageBoxDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel);
}

void MessageBoxDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& oViewModel)
{
    auto& oMessageBoxViewModel = reinterpret_cast<MessageBoxViewModel&>(oViewModel);
    int nButton = 0;

    if (pTaskDialog == nullptr || oMessageBoxViewModel.GetHeader().empty())
    {
        std::wstring sMessage;
        gsl::not_null<const std::wstring*> pMessage{gsl::make_not_null(&oMessageBoxViewModel.GetMessage())};
        if (pTaskDialog == nullptr && !oMessageBoxViewModel.GetHeader().empty())
        {
            sMessage.reserve(oMessageBoxViewModel.GetHeader().length() + pMessage->length() + 2);
            sMessage = StringPrintf(L"%s\n\n%s", oMessageBoxViewModel.GetHeader(), *pMessage);
            pMessage = gsl::make_not_null(&sMessage);
        }

        UINT uType{};
        switch (oMessageBoxViewModel.GetIcon())
        {
            default:
            case MessageBoxViewModel::Icon::None: uType = 0; break;
            case MessageBoxViewModel::Icon::Info: uType = MB_ICONINFORMATION; break;
            case MessageBoxViewModel::Icon::Warning: uType = MB_ICONWARNING; break;
            case MessageBoxViewModel::Icon::Error: uType = MB_ICONERROR; break;
        }

        switch (oMessageBoxViewModel.GetButtons())
        {
            default:
            case MessageBoxViewModel::Buttons::OK: uType |= MB_OK; break;
            case MessageBoxViewModel::Buttons::OKCancel: uType |= MB_OKCANCEL; break;
            case MessageBoxViewModel::Buttons::YesNo: uType |= MB_YESNO; break;
            case MessageBoxViewModel::Buttons::YesNoCancel: uType |= MB_YESNOCANCEL; break;
            case MessageBoxViewModel::Buttons::RetryCancel: uType |= MB_RETRYCANCEL; break;
        }

        nButton = MessageBoxW(g_RAMainWnd, pMessage->c_str(), oMessageBoxViewModel.GetWindowTitle().c_str(), uType);
    }
    else
    {
        PCWSTR pszIcon{};
        switch (oMessageBoxViewModel.GetIcon())
        {
            default:
            case MessageBoxViewModel::Icon::None: pszIcon = nullptr; break;
            case MessageBoxViewModel::Icon::Info: pszIcon = TD_INFORMATION_ICON; break;
            case MessageBoxViewModel::Icon::Warning: pszIcon = TD_WARNING_ICON; break;
            case MessageBoxViewModel::Icon::Error: pszIcon = TD_ERROR_ICON; break;
        }

        TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons{};
        switch (oMessageBoxViewModel.GetButtons())
        {
            default:
            case MessageBoxViewModel::Buttons::OK: dwCommonButtons = TDCBF_OK_BUTTON; break;
            case MessageBoxViewModel::Buttons::OKCancel: dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON; break;
            case MessageBoxViewModel::Buttons::YesNo: dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON; break;
            case MessageBoxViewModel::Buttons::YesNoCancel: dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON; break;
            case MessageBoxViewModel::Buttons::RetryCancel: dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON; break;
        }

        const HRESULT result = pTaskDialog(g_RAMainWnd, nullptr, oMessageBoxViewModel.GetWindowTitle().c_str(),
            oMessageBoxViewModel.GetHeader().c_str(), oMessageBoxViewModel.GetMessage().c_str(), dwCommonButtons, pszIcon, &nButton);

        if (!SUCCEEDED(result))
            nButton = IDABORT;
    }

    switch (nButton)
    {
        case IDOK: oMessageBoxViewModel.SetDialogResult(DialogResult::OK); break;
        case IDCANCEL: oMessageBoxViewModel.SetDialogResult(DialogResult::Cancel); break;
        case IDYES: oMessageBoxViewModel.SetDialogResult(DialogResult::Yes); break;
        case IDNO: oMessageBoxViewModel.SetDialogResult(DialogResult::No); break;
        case IDRETRY: oMessageBoxViewModel.SetDialogResult(DialogResult::Retry); break;
        default: oMessageBoxViewModel.SetDialogResult(DialogResult::None); break;
    }
}

} // namespace win32
} // namespace ui
} // namespace ra
