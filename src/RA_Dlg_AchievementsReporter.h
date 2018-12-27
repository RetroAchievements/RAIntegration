#ifndef RA_DLG_ACHIEVEMENTSREPORTER_H
#define RA_DLG_ACHIEVEMENTSREPORTER_H
#pragma once

#include "ra_fwd.h"

class Achievement;

class Dlg_AchievementsReporter
{
    inline static constexpr auto MAX_ACHIEVEMENTS{200};
    inline static constexpr auto MAX_TEXT_LEN{250};

    enum class Column : std::size_t
    {
        Checked,
        Title,
        Desc,
        Author,
        Achieved
    };
    inline static constexpr std::array<LPCTSTR, 5> COL_TITLE{_T(""), _T("Title"), _T("Description"), _T("Author"),
                                                             _T("Achieved?")};
    inline static constexpr std::array<int, 5> COL_SIZE{19, 105, 205, 75, 62};

public:
    static void DoModalDialog(HINSTANCE hInst, HWND hParent) noexcept;

private:
    static INT_PTR CALLBACK AchievementsReporterProc(HWND, UINT, WPARAM, _UNUSED LPARAM);

    static void SetupColumns(HWND hList);
    static void AddAchievementToListBox(_In_ HWND hList, _In_ const Achievement* restrict pAch);

    inline static constexpr auto LbxDataAt(gsl::index row, gsl::index column) noexcept
    {
        return gsl::at(gsl::at(ms_lbxData, row), column);
    }

    static gsl::index ms_nNumOccupiedRows;
    static char ms_lbxData[MAX_ACHIEVEMENTS][COL_SIZE.size()][MAX_TEXT_LEN];
};

extern Dlg_AchievementsReporter g_AchievementsReporterDialog;

#endif // !RA_DLG_ACHIEVEMENTSREPORTER_H
