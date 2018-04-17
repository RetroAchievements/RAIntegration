#pragma once

#include <wtypes.h>

class RA_Dlg_Login
{
public:
	static BOOL DoModalLogin();

public:
	static INT_PTR CALLBACK RA_Dlg_LoginProc( HWND, UINT, WPARAM, LPARAM );
};
