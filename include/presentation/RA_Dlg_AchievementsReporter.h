#pragma once
#include "RA_Defs.h"

class Achievement;

class Dlg_AchievementsReporter
{
public:
	static const int MAX_ACHIEVEMENTS = 200;
	static const int MAX_TEXT_LEN = 250;

public:
	enum ReporterColumns
	{
		Checked,
		//ID,
		Title,
		Desc,
		Author,
		Achieved,

		NumReporterColumns
	};

public:
	static void DoModalDialog( HINSTANCE hInst, HWND hParent );
	static INT_PTR CALLBACK AchievementsReporterProc( HWND, UINT, WPARAM, LPARAM );

public:
	static void SetupColumns( HWND hList );
	static int AddAchievementToListBox( HWND hList, const Achievement* pAch );

public:
	static int ms_nNumOccupiedRows;
	static char ms_lbxData[ MAX_ACHIEVEMENTS ][ NumReporterColumns ][ MAX_TEXT_LEN ];
};

extern Dlg_AchievementsReporter g_AchievementsReporterDialog;




