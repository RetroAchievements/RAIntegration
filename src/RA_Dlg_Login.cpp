#include "RA_Dlg_Login.h"

#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_PopupWindows.h"

#include "api\Login.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

//static 
BOOL RA_Dlg_Login::DoModalLogin()
{
    return DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_LOGIN), g_RAMainWnd, RA_Dlg_Login::RA_Dlg_LoginProc);
}

INT_PTR CALLBACK RA_Dlg_Login::RA_Dlg_LoginProc(HWND hDlg, UINT uMsg, WPARAM wParam, _UNUSED LPARAM)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SetDlgItemText(hDlg, IDC_RA_USERNAME, NativeStr(RAUsers::LocalUser().Username()).c_str());
            if (RAUsers::LocalUser().Username().length() > 2)
            {
                HWND hPass = GetDlgItem(hDlg, IDC_RA_PASSWORD);
                SetFocus(hPass);
                return FALSE;	//	Must return FALSE if setting to a non-default active control.
            }
            else
            {
                return TRUE;
            }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                {
                    TCHAR sUserEntry[64];
                    if (GetDlgItemText(hDlg, IDC_RA_USERNAME, sUserEntry, 64) == 0)
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Username is required.");
                        return TRUE;
                    }

                    TCHAR sPassEntry[64];
                    if (GetDlgItemText(hDlg, IDC_RA_PASSWORD, sPassEntry, 64) == 0)
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Password is required.");
                        return TRUE;
                    }

                    ra::api::Login::Request request;
                    request.Username = ra::Narrow(sUserEntry);
                    request.Password = ra::Narrow(sPassEntry);

                    const auto response = request.Call();

                    if (response.Succeeded())
                    {
                        bool bRememberLogin = (IsDlgButtonChecked(hDlg, IDC_RA_SAVEPASSWORD) != BST_UNCHECKED);

                        RAUsers::LocalUser().ProcessSuccessfulLogin(response.Username, response.ApiToken, response.Score, response.NumUnreadMessages, bRememberLogin);

                        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(
                            std::wstring(L"Successfully logged in as ") + ra::Widen(response.Username)
                        );

                        EndDialog(hDlg, TRUE);
                    }
                    else
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                            L"Failed to login", ra::Widen(response.ErrorMessage)
                        );
                    }

                    return TRUE;
                }
                break;

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, TRUE);
            return TRUE;
    }

    return FALSE;
}
