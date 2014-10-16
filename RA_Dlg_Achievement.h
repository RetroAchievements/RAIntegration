#ifndef _DLG_ACHIEVEMENT_H_
#define _DLG_ACHIEVEMENT_H_

#include "RA_Achievement.h"

#include <wtypes.h>
#include <assert.h>

const int g_nMaxAchievements = 200;
const int g_nMaxTextItemSize = 80;

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
		Author,
		Achieved,
		Active=Achieved,
		Modified,
		Votes=Modified,

		_NumberOfCols,
		_Max=_NumberOfCols
	};

public:
	Dlg_Achievements();

public:
	static INT_PTR CALLBACK s_AchievementsProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR AchievementsProc(HWND, UINT, WPARAM, LPARAM);

public:
	int GetSelectedAchievementIndex();
	void OnLoad_NewRom( unsigned int nGameID );
	void OnGet_Achievement( int nOffs );
	void ReloadLBXData( int nOffset );
	void OnEditData( unsigned int nItem, /*Column*/int nColumn, const char* sNewData );
	void OnEditAchievement( Achievement* pAch );
	void OnClickAchievementSet( AchievementType nAchievementSet );

	inline char* LbxDataAt( unsigned int nRow, unsigned int nCol ) { if(nRow<m_nNumOccupiedRows && nCol<m_nNumCols) return m_lbxData[nRow][nCol]; assert(0); return NULL; }

	inline HWND GetHWND() const		{ return m_hAchievementsDlg; }
	void InstallHWND( HWND hWnd )	{ m_hAchievementsDlg = hWnd; }

private:
	//void Clear();
	void SetupColumns( HWND hList );
	void LoadAchievements( HWND hList );
	BOOL GetRowData( unsigned int nRow, unsigned int& rnID, char*& sTitle, char*& sAuthor, BOOL& bAchieved, BOOL& bModified );
	BOOL FindRowData( unsigned int nAchievementID, char*& sTitle, char*& sAuthor, BOOL& bAchieved );

	void RemoveAchievement( HWND hList, const int nIter );
	const int AddAchievement( HWND hList, const Achievement& Ach );

private:
	static const int m_nNumCols = 5;//;sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

	HWND m_hAchievementsDlg;

	char m_lbxData[g_nMaxAchievements][m_nNumCols][g_nMaxTextItemSize];
	unsigned int m_nNumOccupiedRows;

};

extern Dlg_Achievements g_AchievementsDialog;

#endif //_DLG_ACHIEVEMENT_H_