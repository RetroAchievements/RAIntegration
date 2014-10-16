#ifndef _DLG_ACHIEVEMENTEDITOR_H_
#define _DLG_ACHIEVEMENTEDITOR_H_

#include <wtypes.h>
#include <vector>

#include "RA_httpthread.h"
#include "RA_Achievement.h"

namespace
{
	const int g_nMaxConditions = 200;
	const int g_nMaxMemStringTextItemSize = 80;
}

//struct Achievement;

//extern const char* g_sColTitles[];
//extern const int g_nColSizes[];

class BadgeNames
{
public:
	BadgeNames()
	{
		m_hDestComboBox = NULL;
	}

	void InstallAchEditorCombo( HWND hCombo )	{ m_hDestComboBox = hCombo; }

	//cb
	static void CB_OnNewBadgeNames( void* pObj );
	void FetchNewBadgeNamesThreaded();
	void AddNewBadgeName( const char* pStr, BOOL bAndSelect );

private:
	static HWND m_hDestComboBox;
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
	void LoadAchievement( Achievement* pCheevo, BOOL bAttemptKeepScrollbar );

	inline void SetICEControl( HWND hIce )							{ m_hICEControl = hIce; }
	inline char* LbxDataAt( unsigned int nRow, unsigned int nCol )	{ return m_lbxData[nRow][nCol]; }

	HWND GetICEControl() const										{ return m_hICEControl; }

	void InstallHWND( HWND hWnd )									{ m_hAchievementEditorDlg = hWnd; }
	HWND GetHWND() const											{ return m_hAchievementEditorDlg; }

	Achievement* ActiveAchievement() const							{ return m_pSelectedAchievement; }

	BOOL IsPopulatingAchievementEditorData() const					{ return m_bPopulatingAchievementEditorData; }

	void SetIgnoreEdits( BOOL bIgnore )								{ m_bPopulatingAchievementEditorData = bIgnore; }

	void UpdateBadge( const char* sNewBadgeName );					//	Call to set/update data
	void UpdateSelectedBadgeImage(const char* sBackupBadgeToUse=NULL);	//	Call to just update the badge image/bitmap

	BadgeNames& GetBadgeNames()										{ return m_BadgeNames; }

	int GetSelectedConditionGroup() const;
	void SetSelectedConditionGroup( int nGrp ) const;

private:
	void RepopulateGroupList( Achievement* pCheevo );
	void PopulateConditions( Achievement* pCheevo );
	//void Clear();
	void SetupColumns( HWND hList );

	const int AddCondition( HWND hList, const Condition& Cond );

private:
	static const int m_nNumCols = 10;//;sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

	HWND m_hAchievementEditorDlg;
	HWND m_hICEControl;

	char m_lbxData[g_nMaxConditions][m_nNumCols][g_nMaxMemStringTextItemSize];
	char m_lbxGroupNames[g_nMaxConditions][g_nMaxMemStringTextItemSize];
	int m_nNumOccupiedRows;

	Achievement* m_pSelectedAchievement;

	BOOL m_bPopulatingAchievementEditorData;

	HBITMAP m_hAchievementBadge;

	BadgeNames m_BadgeNames;
};

extern Dlg_AchievementEditor g_AchievementEditorDialog;


#endif //_DLG_ACHIEVEMENT_H_