#ifndef RA_DLG_LOGIN_H
#define RA_DLG_LOGIN_H
#pragma once

#include "ira_dialog.h"

namespace ra {

class Dlg_Login :
    public IRA_Dialog
{
public:
    using IRA_Dialog::IRA_Dialog;
    Dlg_Login() noexcept;
    BOOL InitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept override;
    
protected:
    _NORETURN void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept override;
    _NORETURN void NCDestroy(HWND hwnd) noexcept override;
    /* it should throw but not sure how to deal with it so it's not marked [[noreturn]]*/
    void OnOK(HWND hwnd) noexcept override; 
    _NODISCARD INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

private:
    // Don't initialize controls in constructors, do it in WM_INITDIALOG
    HWND m_hUsername{ nullptr };
    HWND m_hPassword{ nullptr };
    HWND m_hSavePassword{ nullptr };
    IRA_WndClass m_WndClass{ TEXT("Login") };
}; 

} // namespace ra


#endif // !RA_DLG_LOGIN_H
