#ifndef RA_DLG_LOGIN_H
#define RA_DLG_LOGIN_H
#pragma once

#include <wtypes.h>

namespace ra {
namespace ui {

class Dlg_Login
{
public:
    inline constexpr Dlg_Login() noexcept = default;
    ~Dlg_Login() noexcept = default;
    Dlg_Login(const Dlg_Login&) = delete;
    Dlg_Login& operator=(const Dlg_Login&) = delete;
    inline constexpr Dlg_Login(Dlg_Login&&) noexcept = default;
    inline constexpr Dlg_Login& operator=(Dlg_Login&&) noexcept = default;

    INT_PTR DoModal();

private:
    friend INT_PTR CALLBACK Dlg_LoginProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

} // namespace ui
} // namespace ra


#endif // !RA_DLG_LOGIN_H
