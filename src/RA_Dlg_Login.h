#ifndef RA_DLG_LOGIN_H
#define RA_DLG_LOGIN_H
#pragma once

#ifndef PCH_H
#include "windows_nodefines.h"
#include <wtypes.h>
#endif /* !PCH_H */

class RA_Dlg_Login
{
public:
    static BOOL DoModalLogin();

public:
    static INT_PTR CALLBACK RA_Dlg_LoginProc(HWND, UINT, WPARAM, LPARAM);
};


#endif // !RA_DLG_LOGIN_H
