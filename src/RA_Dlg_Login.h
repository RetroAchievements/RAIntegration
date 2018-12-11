#ifndef RA_DLG_LOGIN_H
#define RA_DLG_LOGIN_H
#pragma once

class RA_Dlg_Login
{
public:
    static INT_PTR DoModalLogin() noexcept;

public:
    static INT_PTR CALLBACK RA_Dlg_LoginProc(HWND, UINT, WPARAM, [[maybe_unused]] LPARAM);
};


#endif // !RA_DLG_LOGIN_H
