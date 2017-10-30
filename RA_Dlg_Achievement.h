#pragma once

#include "RA_Achievement.h"

#include <wtypes.h>
#include <assert.h>

const int MAX_TEXT_ITEM_SIZE = 80;

//extern const char* g_sColTitles[];
//extern const int g_nColSizes[];

//typedef struct Achievement;

class Dlg_Achievements
{
public:
	enum Column
	{
		ID,
		Title,
    Points,
		Author,
		Achieved,
		Active=Achieved,
		Modified,
		Votes=Modified,

		NUM_COLS
	};

public:
	Dlg_Achievements();

public:
	static INT_PTR CALLBACK s_AchievementsProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR AchievementsProc(HWND, UINT, WPARAM, LPARAM);

public:
	int GetSelectedAchievementIndex();
	void OnLoad_NewRom( GameID nGameID );
	void OnGet_Achievement( const Achievement& ach );
	void ReloadLBXData( int nOffset );
	void OnEditData( size_t nItem, Column nColumn, const std::string& sNewData );
	void OnEditAchievement( const Achievement& ach );
	void OnClickAchievementSet( AchievementSetType nAchievementSet );

	inline std::string& LbxDataAt( size_t nRow, Column nCol )	{ return( m_lbxData[ nRow ] )[ nCol ]; }

	inline HWND GetHWND() const		{ return m_hAchievementsDlg; }
	void InstallHWND( HWND hWnd )	{ m_hAchievementsDlg = hWnd; }

private:
	void SetupColumns( HWND hList );
	void LoadAchievements( HWND hList );

	void RemoveAchievement( HWND hList, int nIter );
	size_t AddAchievement( HWND hList, const Achievement& Ach );

	INT_PTR Dlg_Achievements::CommitAchievements( HWND hDlg );

	void UpdateSelectedAchievementButtons(const Achievement* Cheevo);

private:
	HWND m_hAchievementsDlg;
	typedef std::vector< std::string > AchievementDlgRow;
	std::vector< AchievementDlgRow > m_lbxData;
};

extern Dlg_Achievements g_AchievementsDialog;
