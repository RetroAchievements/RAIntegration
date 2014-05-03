#pragma once

#include <wtypes.h>

INT_PTR CALLBACK GameTitleProc(HWND, UINT, WPARAM, LPARAM);

class Dlg_GameTitle
{
public:
	static void CleanRomName( char* sRomNameRef, unsigned int nLen );
	static unsigned int DoModalDialog( HINSTANCE hInst, HWND hParent, const char* sMD5, const char* sEstimatedGameTitle );

public:
	unsigned int m_nReturnedGameID;
	const char* m_sMD5;
	const char* m_sEstimatedGameTitle;
};

extern Dlg_GameTitle g_GameTitleDialog;




