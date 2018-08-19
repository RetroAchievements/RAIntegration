#ifndef RA_DLG_ACHIEVEMENT_H
#define RA_DLG_ACHIEVEMENT_H
#pragma once

#include "RA_Achievement.h"

namespace ra {

// More or less would have the same length scope wise so it was taken out

enum class DlgAchievementColumn
{
    ID,
    Title,
    Points,
    Author,
    Achieved,
    Active = Achieved,
    Modified,
    Votes = Modified
};

} // namespace ra

class Dlg_Achievements
{
    using AchievementDlgRow   = std::vector<std::string>;
    using AchievementDlgTable = std::vector<AchievementDlgRow>;
    friend INT_PTR CALLBACK s_AchievementsProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);

public:
    _NODISCARD INT_PTR AchievementsProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);

public:
    _NODISCARD int GetSelectedAchievementIndex();
    void OnLoad_NewRom(_In_ ra::GameID nGameID);
    void OnGet_Achievement(_In_ const Achievement& ach);
    void ReloadLBXData(_In_ int nOffset);
    /*_NORETURN*/ void OnEditData(_In_ size_t nItem, _In_ ra::DlgAchievementColumn nColumn, _In_ const std::string& sNewData);
    void OnEditAchievement(_In_ const Achievement& ach);
    void OnClickAchievementSet(_In_ ra::AchievementSetType nAchievementSet);

    // there's no way this is inline
    _NODISCARD inline std::string& LbxDataAt(_In_ size_t nRow, _In_ ra::DlgAchievementColumn nCol) { return(m_lbxData.at(nRow)).at(ra::etoi(nCol)); }

    _NODISCARD inline HWND GetHWND() const { return m_hAchievementsDlg; }
    void InstallHWND(_In_ HWND hWnd) { m_hAchievementsDlg = hWnd; }

private:
    void SetupColumns(_In_ HWND hList);
    void LoadAchievements(_In_ HWND hList);

    void RemoveAchievement(_In_ HWND hList, _In_ int nIter);
    _NODISCARD size_t AddAchievement(_In_ HWND hList, const Achievement& Ach);

    _NODISCARD INT_PTR CommitAchievements(_In_ HWND hDlg);

    void UpdateSelectedAchievementButtons(_In_ const Achievement* Cheevo);

private:
    HWND m_hAchievementsDlg{};
    AchievementDlgTable m_lbxData;
};

extern Dlg_Achievements g_AchievementsDialog;


#endif // !RA_DLG_ACHIEVEMENT_H
