#pragma once

#include <wtypes.h>
#include <vector>
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
	static void DoModalDialog( HINSTANCE hInst, HWND hParent );
	static INT_PTR CALLBACK s_GameLibraryProc( HWND, UINT, WPARAM, LPARAM );

public:
	static void AddTitle( const std::string& sTitle, const std::string& sFilename, unsigned int nGameID );
	static void ClearTitles();
	
private:
	static void SetupColumns( HWND hList );
	static void ReloadGameListData();
	static void ScanAndAddRomsRecursive( std::string sBaseDir );
	static BOOL LaunchSelected();
	
private:
	static HWND m_hDialogBox;

	static std::map<std::string, unsigned int> m_GameHashLibrary;
	static std::map<unsigned int, std::string> m_GameTitlesLibrary;
	static std::map<unsigned int, std::string> m_ProgressLibrary;
	static std::vector<GameEntry> m_vGameEntries;
};
