#ifndef RA_DLG_ACHIEVEMENT_H
#define RA_DLG_ACHIEVEMENT_H
#pragma once

#include "RA_AchievementSet.h"

inline constexpr int MAX_TEXT_ITEM_SIZE = 80;

class Dlg_Achievements
{
    using AchievementDlgRow = std::vector<std::string>;

public:
    enum class Column : std::size_t
    {
        Id,
        Title,
        Points,
        Author,
        Achieved,
        Active = Achieved,
        Modified,
        Votes = Modified
    };

public:
    static INT_PTR CALLBACK s_AchievementsProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR AchievementsProc(HWND, UINT, WPARAM, LPARAM);

public:
    int GetSelectedAchievementIndex() noexcept;
    void OnLoad_NewRom(unsigned int nGameID);
    void ReloadLBXData(int nOffset);
    void OnEditData(size_t nItem, Column nColumn, const std::string& sNewData);
    void OnEditAchievement(const Achievement& ach);
    void OnClickAchievementSet(_In_ AchievementSet::Type nAchievementSet);

    _NODISCARD inline auto& LbxDataAt(_In_ std::size_t nRow, _In_ Column nCol)
    {
        // we need to check stuff to prevent throwing in release mode
        assert(nRow >= 0 && nRow < m_lbxData.size());
        auto& rowRef{m_lbxData.at(nRow)};

        using namespace ra::rel_ops;
        assert(nCol >= 0 && nCol < rowRef.size());
        return rowRef.at(ra::etoi(nCol));
    }

    inline HWND GetHWND() const noexcept { return m_hAchievementsDlg; }
    void InstallHWND(HWND hWnd) noexcept { m_hAchievementsDlg = hWnd; }

private:
    void SetupColumns(HWND hList);
    void LoadAchievements(HWND hList);

    void RemoveAchievement(HWND hList, int nIter);
    size_t AddAchievement(HWND hList, const Achievement& Ach);
    void AddAchievementRow(const Achievement& Ach);

    INT_PTR CommitAchievements(HWND hDlg);

    void UpdateSelectedAchievementButtons(const Achievement* restrict Cheevo) noexcept;

private:
    HWND m_hAchievementsDlg = nullptr;
    std::vector<AchievementDlgRow> m_lbxData;
};

extern Dlg_Achievements g_AchievementsDialog;

#endif // !RA_DLG_ACHIEVEMENT_H
