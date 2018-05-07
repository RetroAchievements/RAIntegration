#pragma once
#include "RA_Defs.h"

class Achievement;

constexpr auto MAX_ACHIEVEMENTS = 200;
constexpr auto MAX_TEXT_LEN     = 250;

class Dlg_AchievementsReporter
{
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
    static void DoModalDialog(HINSTANCE hInst, HWND hParent);
    static INT_PTR CALLBACK AchievementsReporterProc(HWND, UINT, WPARAM, LPARAM);

public:
    static void SetupColumns(HWND hList);
    static int AddAchievementToListBox(HWND hList, const Achievement* pAch);

public:
    static int ms_nNumOccupiedRows;
    static char ms_lbxData[MAX_ACHIEVEMENTS][NumReporterColumns][MAX_TEXT_LEN];
};

extern Dlg_AchievementsReporter g_AchievementsReporterDialog;




