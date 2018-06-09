#include "RA_Dlg_GameLibrary.h"

#include <mutex>
#include <stack>

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



}

//static 
std::deque<std::string> Dlg_GameLibrary::FilesToScan;
std::map<std::string, std::string> Dlg_GameLibrary::Results;	//	filepath,md5
std::map<std::string, std::string> Dlg_GameLibrary::VisibleResults;	//	filepath,md5
size_t Dlg_GameLibrary::nNumParsed = 0;
bool Dlg_GameLibrary::ThreadProcessingAllowed = true;
bool Dlg_GameLibrary::ThreadProcessingActive = false;

Dlg_GameLibrary g_GameLibrary;

_Success_(return == true)
_NODISCARD bool CALLBACK ListFiles(
    _Inout_ std::string& path,
    _In_ const std::string& mask,
    _Out_ std::deque<std::string>& rFileListOut)
{
    // you must ensure out variables are in an empty state or better, use the out param as a return type instead (not ref)
    if (!rFileListOut.empty())
        rFileListOut.clear();

    std::stack<std::string> directories;
    directories.push(path);

    // If probably got destroyed too early
    
    

    while (!directories.empty())
    {
        path = directories.top();
        std::string spec = path + "\\" + mask;
        directories.pop();

        auto lpfd{std::make_unique<WIN32_FIND_DATA>()};
        auto hFind{ ra::make_find_file(NativeStr(spec).c_str(), lpfd.get()) };

        do
        {
            if (auto& sFilename = Narrow(lpfd->cFileName); (sFilename  == ".") or (sFilename == ".."))
                continue;
            else if (lpfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                directories.push(path + "\\" + sFilename);
            else if (((FindNextFile(hFind.get(), lpfd.get()) == FALSE)) or (::GetLastError() == ERROR_NO_MORE_FILES))
            {
                hFind.reset();
                return false;
            }
            else
                rFileListOut.push_front(path + "\\" + sFilename);
        } while(FindNextFile(hFind.get(), lpfd.get()) != FALSE);

        // A static function calls this so we have to reset it manually, the
        // other one doesn't matter because it justs delete (calls compiler generated destructor)
        hFind.reset();
    }

    return true;
}

//HWND Dlg_GameLibrary::m_hDialogBox = nullptr;
//std::vector<GameEntry> Dlg_GameLibrary::m_vGameEntries;
//std::map<std::string, unsigned int> Dlg_GameLibrary::m_GameHashLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_GameTitlesLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_ProgressLibrary;
Dlg_GameLibrary::Dlg_GameLibrary()
    : m_hDialogBox(nullptr)
{
}

Dlg_GameLibrary::~Dlg_GameLibrary()
{
}

_NORETURN void CALLBACK ParseGameHashLibraryFromFile(_Out_ std::map<std::string, GameID>& GameHashLibraryOut)
{
    if (!GameHashLibraryOut.empty())
        GameHashLibraryOut.clear();

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
                GameHashLibraryOut.try_emplace(sMD5, nID);
            }
        }

        fclose(pf);
    }
}


_NORETURN void CALLBACK ParseGameTitlesFromFile(_Out_ std::map<GameID, std::string>& GameTitlesListOut)
{
    if (!GameTitlesListOut.empty())
        GameTitlesListOut.clear();

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
                GameTitlesListOut.try_emplace(nID, sTitle);
            }
        }

        fclose(pf);
    }
}

_NORETURN void CALLBACK ParseMyProgressFromFile(_Out_ std::map<GameID, std::string>& GameProgressOut)
{
    if (!GameProgressOut.empty())
        GameProgressOut.clear();

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
                    // The stream probably dangled by now (invalid state)
                    const int nNumEarnedTotal = nEarned + nEarnedHardcore;
                    auto bufPct = std::make_unique<char[]>(256);
                    sprintf_s(bufPct.get(), 256, " (%1.1f%%)", (nNumEarnedTotal / static_cast<float>(nNumAchievements)) * 100.0f);
                    auto myStr = sstr.str();
                    sstr.str("");
                    sstr << myStr << bufPct.get();
                }

                GameProgressOut.try_emplace(nID, sstr.str());
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
        // mutexes from C++ are meant to be static locals encased in a lock
        static std::mutex mtx;
        if (std::scoped_lock myLock{ mtx }; !Dlg_GameLibrary::ThreadProcessingAllowed)
            break;

        FILE* pf = nullptr;
        if (fopen_s(&pf, FilesToScan.front().c_str(), "rb") == 0)
        {
            // obtain file size:
            fseek(pf, 0, SEEK_END);
            DWORD nSize = ftell(pf);
            rewind(pf);

            static BYTE* pBuf = nullptr;	//	static (optimisation)
            if (pBuf == nullptr)
                pBuf = new BYTE[6 * 1024 * 1024];

            //BYTE* pBuf = new BYTE[ nSize ];	//	Do we really need to load this into memory?
            if (pBuf != nullptr)
            {
                fread(pBuf, sizeof(BYTE), nSize, pf);	//Check
                Results[FilesToScan.front()] = RAGenerateMD5(pBuf, nSize);

                // TODO: Use the the appropriate literals when PR 23 is accepted
                SendMessage(g_GameLibrary.GetHWND(), WM_TIMER, WPARAM{}, LPARAM{});
            }

            fclose(pf);
        }

        {
            std::scoped_lock myLock{ mtx };
            FilesToScan.pop_front();
        }
    }

    Dlg_GameLibrary::ThreadProcessingActive = false;
    ExitThread(0);
}

_Use_decl_annotations_
void Dlg_GameLibrary::ScanAndAddRomsRecursive(const std::string& sBaseDir)
{
    // static for no reason...
    auto sSearchDir = std::make_unique<char[]>(2048U);
    sprintf_s(sSearchDir.get(), 2048, "%s\\*.*", sBaseDir.c_str());

    auto lpfd{ std::make_unique<WIN32_FIND_DATA>() };
    auto hFind{ ra::make_find_file(NativeStr(sSearchDir.get()).c_str(), lpfd.get()) };

    // You probably shouldn't call this function then if you are expecting it to be invalid

    // why not just make it a global if it's always the same?
    constexpr auto ROM_MAX_SIZE{ ra::to_unsigned(6 * 1024 * 1024) }; 
    auto sROMRawData{ std::make_unique<unsigned char[]>(ROM_MAX_SIZE) };

    do
    {
        if (KEYDOWN(VK_ESCAPE))
            break;
        else if (const auto sFilename = Narrow(lpfd->cFileName); ((sFilename  == ".") or (sFilename == "..")))
        {
            //	Ignore 'this'
        }
        else if (lpfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RA_LOG("Directory found: %s\n", lpfd->cFileName);
            std::string sRecurseDir = sBaseDir + "\\" + sFilename.c_str();
            ScanAndAddRomsRecursive(sRecurseDir);
        }
        else
        {

            if (std::unique_ptr<LARGE_INTEGER> filesize{
                new LARGE_INTEGER{lpfd->nFileSizeLow, ra::to_signed(lpfd->nFileSizeHigh)}
                }; ((filesize->QuadPart < 2048) || (filesize->QuadPart > ra::to_signed(ROM_MAX_SIZE))))
            {
                //	Ignore: wrong size
                RA_LOG("Ignoring %s, wrong size\n", sFilename.c_str());
            }
            else
            {
                //	Parse as ROM!
                RA_LOG("%s looks good: parsing!\n", sFilename.c_str());

                auto sAbsFileDir = std::make_unique<char[]>(2048U);
                sprintf_s(sAbsFileDir.get(), 2048, "%s\\%s", sBaseDir.c_str(), sFilename.c_str());

                auto hROMReader{ ra::make_file(NativeStr(sAbsFileDir.get()).c_str(), GENERIC_READ,
                    FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL) };

                auto lpFileInfo{ std::make_unique<BY_HANDLE_FILE_INFORMATION>() };
                DWORD nSize = DWORD{};
                if (GetFileInformationByHandle(hROMReader.get(), lpFileInfo.get()))
                    nSize = (lpFileInfo->nFileSizeHigh << 16) + lpFileInfo->nFileSizeLow;

                DWORD nBytes = DWORD{};
                if (auto bResult{ ReadFile(hROMReader.get(), sROMRawData.get(), nSize, &nBytes, nullptr)
                    }; bResult == FALSE)
                {
                    ra::ThrowLastError();
                }

                // Braces are construction, = is assignment
                const auto sHashOut{ RAGenerateMD5(sROMRawData.get(), nSize) };

                if (m_GameHashLibrary.find(sHashOut) != m_GameHashLibrary.end())
                {
                    const auto nGameID = m_GameHashLibrary.at(sHashOut);
                    RA_LOG("Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary.at(nGameID).c_str());

                    const auto& sGameTitle{ m_GameTitlesLibrary.at(nGameID) };
                    AddTitle(sGameTitle, sAbsFileDir.get(), nGameID);
                    SetDlgItemText(m_hDialogBox, IDC_RA_GLIB_NAME, NativeStr(sGameTitle).c_str());
                    InvalidateRect(m_hDialogBox, nullptr, TRUE);
                }
            }
        }
    } while (FindNextFile(hFind.get(), lpfd.get()) != 0);

    SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning complete"));

}

void Dlg_GameLibrary::ReloadGameListData()
{
    ClearTitles();

    TCHAR sROMDir[1024];
    GetDlgItemText(m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024);


    {
        static std::mutex mtx;
        std::scoped_lock myLock{ mtx };
        while (FilesToScan.size() > 0)
            FilesToScan.pop_front();
    }

    // You only accounted for gens and nothing else.
    bool bOK{ bool{} };
    if(g_EmulatorID == EmulatorID::RA_Gens)
    {
        bOK = ListFiles(Narrow(sROMDir), "*.bin", FilesToScan);
        bOK |= ListFiles(Narrow(sROMDir), "*.gen", FilesToScan);
    }
    else if (g_EmulatorID == EmulatorID::RA_FCEUX)
    {
        bOK = ListFiles(Narrow(sROMDir), "*.nes" , FilesToScan);
        bOK |= ListFiles(Narrow(sROMDir), "*.*", FilesToScan);
    }

    if (bOK)
    {
        std::thread scanner(&Dlg_GameLibrary::ThreadedScanProc);
        scanner.detach();
    }
}

void Dlg_GameLibrary::RefreshList()
{
    std::map<std::string, std::string>::iterator iter = Results.begin();
    while (iter != Results.end())
    {
        const std::string& filepath = iter->first;
        const std::string& md5 = iter->second;

        if (VisibleResults.find(filepath) == VisibleResults.end())
        {
            //	Not yet added,
            if (m_GameHashLibrary.find(md5) != m_GameHashLibrary.end())
            {
                //	Found in our hash library!
                const GameID nGameID = m_GameHashLibrary[md5];
                RA_LOG("Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[nGameID].c_str());

                const std::string& sGameTitle = m_GameTitlesLibrary[nGameID];
                AddTitle(sGameTitle, filepath, nGameID);

                SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, NativeStr(sGameTitle).c_str());
                VisibleResults[filepath] = md5;	//	Copy to VisibleResults
            }
        }
        iter++;
    }
}

BOOL Dlg_GameLibrary::LaunchSelected()
{
    HWND hList = GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST);
    const int nSel = ListView_GetSelectionMark(hList);
    if (nSel != -1)
    {
        TCHAR buffer[1024];
        ListView_GetItemText(hList, nSel, 1, buffer, 1024);
        SetWindowText(GetDlgItem(m_hDialogBox, IDC_RA_GLIB_NAME), buffer);

        ListView_GetItemText(hList, nSel, 3, buffer, 1024);
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
    static std::mutex mtx;

    // stdio already locks FILE via EnterCriticalSection, the way it was before probably caused a data race or deadlock
    FILE* pLoadIn = nullptr;
    fopen_s(&pLoadIn, RA_MY_GAME_LIBRARY_FILENAME, "rb");

    if (std::scoped_lock myLock{ mtx }; pLoadIn != nullptr)
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
}

void Dlg_GameLibrary::SaveAll()
{
    static std::mutex mtx;

    FILE* pf = nullptr;
    fopen_s(&pf, RA_MY_GAME_LIBRARY_FILENAME, "wb");
    if (std::scoped_lock myLock{ mtx }; pf != nullptr)
    {
        std::map<std::string, std::string>::iterator iter = Results.begin();
        while (iter != Results.end())
        {
            const std::string& sFilepath = iter->first;
            const std::string& sMD5 = iter->second;

            fwrite(sFilepath.c_str(), sizeof(char), strlen(sFilepath.c_str()), pf);
            fwrite("\n", sizeof(char), 1, pf);
            fwrite(sMD5.c_str(), sizeof(char), strlen(sMD5.c_str()), pf);
            fwrite("\n", sizeof(char), 1, pf);

            iter++;
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
            HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST);
            SetupColumns(hList);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

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
                            HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST);
                            const int nSel = ListView_GetSelectionMark(hList);
                            if (nSel != -1)
                            {
                                TCHAR buffer[1024];
                                ListView_GetItemText(hList, nSel, 1, buffer, 1024);
                                SetWindowText(GetDlgItem(hDlg, IDC_RA_GLIB_NAME), buffer);
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
                {

                    ReloadGameListData();
                    static std::mutex mtx;
                    {
                        std::scoped_lock myLock{ mtx };
                        SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning..."));
                    }
                }
                return FALSE;

                case IDC_RA_PICKROMDIR:
                    g_sROMDirLocation = GetFolderFromDialog();
                    RA_LOG("Selected Folder: %s\n", g_sROMDirLocation.c_str());
                    SetDlgItemText(hDlg, IDC_RA_ROMDIR, NativeStr(g_sROMDirLocation).c_str());
                    return FALSE;

                case IDC_RA_LBX_GAMELIST:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST);
                    const int nSel = ListView_GetSelectionMark(hList);
                    if (nSel != -1)
                    {
                        TCHAR sGameTitle[1024];
                        ListView_GetItemText(hList, nSel, 1, sGameTitle, 1024);
                        SetWindowText(GetDlgItem(hDlg, IDC_RA_GLIB_NAME), sGameTitle);
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
