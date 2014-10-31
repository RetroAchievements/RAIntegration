#pragma once

#include <wtypes.h>
#include <map>

#include "RA_Defs.h"

class Dlg_GameTitle
{
public:
	static void CleanRomName( char* sRomNameRef, unsigned int nLen );
	static unsigned int DoModalDialog( HINSTANCE hInst, HWND hParent, const char* sMD5, const char* sEstimatedGameTitle );
	
	INT_PTR GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	unsigned int m_nReturnedGameID;
	const char* m_sMD5;
	const char* m_sEstimatedGameTitle;

	std::map<std::string, GameID> m_aGameTitles;
};

extern Dlg_GameTitle g_GameTitleDialog;
