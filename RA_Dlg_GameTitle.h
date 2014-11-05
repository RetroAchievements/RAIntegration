#pragma once

#include <wtypes.h>
#include <map>

#include "RA_Defs.h"

class Dlg_GameTitle
{
public:
	static void CleanRomName( char* sRomNameRef, unsigned int nLen );
	static unsigned int DoModalDialog( HINSTANCE hInst, HWND hParent, const std::string& sMD5, const std::string& sEstimatedGameTitle );
	
	INT_PTR GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	GameID m_nReturnedGameID;
	std::string m_sMD5;
	std::string m_sEstimatedGameTitle;

	std::map<std::string, GameID> m_aGameTitles;
};

extern Dlg_GameTitle g_GameTitleDialog;
