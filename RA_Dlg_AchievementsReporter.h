#pragma once

#include <wtypes.h>

INT_PTR CALLBACK AchievementsReporterProc(HWND, UINT, WPARAM, LPARAM);

const int MAX_ACHIEVEMENTS = 200;
const int MAX_TEXT_SIZE = 250;

class Achievement;

class Dlg_AchievementsReporter
{
public:
	enum ReporterColumns
	{
		COL_CHECKED,
		//COL_ID,
		COL_TITLE,
		COL_DESC,
		COL_AUTHOR,
		COL_ACHIEVED,

		COL_NUMCOLS
	};
public:
	//static void CleanRomName( char* sRomNameRef, unsigned int nLen );
	static void DoModalDialog( HINSTANCE hInst, HWND hParent );

public:
	static void SetupColumns( HWND hList );
	static int AddAchievementToListBox( HWND hList, const Achievement* pAch );

public:
	static int m_nNumOccupiedRows;
	//static char m_lbxData[g_nMaxAchievements][COL_NUMCOLS][g_nMaxTextItemSize];
	static char m_lbxData[MAX_ACHIEVEMENTS][COL_NUMCOLS][MAX_TEXT_SIZE];
};

extern Dlg_AchievementsReporter g_AchievementsReporterDialog;




