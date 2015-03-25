#pragma once

#include <wtypes.h>
#include <map>

#include "RA_Defs.h"

class Dlg_GameTitle
{
public:
	std::string CleanRomName( const std::string& sTryName );
	static void DoModalDialog( HINSTANCE hInst, HWND hParent, std::string& sMD5InOut, std::string& sEstimatedGameTitleInOut, GameID& nGameIDOut );
	
	INT_PTR GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	GameID m_nReturnedGameID;
	std::string m_sMD5;
	std::string m_sEstimatedGameTitle;

	std::map<std::string, GameID> m_aGameTitles;
};

extern Dlg_GameTitle g_GameTitleDialog;
