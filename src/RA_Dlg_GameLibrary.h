#ifndef RA_DLG_GAMELIBRARY_H
#define RA_DLG_GAMELIBRARY_H
#pragma once

#include "ra_fwd.h"

class GameEntry
{
public:
    GameEntry(const std::string& sTitle, const std::string& sFile, unsigned int nGameID) :
        m_sTitle(sTitle), m_sFilename(sFile), m_nGameID(nGameID)
    {
    }

    const std::string& Title() const noexcept { return m_sTitle; }
    const std::string& Filename() const noexcept { return m_sFilename; }
    unsigned int GameID() const noexcept { return m_nGameID; }

    const std::string m_sTitle;
    const std::string m_sFilename;
    const unsigned int m_nGameID;
};

class Dlg_GameLibrary
{
public:
    //void DoModalDialog( HINSTANCE hInst, HWND hParent );
    static INT_PTR CALLBACK s_GameLibraryProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK GameLibraryProc(HWND, UINT, WPARAM, LPARAM);

public:
    void InstallHWND(HWND hWnd) noexcept { m_hDialogBox = hWnd; }
    HWND GetHWND() const noexcept { return m_hDialogBox; }

    void AddTitle(const std::string& sTitle, const std::string& sFilename, unsigned int nGameID);
    void ClearTitles() noexcept;

    void LoadAll();
    void SaveAll();

    void KillThread();

private:
    void SetupColumns(HWND hList);
    void ReloadGameListData();
    void ScanAndAddRomsRecursive(const std::string& sBaseDir);
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
    HWND m_hDialogBox{};

    std::map<std::string, unsigned int> m_GameHashLibrary;
    std::map<unsigned int, std::string> m_GameTitlesLibrary;
    std::map<unsigned int, std::string> m_ProgressLibrary;
    std::vector<GameEntry> m_vGameEntries;
};
extern Dlg_GameLibrary g_GameLibrary;


#endif // !RA_DLG_GAMELIBRARY_H
