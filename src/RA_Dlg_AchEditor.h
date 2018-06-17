#ifndef RA_DLG_ACHEDITOR_H
#define RA_DLG_ACHEDITOR_H
#pragma once



#include "RA_httpthread.h"
#include "RA_Achievement.h"

namespace {
const size_t MAX_CONDITIONS = 200;
const size_t MEM_STRING_TEXT_LEN = 80;
}

class BadgeNames
{
public:
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    void InstallAchEditorCombo(HWND hCombo) { m_hDestComboBox = hCombo; }
#pragma warning(pop)

    void FetchNewBadgeNamesThreaded();
    void AddNewBadgeName(const char* pStr, bool bAndSelect);
    void OnNewBadgeNames(const Document& data) const;

private:
    HWND m_hDestComboBox{ nullptr };
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

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    inline void SetICEControl(HWND hIce) { m_hICEControl = hIce; }
    inline char* LbxDataAt(unsigned int nRow, unsigned int nCol) { return m_lbxData[nRow][nCol]; }

    HWND GetICEControl() const { return m_hICEControl; }

    void InstallHWND(HWND hWnd) { m_hAchievementEditorDlg = hWnd; }
    HWND GetHWND() const { return m_hAchievementEditorDlg; }

    BOOL IsPopulatingAchievementEditorData() const { return m_bPopulatingAchievementEditorData; }
    void SetIgnoreEdits(BOOL bIgnore) { m_bPopulatingAchievementEditorData = bIgnore; }

    auto& GetBadgeNames() const { return m_BadgeNames; }
    auto ActiveAchievement() const { return m_pSelectedAchievement; }
#pragma warning(pop)

    BOOL IsActive() const;

    
    

    void UpdateBadge(const std::string& sNewName);							//	Call to set/update data
    void UpdateSelectedBadgeImage(const std::string& sBackupBadgeToUse = "");	//	Call to just update the badge image/bitmap

    size_t GetSelectedConditionGroup() const;
    void SetSelectedConditionGroup(size_t nGrp) const;

    ConditionGroup m_ConditionClipboard;

private:
    void RepopulateGroupList(Achievement* pCheevo);
    void PopulateConditions(Achievement* pCheevo);
    void SetupColumns(HWND hList);

    const int AddCondition(HWND hList, const Condition& Cond);
    void UpdateCondition(HWND hList, LV_ITEM& item, const Condition& Cond);

private:
    static const int m_nNumCols = 10;//;sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

    HWND m_hAchievementEditorDlg;
    HWND m_hICEControl;

    char m_lbxData[MAX_CONDITIONS][m_nNumCols][MEM_STRING_TEXT_LEN];
    TCHAR m_lbxGroupNames[MAX_CONDITIONS][MEM_STRING_TEXT_LEN];
    int m_nNumOccupiedRows;

    Achievement* m_pSelectedAchievement;
    BOOL m_bPopulatingAchievementEditorData;
    HBITMAP m_hAchievementBadge;

    BadgeNames m_BadgeNames;
};

void GenerateResizes(HWND hDlg);

extern Dlg_AchievementEditor g_AchievementEditorDialog;


#endif // !RA_DLG_ACHEDITOR_H
