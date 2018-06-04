#include "RA_Dlg_GameLibrary.h"

#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <queue>
#include <deque>
#include <stack>
#include <thread>
#include <shlobj.h>
#include <fstream>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"

#define KEYDOWN(vkCode) ((GetAsyncKeyState(vkCode) & 0x8000) ? true : false)

namespace {

const char* COL_TITLE[] ={ "ID", "Game Title", "Completion", "File Path" };
const int COL_SIZE[] ={ 30, 230, 110, 170 };
static_assert(SIZEOF_ARRAY(COL_TITLE) == SIZEOF_ARRAY(COL_SIZE), "Must match!");
const bool bCancelScan = false;

std::mutex mtx;

}

//static 
std::deque<std::string> Dlg_GameLibrary::FilesToScan;
std::map<std::string, std::string> Dlg_GameLibrary::Results;	//	filepath,md5
std::map<std::string, std::string> Dlg_GameLibrary::VisibleResults;	//	filepath,md5
size_t Dlg_GameLibrary::nNumParsed = 0;
bool Dlg_GameLibrary::ThreadProcessingAllowed = true;
bool Dlg_GameLibrary::ThreadProcessingActive = false;

Dlg_GameLibrary g_GameLibrary;

bool ListFiles(std::string path, std::string mask, std::deque<std::string>& rFileListOut)
{
    std::stack<std::string> directories;
    directories.push(path);

    while (!directories.empty())
    {
        path = directories.top();
        std::string spec = path + "\\" + mask;
        directories.pop();

        WIN32_FIND_DATA ffd = WIN32_FIND_DATA{};
        // how is this a deleted function?
        auto hFind{ ra::make_find_file(NativeStr(spec).c_str(), &ffd) };
        do
        {
            if (auto sFilename = Narrow(ffd.cFileName)
                ; (sFilename == ".") or (sFilename == ".."))
                continue;

            else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                directories.push(path + "\\" + sFilename);
            else
                rFileListOut.push_front(path + "\\" + sFilename);
        } while (FindNextFile(hFind.get(), &ffd) != 0);
    }

    return true;
}

//HWND Dlg_GameLibrary::m_hDialogBox = nullptr;
//std::vector<GameEntry> Dlg_GameLibrary::m_vGameEntries;
//std::map<std::string, unsigned int> Dlg_GameLibrary::m_GameHashLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_GameTitlesLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_ProgressLibrary;


void ParseGameHashLibraryFromFile(std::map<std::string, GameID>& GameHashLibraryOut)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pf = nullptr;
    fopen_s(&pf, RA_GAME_HASH_FILENAME, "rb");
    if (pf != nullptr)
    {
        Document doc;
        doc.ParseStream(FileStream(pf));

        if (!doc.HasParseError() && doc.HasMember("Success") && doc["Success"].GetBool() && doc.HasMember("MD5List"))
        {
            const Value& List = doc["MD5List"];
            for (Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter)
            {
                if (iter->name.IsNull() || iter->value.IsNull())
                    continue;

                const std::string sMD5 = iter->name.GetString();
                //GameID nID = static_cast<GameID>( std::strtoul( iter->value.GetString(), nullptr, 10 ) );	//	MUST BE STRING, then converted to uint. Keys are strings ONLY
                GameID nID = static_cast<GameID>(iter->value.GetUint());
                GameHashLibraryOut[sMD5] = nID;
            }
        }

        fclose(pf);
    }
}

void ParseGameTitlesFromFile(std::map<GameID, std::string>& GameTitlesListOut)
{
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pf = nullptr;
    fopen_s(&pf, RA_TITLES_FILENAME, "rb");
    if (pf != nullptr)
    {
        Document doc;
        doc.ParseStream(FileStream(pf));

        if (!doc.HasParseError() && doc.HasMember("Success") && doc["Success"].GetBool() && doc.HasMember("Response"))
        {
            const Value& List = doc["Response"];
            for (Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter)
            {
                if (iter->name.IsNull() || iter->value.IsNull())
                    continue;

                GameID nID = static_cast<GameID>(std::strtoul(iter->name.GetString(), nullptr, 10));	//	KEYS ARE STRINGS, must convert afterwards!
                const std::string sTitle = iter->value.GetString();
                GameTitlesListOut[nID] = sTitle;
            }
        }

        fclose(pf);
    }
}

void ParseMyProgressFromFile(std::map<GameID, std::string>& GameProgressOut)
{
    FILE* pf = nullptr;
    fopen_s(&pf, RA_MY_PROGRESS_FILENAME, "rb");
    if (pf != nullptr)
    {
        Document doc;
        doc.ParseStream(FileStream(pf));

        if (!doc.HasParseError() && doc.HasMember("Success") && doc["Success"].GetBool() && doc.HasMember("Response"))
        {
            //{"ID":"7","NumAch":"14","Earned":"10","HCEarned":"0"},

            const Value& List = doc["Response"];
            for (Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter)
            {
                GameID nID = static_cast<GameID>(std::strtoul(iter->name.GetString(), nullptr, 10));	//	KEYS MUST BE STRINGS
                const unsigned int nNumAchievements = iter->value["NumAch"].GetUint();
                const unsigned int nEarned = iter->value["Earned"].GetUint();
                const unsigned int nEarnedHardcore = iter->value["HCEarned"].GetUint();

                std::stringstream sstr;
                sstr << nEarned;
                if (nEarnedHardcore > 0)
                    sstr << " (" << std::to_string(nEarnedHardcore) << ")";
                sstr << " / " << nNumAchievements;
                if (nNumAchievements > 0)
                {
                    const int nNumEarnedTotal = nEarned + nEarnedHardcore;
                    char bufPct[256];
                    auto fPercent{
                        ra::to_floating(
                            ra::to_unsigned(nNumEarnedTotal) / nNumAchievements
                        )
                    };
                    sprintf_s(bufPct, 256, " (%1.1f%%)", static_cast<double>(fPercent) * 100.0);
                    sstr << bufPct;
                }

                GameProgressOut[nID] = sstr.str();
            }
        }

        fclose(pf);
    }
}

void Dlg_GameLibrary::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0)) {}

    //	Remove all data.
    ListView_DeleteAllItems(hList);

    LV_COLUMN col;
    ZeroMemory(&col, sizeof(col));

    for (size_t i = 0; i < SIZEOF_ARRAY(COL_TITLE); ++i)
    {
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.cchTextMax = 255;
        tstring sCol = COL_TITLE[i];	//	scoped cache
        col.pszText = const_cast<LPTSTR>(sCol.c_str());
        col.cx = COL_SIZE[i];
        col.iSubItem = i;

        col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
        if (i == SIZEOF_ARRAY(COL_TITLE) - 1)	//	Final column should fill
        {
            col.fmt |= LVCFMT_FILL;
        }

        ListView_InsertColumn(hList, i, reinterpret_cast<LPARAM>(&col));
    }
}

//static
void Dlg_GameLibrary::AddTitle(const std::string& sTitle, const std::string& sFilename, GameID nGameID)
{
    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT;
    item.cchTextMax = 255;
    item.iItem = m_vGameEntries.size();

    HWND hList = GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST);

    //	id:
    item.iSubItem = 0;
    tstring sID = std::to_string(nGameID);	//scoped cache!
    item.pszText = const_cast<LPTSTR>(sID.c_str());
    item.iItem = ListView_InsertItem(hList, &item);

    item.iSubItem = 1;
    ListView_SetItemText(hList, item.iItem, 1, const_cast<LPTSTR>(NativeStr(sTitle).c_str()));

    item.iSubItem = 2;
    ListView_SetItemText(hList, item.iItem, 2, const_cast<LPTSTR>(NativeStr(m_ProgressLibrary[nGameID]).c_str()));

    item.iSubItem = 3;
    ListView_SetItemText(hList, item.iItem, 3, const_cast<LPTSTR>(NativeStr(sFilename).c_str()));

    m_vGameEntries.push_back(GameEntry(sTitle, sFilename, nGameID));
}

void Dlg_GameLibrary::ClearTitles()
{
    nNumParsed = 0;

    m_vGameEntries.clear();

    ListView_DeleteAllItems(GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST));
    VisibleResults.clear();
}

//static
void Dlg_GameLibrary::ThreadedScanProc()
{
    Dlg_GameLibrary::ThreadProcessingActive = true;

    while (FilesToScan.size() > 0)
    {
        if (std::scoped_lock lock{ mtx }; !Dlg_GameLibrary::ThreadProcessingAllowed)
            break;

        std::ifstream ifile{ FilesToScan.front(), std::ios::app, std::ios::ate };
        if (std::scoped_lock lock{ mtx }; !ifile.is_open())
            break;


        // obtain file size:

        auto nSize = ifile.gcount();
        // This clears the state bit not the file
        ifile.clear();
        ifile.seekg(0LL);

        // making it explicitly static does not optimize anything
        auto pBuf = std::make_unique<char[]>(ra::to_unsigned(6 * 1024 * 1024));
        ifile.read(pBuf.get(), nSize); //Check

        std::basic_ostringstream<BYTE> oss;
        std::string tmp{ pBuf.get() };
        pBuf.reset(); // don't need it no more

        for (auto& i : tmp)
            oss << ra::to_unsigned(i);

        auto ustr{ oss.str() };
        Results.try_emplace(FilesToScan.front(), RAGenerateMD5(ustr.c_str(), nSize));

        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms644902(v=vs.85).aspx
        FORWARD_WM_TIMER(g_GameLibrary.GetHWND(), 0U, PostMessage);

        // pBuf destroyed here
    }

    {
        std::scoped_lock myLock{ mtx };
        FilesToScan.pop_front();
    }


    Dlg_GameLibrary::ThreadProcessingActive = false;
    ExitThread(0);
}

void Dlg_GameLibrary::ScanAndAddRomsRecursive(const std::string& sBaseDir)
{
    char sSearchDir[2048];
    sprintf_s(sSearchDir, 2048, "%s\\*.*", sBaseDir.c_str());

    WIN32_FIND_DATA ffd = WIN32_FIND_DATA{};
    auto hFind = ra::make_find_file(NativeStr(sSearchDir).c_str(), &ffd);

    // it would throw an exception otherwise

    constexpr auto ROM_MAX_SIZE = ra::to_unsigned(6 * 1024 * 1024);
    auto sROMRawData{ std::make_unique<unsigned char[]>(ROM_MAX_SIZE) };

    do
    {
        if (KEYDOWN(VK_ESCAPE))
            break;
        // what is with this memset s...

        const std::string sFilename = Narrow(ffd.cFileName);
        if (strcmp(sFilename.c_str(), ".") == 0 ||
            strcmp(sFilename.c_str(), "..") == 0)
        {
            //	Ignore 'this'
        }
        else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RA_LOG("Directory found: %s\n", ffd.cFileName);
            std::string sRecurseDir = sBaseDir + "\\" + sFilename.c_str();
            ScanAndAddRomsRecursive(sRecurseDir);
        }
        else
        {
            LARGE_INTEGER filesize;
            filesize.LowPart = ffd.nFileSizeLow;
            filesize.HighPart = ffd.nFileSizeHigh;
            if (filesize.QuadPart < 2048 || filesize.QuadPart > ROM_MAX_SIZE)
            {
                //	Ignore: wrong size
                RA_LOG("Ignoring %s, wrong size\n", sFilename.c_str());
            }
            else
            {
                //	Parse as ROM!
                RA_LOG("%s looks good: parsing!\n", sFilename.c_str());

                char sAbsFileDir[2048];
                sprintf_s(sAbsFileDir, 2048, "%s\\%s", sBaseDir.c_str(), sFilename.c_str());

                auto hROMReader{ ra::make_file(NativeStr(sAbsFileDir).c_str(),GENERIC_READ, FILE_SHARE_READ,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL) };


                BY_HANDLE_FILE_INFORMATION File_Inf=BY_HANDLE_FILE_INFORMATION{};
                int nSize = 0;
                if (GetFileInformationByHandle(hROMReader.get(), &File_Inf))
                    nSize = (File_Inf.nFileSizeHigh << 16) + File_Inf.nFileSizeLow;

                DWORD nBytes = 0UL;

                if (BOOL bResult = ReadFile(hROMReader.get(), sROMRawData.get(), nSize, &nBytes, nullptr
                ); (bResult == ERROR_INVALID_USER_BUFFER) or (bResult == ERROR_NOT_ENOUGH_MEMORY))
                    ra::ThrowLastError();

                const std::string sHashOut = RAGenerateMD5(sROMRawData.get(), nSize);

                if (m_GameHashLibrary.find(sHashOut) != m_GameHashLibrary.end())
                {
                    const unsigned int nGameID = m_GameHashLibrary[std::string(sHashOut)];
                    RA_LOG("Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[nGameID].c_str());

                    const std::string& sGameTitle = m_GameTitlesLibrary[nGameID];
                    AddTitle(sGameTitle, sAbsFileDir, nGameID);
                    SetDlgItemText(m_hDialogBox, IDC_RA_GLIB_NAME, NativeStr(sGameTitle).c_str());
                    InvalidateRect(m_hDialogBox, nullptr, TRUE);
                }
            }
        }
    } while (FindNextFile(hFind.get(), &ffd) != 0);

    SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning complete"));
}

void Dlg_GameLibrary::ReloadGameListData()
{
    ClearTitles();

    TCHAR sROMDir[1024];
    GetDlgItemText(m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024);

    {
        std::scoped_lock lock{ mtx };
        while (FilesToScan.size() > 0)
            FilesToScan.pop_front();
    }

    bool bOK = ListFiles(Narrow(sROMDir), "*.bin", FilesToScan);
    bOK |= ListFiles(Narrow(sROMDir), "*.gen", FilesToScan);

    if (bOK)
    {
        std::thread scanner(&Dlg_GameLibrary::ThreadedScanProc);
        scanner.detach();
    }
}

void Dlg_GameLibrary::RefreshList()
{
    for (auto& resultPair : Results)
    {
        auto filepath = resultPair.first;
        auto md5      = resultPair.second;

        for (auto& i : VisibleResults)
        {
            //	Not yet added,
            if (m_GameHashLibrary.find(md5) != m_GameHashLibrary.end())
            {
                //	Found in our hash library!
                auto nGameID = m_GameHashLibrary.at(md5);
                RA_LOG("Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[nGameID].c_str());

                auto sGameTitle = m_GameTitlesLibrary.at(nGameID);
                AddTitle(sGameTitle, filepath, nGameID);

                SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, NativeStr(sGameTitle).c_str());
                const auto _ = VisibleResults.try_emplace(filepath, md5);	//	Copy to VisibleResults
            }
        }
    }
}

BOOL Dlg_GameLibrary::LaunchSelected()
{


    WindowH hList{ GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST), &::DestroyWindow };

    if (const auto nSel = ListView_GetSelectionMark(hList.get()); nSel != -1)
    {
        auto len{ ::GetWindowTextLength(hList.get()) + 1 };
        tstring buffer;
        buffer.reserve(ra::to_unsigned(len));

        ListView_GetItemText(hList.get(), nSel, 1, buffer.data(), len);
        SetWindowText(GetDlgItem(m_hDialogBox, IDC_RA_GLIB_NAME), buffer.data());

        ListView_GetItemText(hList.get(), nSel, 3, buffer.data(), len);
        _RA_LoadROM(Narrow(buffer).c_str());

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void Dlg_GameLibrary::LoadAll()
{
    mtx.lock();
    FILE* pLoadIn = nullptr;
    fopen_s(&pLoadIn, RA_MY_GAME_LIBRARY_FILENAME, "rb");
    if (pLoadIn != nullptr)
    {
        DWORD nCharsRead1 = 0;
        DWORD nCharsRead2 = 0;
        do
        {
            nCharsRead1 = 0;
            nCharsRead2 = 0;
            char fileBuf[2048];
            char md5Buf[64];
            ZeroMemory(fileBuf, 2048);
            ZeroMemory(md5Buf, 64);
            _ReadTil('\n', fileBuf, 2048, &nCharsRead1, pLoadIn);

            if (nCharsRead1 > 0)
            {
                _ReadTil('\n', md5Buf, 64, &nCharsRead2, pLoadIn);
            }

            if (fileBuf[0] != '\0' && md5Buf[0] != '\0' && nCharsRead1 > 0 && nCharsRead2 > 0)
            {
                fileBuf[nCharsRead1 - 1] = '\0';
                md5Buf[nCharsRead2 - 1] = '\0';

                //	Add
                std::string file = fileBuf;
                std::string md5 = md5Buf;

                Results[file] = md5;
            }

        } while (nCharsRead1 > 0 && nCharsRead2 > 0);
        fclose(pLoadIn);
    }
    mtx.unlock();
}

void Dlg_GameLibrary::SaveAll()
{
    FILE* pf = nullptr;
    fopen_s(&pf, RA_MY_GAME_LIBRARY_FILENAME, "wb");
    if (std::scoped_lock myLock{ mtx }; pf != nullptr)
    {
        for (auto& myPair : Results)
        {
            auto sFilepath = myPair.first;
            auto sMD5 = myPair.second;

            fwrite(sFilepath.c_str(), sizeof(char), strlen(sFilepath.c_str()), pf);
            fwrite("\n", sizeof(char), 1, pf);
            fwrite(sMD5.c_str(), sizeof(char), strlen(sMD5.c_str()), pf);
            fwrite("\n", sizeof(char), 1, pf);
        }

        fclose(pf);
    }
}

//static 
INT_PTR CALLBACK Dlg_GameLibrary::s_GameLibraryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_GameLibrary.GameLibraryProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK Dlg_GameLibrary::GameLibraryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            WindowH myWindow{ ::GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST), &::DestroyWindow };
            hList.swap(myWindow);
            SetupColumns(hList.get());

            ListView_SetExtendedListViewStyle(hList.get(), LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

            SetDlgItemText(hDlg, IDC_RA_ROMDIR, NativeStr(g_sROMDirLocation).c_str());
            SetDlgItemText(hDlg, IDC_RA_GLIB_NAME, TEXT(""));

            m_GameHashLibrary.clear();
            m_GameTitlesLibrary.clear();
            m_ProgressLibrary.clear();
            ParseGameHashLibraryFromFile(m_GameHashLibrary);
            ParseGameTitlesFromFile(m_GameTitlesLibrary);
            ParseMyProgressFromFile(m_ProgressLibrary);

            //int msBetweenRefresh = 1000;	//	auto?
            //SetTimer( hDlg, 1, msBetweenRefresh, (TIMERPROC)g_GameLibrary.s_GameLibraryProc );
            RefreshList();

            return FALSE;
        }

        case WM_TIMER:
            if ((g_GameLibrary.GetHWND() != nullptr) && (IsWindowVisible(g_GameLibrary.GetHWND())))
                RefreshList();
            //ReloadGameListData();
            return FALSE;

        case WM_NOTIFY:
            switch (LOWORD(wParam))
            {
                case IDC_RA_LBX_GAMELIST:
                {
                    switch (((LPNMHDR)lParam)->code)
                    {
                        case LVN_ITEMCHANGED:
                        {
                            //RA_LOG( "Item Changed\n" );

                            if (const auto nSel = ListView_GetSelectionMark(hList.get()); nSel != -1)
                            {
                                auto len{ ::GetWindowTextLength(::GetDlgItem(hDlg, IDC_RA_GLIB_NAME)) + 1 };
                                tstring buffer;
                                buffer.reserve(ra::to_unsigned(len));
                                ListView_GetItemText(hList.get(), nSel, 1, buffer.data(), len);
                                SetWindowText(GetDlgItem(hDlg, IDC_RA_GLIB_NAME), buffer.data());
                            }
                        }
                        break;

                        case NM_CLICK:
                            //RA_LOG( "Click\n" );
                            break;

                        case NM_DBLCLK:
                            if (LaunchSelected())
                            {
                                EndDialog(hDlg, TRUE);
                                return TRUE;
                            }
                            break;

                        default:
                            break;
                    }
                }
                return FALSE;

                default:
                    RA_LOG("%08x, %08x\n", wParam, lParam);
                    return FALSE;
            }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (LaunchSelected())
                    {
                        EndDialog(hDlg, TRUE);
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }

                case IDC_RA_RESCAN:
                    ReloadGameListData();

                    mtx.lock();	//?
                    SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning..."));
                    mtx.unlock();
                    return FALSE;

                case IDC_RA_PICKROMDIR:
                    g_sROMDirLocation = GetFolderFromDialog();
                    RA_LOG("Selected Folder: %s\n", g_sROMDirLocation.c_str());
                    SetDlgItemText(hDlg, IDC_RA_ROMDIR, NativeStr(g_sROMDirLocation).c_str());
                    return FALSE;

                case IDC_RA_LBX_GAMELIST:
                {

                    if (const auto nSel = ListView_GetSelectionMark(hList.get()); nSel != -1)
                    {
                        auto len{ ::GetWindowTextLength(GetDlgItem(hDlg, IDC_RA_GLIB_NAME)) + 1 };
                        tstring sGameTitle;
                        sGameTitle.reserve(ra::to_unsigned(len));
                        ListView_GetItemText(hList.get(), nSel, 1, sGameTitle.data(), len);
                        SetWindowText(GetDlgItem(hDlg, IDC_RA_GLIB_NAME), sGameTitle.data());
                    }
                }
                return FALSE;

                case IDC_RA_REFRESH:
                    RefreshList();
                    return FALSE;

                default:
                    return FALSE;
            }

        case WM_PAINT:
            if (nNumParsed != Results.size())
                nNumParsed = Results.size();
            return FALSE;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;

        case WM_USER:
            return FALSE;

        default:
            return FALSE;
    }
}

void Dlg_GameLibrary::KillThread()
{
    Dlg_GameLibrary::ThreadProcessingAllowed = false;
    while (Dlg_GameLibrary::ThreadProcessingActive)
    {
        RA_LOG("Waiting for background scanner...");
        Sleep(200);
    }
}

////static
//void Dlg_GameLibrary::DoModalDialog( HINSTANCE hInst, HWND hParent )
//{
//	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), hParent, Dlg_GameLibrary::s_GameLibraryProc );
//}
