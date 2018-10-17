#ifndef RA_DLG_ACHIEVEMENTSREPORTER_H
#define RA_DLG_ACHIEVEMENTSREPORTER_H
#pragma once
#include "RA_Defs.h"

class Achievement;

class Dlg_AchievementsReporter
{
    inline static constexpr auto MAX_ACHIEVEMENTS{ 200 };
    inline static constexpr auto MAX_TEXT_LEN{ 250 };

    enum class Columns
    {
        Checked,
        Title,
        Desc,
        Author,
        Achieved
    };
    inline static constexpr std::array<const LPCTSTR, 5> COL_TITLE
    {
        _T(""),
        _T("Title"),
        _T("Description"),
        _T("Author"),
        _T("Achieved?")
    };
    inline static constexpr std::array<int, 5> COL_SIZE{ 19, 105, 205, 75, 62 };

public:
    static void DoModalDialog(HINSTANCE hInst, HWND hParent);
    static INT_PTR CALLBACK AchievementsReporterProc(HWND, UINT, WPARAM, _UNUSED LPARAM);

public:
    static void SetupColumns(HWND hList);
    _NODISCARD static int AddAchievementToListBox(_In_ HWND hList, _In_ const Achievement* const pAch);

public:
    static int ms_nNumOccupiedRows;
    static char ms_lbxData[MAX_ACHIEVEMENTS][COL_SIZE.size()][MAX_TEXT_LEN];
};

extern Dlg_AchievementsReporter g_AchievementsReporterDialog;






#endif // !RA_DLG_ACHIEVEMENTSREPORTER_H
