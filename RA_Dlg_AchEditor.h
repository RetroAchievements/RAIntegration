#pragma once

#include <wtypes.h>
#include <vector>

#include "RA_httpthread.h"
#include "RA_Achievement.h"

namespace
{
	const int g_nMaxConditions = 200;
	const int g_nMaxMemStringTextItemSize = 80;
}

class BadgeNames
{
public:
	BadgeNames()
	 :	m_hDestComboBox( nullptr ) {}
	void InstallAchEditorCombo( HWND hCombo )	{ m_hDestComboBox = hCombo; }
	
	void FetchNewBadgeNamesThreaded();
	void AddNewBadgeName( const char* pStr, BOOL bAndSelect );
	void OnNewBadgeNames( const Document& data );

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
	void LoadAchievement( Achievement* pCheevo, BOOL bAttemptKeepScrollbar );

	inline void SetICEControl( HWND hIce )							{ m_hICEControl = hIce; }
	inline char* LbxDataAt( unsigned int nRow, unsigned int nCol )	{ return m_lbxData[nRow][nCol]; }

	HWND GetICEControl() const										{ return m_hICEControl; }

	void InstallHWND( HWND hWnd )									{ m_hAchievementEditorDlg = hWnd; }
	HWND GetHWND() const											{ return m_hAchievementEditorDlg; }

	Achievement* ActiveAchievement() const							{ return m_pSelectedAchievement; }
	BOOL IsPopulatingAchievementEditorData() const					{ return m_bPopulatingAchievementEditorData; }
	void SetIgnoreEdits( BOOL bIgnore )								{ m_bPopulatingAchievementEditorData = bIgnore; }

	void UpdateBadge( const std::string& sNewName );							//	Call to set/update data
	void UpdateSelectedBadgeImage( const std::string& sBackupBadgeToUse = "" );	//	Call to just update the badge image/bitmap

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
