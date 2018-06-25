#include "RA_Dlg_Login.h"

#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_PopupWindows.h"


namespace ra {

Dlg_Login::Dlg_Login() noexcept :
    IRA_Dialog{ IDD_RA_LOGIN, m_WndClass }
{
    this->m_lpCaption = this->m_WndClass.className();
    this->DoModal();
}

void Dlg_Login::OnOK(HWND hwnd) noexcept
{

    auto len{ GetTextLength(m_hUsername) };
    auto sUserEntry{ std::make_unique<TCHAR[]>(len) };
    auto bValid{ GetText(m_hUsername, len, sUserEntry.get()) > 0 };

    len = GetTextLength(m_hPassword);
    auto sPassEntry{ std::make_unique<TCHAR[]>(len) };
    bValid &= (GetText(m_hPassword, len, sPassEntry.get()) > 0);

    if (!(bValid || lstrlen(sUserEntry.get())) < (0 || lstrlen(sPassEntry.get()) < 0))
    {
        MessageBox(nullptr, TEXT("Username/password not valid! Please check and reenter"), TEXT("Error!"),
            MB_OK);
        return;
    }

    PostArgs args;
    args['u'] = Narrow(sUserEntry.release());
    args['p'] = Narrow(sPassEntry.release()); //	Plaintext password(!)

    Document doc;
    if (RAWeb::DoBlockingRequest(RequestLogin, args, doc))
    {
        std::string sResponse;
        std::string sResponseTitle;

        if (doc["Success"].GetBool())
        {
            const std::string& sUser           = doc["User"].GetString();
            const std::string& sToken          = doc["Token"].GetString();
            const unsigned int nPoints         = doc["Score"].GetUint();
            const unsigned int nUnreadMessages = doc["Messages"].GetUint();

            auto bRememberLogin{ Button_GetCheck(m_hSavePassword) };

            RAUsers::LocalUser().ProcessSuccessfulLogin(sUser, sToken, nPoints, nUnreadMessages, bRememberLogin);

            sResponse = "Logged in as " + sUser + ".";
            sResponseTitle = "Logged in Successfully!";

            //g_PopupWindows.AchievementPopups().SuppressNextDeltaUpdate();
        }
        else
        {
            sResponse = std::string("Failed!\r\n"
                "Response From Server:\r\n") +
                doc["Error"].GetString();
            sResponseTitle = "Error logging in!";
        }

        MessageBox(::GetActiveWindow(), NativeStr(sResponse).c_str(), NativeStr(sResponseTitle).c_str(), MB_OK);

        //	If we are now logged in
        //	Close this dialog
        if (RAUsers::LocalUser().IsLoggedIn())
            IRA_Dialog::Close(hwnd);
    }
    else
    {
        if (!doc.HasParseError() && doc.HasMember("Error"))
        {
            MessageBox(::GetActiveWindow(), NativeStr(std::string("Server error: ") +
                std::string(doc["Error"].GetString())).c_str(), TEXT("Error!"), MB_OK);
        }
        else
        {
            MessageBox(::GetActiveWindow(), TEXT("Unknown error connecting... please try again!"), TEXT("Error!"), MB_OK);
        }
    }
}

BOOL Dlg_Login::InitDialog(HWND hwnd, _UNUSED HWND hwndFocus, _UNUSED LPARAM lParam) noexcept
{
    m_hUsername     = ::GetDlgItem(hwnd, IDC_RA_USERNAME);
    m_hPassword     = ::GetDlgItem(hwnd, IDC_RA_PASSWORD);
    m_hSavePassword = ::GetDlgItem(hwnd, IDC_RA_SAVEPASSWORD);

    SetText(m_hUsername, NativeStr(RAUsers::LocalUser().Username()).c_str());
    if (RAUsers::LocalUser().Username().length() > 2)
    {
        SetFocus(m_hPassword, hwnd);
        return FALSE;	//	Must return FALSE if setting to a non-default active control.
    }
    else
    {
        return TRUE;
    }
}
void Dlg_Login::OnCommand(HWND hwnd, int id, _UNUSED HWND hwndCtl, _UNUSED UINT codeNotify) noexcept
{
    switch (id)
    {
        case IDOK:
            this->OnOK(hwnd);
            break;
        case IDCANCEL:
            IRA_Dialog::OnCancel(hwnd);
    }
}
void Dlg_Login::NCDestroy(HWND hwnd) noexcept
{
    IRA_Dialog::NCDestroy(m_hUsername);
    IRA_Dialog::NCDestroy(m_hPassword);
    IRA_Dialog::NCDestroy(m_hSavePassword);
    IRA_Dialog::NCDestroy(hwnd);
}

INT_PTR Dlg_Login::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, this->InitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, this->OnCommand);
        HANDLE_MSG(hwndDlg, WM_NCDESTROY, this->NCDestroy);
        HANDLE_MSG(hwndDlg, WM_CLOSE, IRA_Dialog::Close);

        default:
            return 0;
    }
}

} // namespace ra


