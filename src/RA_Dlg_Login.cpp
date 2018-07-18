#include "RA_Dlg_Login.h"

#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_PopupWindows.h"

#include "ra_wm_handler.h"

namespace ra {
namespace ui {

// This class probably doesn't need to be used anywhere else
struct LoginHelper : public WM_Handler
{
    inline constexpr LoginHelper() noexcept = default;
    ~LoginHelper() noexcept = default;
    LoginHelper(const LoginHelper&) = delete;
    LoginHelper& operator=(const LoginHelper&) = delete;
    inline constexpr LoginHelper(LoginHelper&&) noexcept = default;
    inline constexpr LoginHelper& operator=(LoginHelper&&) noexcept = default;

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, _UNUSED LPARAM lParam) noexcept override
    {
        // want to check what the originally are
        //auto cap1{ GetCaption(hwnd) }; // Login
        //auto id{ ::GetWindowID(hwndFocus) }; // 1549 IDC_RA_USERNAME
        OnSetText(hwndFocus, NativeStr(RAUsers::LocalUser().Username()).c_str());
        if (RAUsers::LocalUser().Username().length() > 2)
        {
            OnSetFocus(GetDlgItem(hwnd, IDC_RA_PASSWORD), hwndFocus);
            return FALSE;
            //	Must return FALSE if setting to a non-default active control.
        }
        else
        {
            return TRUE;
        }
    }

    void OnCommand(HWND hDlg, int id, _UNUSED  HWND hwndCtl, _UNUSED UINT codeNotify) noexcept override
    {
        switch (id)
        {
            case IDOK:
            {
                using WindowH = std::unique_ptr<HWND__, decltype(&::DestroyWindow)>;
                WindowH hUser{::GetDlgItem(hDlg, IDC_RA_USERNAME), ::DestroyWindow };

                auto bValid{ TRUE };
                tstring sUserEntry{ GetCaption(hUser.get()) };
                bValid &= (sUserEntry.length() > 0U);

                WindowH hPassword{ ::GetDlgItem(hDlg, IDC_RA_PASSWORD), ::DestroyWindow };
                tstring sPassEntry{ GetCaption(hPassword.get()) };
                bValid &= (sPassEntry.length() > 0U);

                if (!bValid)
                {
                    MessageBox(nullptr, TEXT("Username/password not valid! Please check and reenter"), TEXT("Error!"), MB_OK);
                    break;
                }

                PostArgs args;
                args['u'] = ra::Narrow(sUserEntry);
                args['p'] = ra::Narrow(sPassEntry);		//	Plaintext password(!)

                Document doc;
                if (RAWeb::DoBlockingRequest(RequestLogin, args, doc))
                {
                    std::string sResponse;
                    std::string sResponseTitle;

                    if (doc["Success"].GetBool())
                    {
                        const std::string& sUser = doc["User"].GetString();
                        const std::string& sToken = doc["Token"].GetString();
                        const unsigned int nPoints = doc["Score"].GetUint();
                        const unsigned int nUnreadMessages = doc["Messages"].GetUint();

                        WindowH hRememberLogin{ ::GetDlgItem(hDlg, IDC_RA_SAVEPASSWORD), ::DestroyWindow };
                        auto bRememberLogin{Button_GetCheck(hRememberLogin.get())};

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

                    MessageBox(hDlg, NativeStr(sResponse).c_str(), NativeStr(sResponseTitle).c_str(), MB_OK);

                    //	If we are now logged in
                    if (RAUsers::LocalUser().IsLoggedIn())
                    {
                        //	Close this dialog
                        EndDialog(hDlg, IDOK);
                    }

                    break;
                }
                else
                {
                    if (!doc.HasParseError() && doc.HasMember("Error"))
                    {
                        MessageBox(hDlg, NativeStr(std::string("Server error: ") +
                                   std::string(doc["Error"].GetString())).c_str(), TEXT("Error!"), MB_OK);
                    }
                    else
                    {
                        MessageBox(hDlg, TEXT("Unknown error connecting... please try again!"), TEXT("Error!"),
                                   MB_OK);
                    }

                    break;	//	==Handled
                }
            }
            break;

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
        }
    }
};

INT_PTR Dlg_Login::DoModal()
{
    return DialogBoxParam(::GetModuleHandle(_T("RA_Integration.dll")), MAKEINTRESOURCE(IDD_RA_LOGIN),
                          ::GetAncestor(::GetActiveWindow(), GA_ROOT), Dlg_LoginProc,
                          reinterpret_cast<LPARAM>(this));
}

INT_PTR CALLBACK Dlg_LoginProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (auto helper = std::make_unique<LoginHelper>(); uMsg)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, helper->OnInitDialog);
        HANDLE_MSG(hDlg, WM_COMMAND, helper->OnCommand);
        HANDLE_MSG(hDlg, WM_CLOSE, helper->OnClose);
    }
    return FALSE;
}


} // namespace ui
} // namespace ra
