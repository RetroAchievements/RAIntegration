#pragma once

#include <wtypes.h>
#include <vector>
#include <deque>
#include <map>
//#include <string>
#include "RA_Defs.h"

class GameEntry
{
public:
    GameEntry(const std::string& sTitle, const std::string& sFile, unsigned int nGameID) :
        m_sTitle(sTitle), m_sFilename(sFile), m_nGameID(nGameID)
    {
    }

    const std::string& Title() const { return m_sTitle; }
    const std::string& Filename() const { return m_sFilename; }
    unsigned int GameID() const { return m_nGameID; }

    const std::string m_sTitle;
    const std::string m_sFilename;
    const unsigned int m_nGameID;
};

class Dlg_GameLibrary
{
    // temp, needed for possible non-zero terminated strings
    // Already have a class wrapper for this later, if you wanted to use this for
    // controls as members, you have to initialize them in either WM_NCCREATE or
    // WM_INITDIALOG depending if it's a window or modeless dialog.
    // Initializing them in the constructor will have no effect.
    using WindowH = std::unique_ptr<std::remove_pointer_t<HWND>, decltype(&::DestroyWindow)>;
public:
    //void DoModalDialog( HINSTANCE hInst, HWND hParent );
    static INT_PTR CALLBACK s_GameLibraryProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK GameLibraryProc(HWND, UINT, WPARAM, LPARAM);

public:
    void InstallHWND(HWND hWnd) { m_hDialogBox = hWnd; }
    HWND GetHWND() const { return m_hDialogBox; }

    void AddTitle(const std::string& sTitle, const std::string& sFilename, unsigned int nGameID);
    void ClearTitles();

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
    HWND m_hDialogBox{ nullptr };

    std::map<std::string, GameID> m_GameHashLibrary;
    std::map<GameID, std::string> m_GameTitlesLibrary;
    std::map<GameID, std::string> m_ProgressLibrary;
    std::vector<GameEntry> m_vGameEntries;

    // this got really repetitive
    WindowH hList{ nullptr, &::DestroyWindow };
};
extern Dlg_GameLibrary g_GameLibrary;
