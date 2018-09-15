#ifndef RA_DLG_ACHEDITOR_H
#define RA_DLG_ACHEDITOR_H
#pragma once

#include "RA_httpthread.h"
#include "RA_Achievement.h"

#include "services\ImageRepository.h"

namespace {
const size_t MAX_CONDITIONS = 200;
const size_t MEM_STRING_TEXT_LEN = 80;
}

class BadgeNames
{
public:
    BadgeNames()
        : m_hDestComboBox(nullptr)
    {
    }
    void InstallAchEditorCombo(HWND hCombo) { m_hDestComboBox = hCombo; }

    void FetchNewBadgeNamesThreaded();
    void AddNewBadgeName(const char* pStr, bool bAndSelect);
    void OnNewBadgeNames(const rapidjson::Document& data);

private:
    HWND m_hDestComboBox;
};


class Dlg_AchievementEditor
{
public:
    Dlg_AchievementEditor();
    ~Dlg_AchievementEditor();

public:
    static INT_PTR CALLBACK s_AchievementEditorProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR AchievementEditorProc(HWND, UINT, WPARAM, LPARAM);
public:
    void OnLoad_NewRom();
    void LoadAchievement(Achievement* pCheevo, BOOL bAttemptKeepScrollbar);

    inline void SetICEControl(HWND hIce) { m_hICEControl = hIce; }
    inline char* LbxDataAt(unsigned int nRow, unsigned int nCol) { return m_lbxData[nRow][nCol]; }

    HWND GetICEControl() const { return m_hICEControl; }

    void InstallHWND(HWND hWnd) { m_hAchievementEditorDlg = hWnd; }
    HWND GetHWND() const { return m_hAchievementEditorDlg; }
    BOOL IsActive() const;

    Achievement* ActiveAchievement() const { return m_pSelectedAchievement; }
    BOOL IsPopulatingAchievementEditorData() const { return m_bPopulatingAchievementEditorData; }
    void SetIgnoreEdits(BOOL bIgnore) { m_bPopulatingAchievementEditorData = bIgnore; }

    void UpdateBadge(const std::string& sNewName);							//	Call to set/update data
    void UpdateSelectedBadgeImage(const std::string& sBackupBadgeToUse = "");	//	Call to just update the badge image/bitmap

    BadgeNames& GetBadgeNames() { return m_BadgeNames; }

    size_t GetSelectedConditionGroup() const;
    void SetSelectedConditionGroup(size_t nGrp) const;

    ConditionGroup m_ConditionClipboard;

private:
    void RepopulateGroupList(Achievement* pCheevo);
    void PopulateConditions(Achievement* pCheevo);
    void SetupColumns(HWND hList);

    static LRESULT CALLBACK ListViewWndProc(HWND, UINT, WPARAM, LPARAM);
    void GetListViewTooltip();

    const int AddCondition(HWND hList, const Condition& Cond, unsigned int nCurrentHits);
    void UpdateCondition(HWND hList, LV_ITEM& item, const Condition& Cond, unsigned int nCurrentHits);

private:
    static const int m_nNumCols = 10;//;sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

    HWND m_hAchievementEditorDlg;
    HWND m_hICEControl;

    HWND m_hTooltip;
    int m_nTooltipLocation;
    ra::tstring m_sTooltip;
    WNDPROC m_pListViewWndProc;

    char m_lbxData[MAX_CONDITIONS][m_nNumCols][MEM_STRING_TEXT_LEN];
    TCHAR m_lbxGroupNames[MAX_CONDITIONS][MEM_STRING_TEXT_LEN];
    int m_nNumOccupiedRows;

    Achievement* m_pSelectedAchievement;
    BOOL m_bPopulatingAchievementEditorData;
    ra::services::ImageReference m_hAchievementBadge;

    BadgeNames m_BadgeNames;
};

void GenerateResizes(HWND hDlg);

extern Dlg_AchievementEditor g_AchievementEditorDialog;


#endif // !RA_DLG_ACHEDITOR_H
