#ifndef RA_DLG_ACHEDITOR_H
#define RA_DLG_ACHEDITOR_H
#pragma once

#include "RA_Achievement.h"
#include "RA_httpthread.h"

#include "ui\ImageReference.hh"

class BadgeNames
{
public:
    void InstallAchEditorCombo(HWND hCombo) { m_hDestComboBox = hCombo; }

    void FetchNewBadgeNamesThreaded();
    void AddNewBadgeName(const char* pStr, bool bAndSelect);
    void OnNewBadgeNames(const rapidjson::Document& data);

private:
    HWND m_hDestComboBox = nullptr;
};

enum class CondSubItems : std::size_t;

class Dlg_AchievementEditor
{
public:
    Dlg_AchievementEditor();

public:
    static INT_PTR CALLBACK s_AchievementEditorProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR AchievementEditorProc(HWND, UINT, WPARAM, LPARAM);

public:
    void OnLoad_NewRom();
    void LoadAchievement(Achievement* pCheevo, _UNUSED BOOL);

    inline void SetICEControl(HWND hIce) { m_hICEControl = hIce; }
    inline char* LbxDataAt(unsigned int nRow, CondSubItems nCol);

    HWND GetICEControl() const { return m_hICEControl; }

    void InstallHWND(HWND hWnd) { m_hAchievementEditorDlg = hWnd; }
    HWND GetHWND() const { return m_hAchievementEditorDlg; }
    BOOL IsActive() const;

    Achievement* ActiveAchievement() const { return m_pSelectedAchievement; }
    BOOL IsPopulatingAchievementEditorData() const { return m_bPopulatingAchievementEditorData; }
    void SetIgnoreEdits(BOOL bIgnore) { m_bPopulatingAchievementEditorData = bIgnore; }

    void UpdateBadge(const std::string& sNewName); // Call to set/update data
    void UpdateSelectedBadgeImage(
        const std::string& sBackupBadgeToUse = ""); // Call to just update the badge image/bitmap

    BadgeNames& GetBadgeNames() { return m_BadgeNames; }

    size_t GetSelectedConditionGroup() const;
    void SetSelectedConditionGroup(size_t nGrp) const;

    ConditionGroup m_ConditionClipboard;

private:
    void RepopulateGroupList(_In_ const Achievement* const pCheevo);
    void PopulateConditions(_In_ const Achievement* const pCheevo);
    void SetupColumns(HWND hList);

    static LRESULT CALLBACK ListViewWndProc(HWND, UINT, WPARAM, LPARAM);
    void GetListViewTooltip();

    const int AddCondition(HWND hList, const Condition& Cond, unsigned int nCurrentHits);
    void UpdateCondition(HWND hList, LV_ITEM& item, const Condition& Cond, unsigned int nCurrentHits);

private:
    static constexpr std::size_t m_nNumCols = 10U;
    static constexpr std::size_t MAX_CONDITIONS = 200U;
    static constexpr std::size_t MEM_STRING_TEXT_LEN = 80U;

    HWND m_hAchievementEditorDlg = nullptr;
    HWND m_hICEControl = nullptr;

    HWND m_hTooltip = nullptr;
    int m_nTooltipLocation = 0;
    ra::tstring m_sTooltip;
    WNDPROC m_pListViewWndProc = nullptr;

    char m_lbxData[MAX_CONDITIONS][m_nNumCols][MEM_STRING_TEXT_LEN]{};
    TCHAR m_lbxGroupNames[MAX_CONDITIONS][MEM_STRING_TEXT_LEN]{};
    int m_nNumOccupiedRows = 0;

    Achievement* m_pSelectedAchievement = nullptr;
    BOOL m_bPopulatingAchievementEditorData = FALSE;
    ra::ui::ImageReference m_hAchievementBadge;

    BadgeNames m_BadgeNames;
};

void GenerateResizes(HWND hDlg);

extern Dlg_AchievementEditor g_AchievementEditorDialog;

#endif // !RA_DLG_ACHEDITOR_H
