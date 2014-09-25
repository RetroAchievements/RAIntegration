#pragma once

#include <wtypes.h>
#include <vector>
#include <deque>
#include <string>
#include <map>

class GameEntry
{
public:
	GameEntry( std::string sTitle, std::string sFile, unsigned int nGameID ) :
		m_sTitle(sTitle), m_sFilename(sFile), m_nGameID(nGameID) {}

	const std::string& Title() const	{ return m_sTitle; }
	const std::string& Filename() const	{ return m_sFilename; }
	unsigned int GameID() const			{ return m_nGameID; }
	
	std::string m_sTitle;
	std::string m_sFilename;
	unsigned int m_nGameID;
};

class Dlg_GameLibrary
{
public:
	Dlg_GameLibrary();
	~Dlg_GameLibrary();

public:
	//void DoModalDialog( HINSTANCE hInst, HWND hParent );
	static INT_PTR CALLBACK s_GameLibraryProc( HWND, UINT, WPARAM, LPARAM );
	INT_PTR CALLBACK GameLibraryProc( HWND, UINT, WPARAM, LPARAM );

public:
	void InstallHWND( HWND hWnd )		{ m_hDialogBox = hWnd; }
	HWND GetHWND() const				{ return m_hDialogBox; }

	void AddTitle( const std::string& sTitle, const std::string& sFilename, unsigned int nGameID );
	void ClearTitles();
	
	void LoadAll();
	void SaveAll();

	void KillThread();
	
private:
	void SetupColumns( HWND hList );
	void ReloadGameListData();
	void ScanAndAddRomsRecursive( std::string sBaseDir );
	BOOL LaunchSelected();
	void RefreshList();

private:
	static std::deque<std::string> FilesToScan;
	static std::map<std::string, std::string> Results;			//	filepath,md5 (parsed/persisted)
	static std::map<std::string, std::string> VisibleResults;	//	filepath,md5 (added to renderable)
	static size_t nNumParsed;

	static void ThreadedScanProc();
	static bool ThreadProcessingAllowed;
	static bool ThreadProcessingActive;
	
private:
	HWND m_hDialogBox;

	std::map<std::string, unsigned int> m_GameHashLibrary;
	std::map<unsigned int, std::string> m_GameTitlesLibrary;
	std::map<unsigned int, std::string> m_ProgressLibrary;
	std::vector<GameEntry> m_vGameEntries;
};
extern Dlg_GameLibrary g_GameLibrary;
