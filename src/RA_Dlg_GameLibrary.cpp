#include "RA_Dlg_GameLibrary.h"

#include "RA_Achievement.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ra_math.h"

#define KEYDOWN(vkCode) ((GetAsyncKeyState(vkCode) & 0x8000) ? true : false)

inline constexpr std::array<LPCTSTR, 4> COL_TITLE{_T("ID"), _T("Game Title"), _T("Completion"), _T("File Path")};
inline constexpr std::array<int, 4> COL_SIZE{30, 230, 110, 170};
inline constexpr auto bCancelScan = false;

std::mutex mtx;

// static
std::deque<std::string> Dlg_GameLibrary::FilesToScan;
std::map<std::string, std::string> Dlg_GameLibrary::Results;        //	filepath,md5
std::map<std::string, std::string> Dlg_GameLibrary::VisibleResults; //	filepath,md5
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

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(NativeStr(spec).c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE)
            return false;

        do
        {
            std::string sFilename = ra::Narrow(ffd.cFileName);
            if ((strcmp(sFilename.c_str(), ".") == 0) || (strcmp(sFilename.c_str(), "..") == 0))
                continue;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                directories.push(path + "\\" + sFilename);
            else
                rFileListOut.push_front(path + "\\" + sFilename);
        } while (FindNextFile(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            FindClose(hFind);
            return false;
        }

        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return true;
}

namespace ra {

inline static void LogErrno() noexcept
{
    std::array<char, 2048> buf{};
    strerror_s(buf.data(), sizeof(buf), errno);
    // TODO: Make StringPrintf support std::array
    GSL_SUPPRESS_F6 RA_LOG("Error: %s", buf.data());
}

} /* namespace ra */

void ParseGameHashLibraryFromFile(std::map<std::string, unsigned int>& GameHashLibraryOut)
{
    std::wstring sGameHashFile{g_sHomeDir};
    sGameHashFile += RA_GAME_HASH_FILENAME;
    std::ifstream ifile{sGameHashFile};
    if (!ifile.is_open())
    {
        ra::LogErrno();
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ifile};
    doc.ParseStream(isw);

    if ((!doc.HasParseError() && doc.HasMember("Success")) && (doc["Success"].GetBool() && doc.HasMember("MD5List")))
    {
        const auto& List{doc["MD5List"]};
        for (auto iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter)
        {
            if (iter->name.IsNull() || iter->value.IsNull())
                continue;

            GameHashLibraryOut.try_emplace(iter->name.GetString(), iter->value.GetUint());
        }
    }
}

void ParseGameTitlesFromFile(std::map<unsigned int, std::string>& GameTitlesListOut)
{
    std::wstring sTitlesFile{g_sHomeDir};
    sTitlesFile += RA_TITLES_FILENAME;
    std::ifstream ifile{sTitlesFile};
    if (!ifile.is_open())
    {
        ra::LogErrno();
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ifile};
    doc.ParseStream(isw);

    if ((!doc.HasParseError() && doc.HasMember("Success")) && (doc["Success"].GetBool() && doc.HasMember("Response")))
    {
        const auto& List{doc["Response"]};
        for (auto iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter)
        {
            if (iter->name.IsNull() || iter->value.IsNull())
                continue;

            //	KEYS ARE STRINGS, must convert afterwards!
            GameTitlesListOut.try_emplace(std::stoul(iter->name.GetString()), iter->value.GetString());
        }
    }
}

void ParseMyProgressFromFile(std::map<unsigned int, std::string>& GameProgressOut)
{
    std::wstring sProgressFile{g_sHomeDir};
    sProgressFile += RA_TITLES_FILENAME;

    std::ifstream ifile{sProgressFile, std::ios::binary};
    if (!ifile.is_open())
    {
        ra::LogErrno();
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ifile};
    doc.ParseStream(isw);

    if ((!doc.HasParseError() && doc.HasMember("Success")) && (doc["Success"].GetBool() && doc.HasMember("Response")))
    {
        //{"ID":"7","NumAch":"14","Earned":"10","HCEarned":"0"},

        const auto& List = doc["Response"];
        for (auto iter = List.MemberBegin(); iter != List.MemberBegin(); ++iter)
        {
            const auto nNumAchievements{iter->value["NumAch"].GetUint()};
            const auto nEarned{iter->value["Earned"].GetUint()};
            const auto nEarnedHardcore{iter->value["HCEarned"].GetUint()};

            std::ostringstream sstr;
            sstr << nEarned;
            if (nEarnedHardcore > 0U)
                sstr << " (" << nEarnedHardcore << ")";
            sstr << " / " << nNumAchievements;
            if (nNumAchievements > 0U)
            {
                const auto fNumEarnedTotal{ra::to_floating(nEarned + nEarnedHardcore)};
                const auto fVal{(fNumEarnedTotal / ra::to_floating(nNumAchievements)) * 100.0F};
                sstr << std::fixed << std::setw(1) << std::setprecision(1) << std::dec << fVal << '%';
            }
            //	KEYS MUST BE STRINGS
            GameProgressOut.try_emplace(std::stoul(iter->name.GetString()), sstr.str());
        }
    }
}

void Dlg_GameLibrary::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0))
    {
    }

    //	Remove all data.
    ListView_DeleteAllItems(hList);

    auto i = 0;
    for (const auto title : COL_TITLE)
    {
        ra::tstring sCol{title}; // scoped cache
        LV_COLUMN col{col.mask = ra::to_unsigned(LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT),
                      col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH,
                      col.cx = COL_SIZE.at(i),
                      col.pszText = sCol.data(),
                      col.cchTextMax = 255,
                      col.iSubItem = i};

        if (i == ra::to_signed(COL_TITLE.size() - 1)) // Final column should fill
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, i, &col);
        i++;
    }
}

// static
void Dlg_GameLibrary::AddTitle(const std::string& sTitle, const std::string& sFilename, unsigned int nGameID)
{
    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT;
    item.cchTextMax = 255;
    item.iItem = m_vGameEntries.size();

    HWND hList = GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST);

    //	id:
    item.iSubItem = 0;
    auto sID = NativeStr(ra::ToString(nGameID)); // scoped cache!
    item.pszText = sID.data();
    item.iItem = ListView_InsertItem(hList, &item);

    item.iSubItem = 1;
    ListView_SetItemText(hList, item.iItem, 1, NativeStr(sTitle).data());

    item.iSubItem = 2;
    ListView_SetItemText(hList, item.iItem, 2, NativeStr(m_ProgressLibrary[nGameID]).data());

    item.iSubItem = 3;
    ListView_SetItemText(hList, item.iItem, 3, NativeStr(sFilename).data());

    // NB: Perfect forwarding seems to cause an access violation here, so it's using an rvalue instead
    m_vGameEntries.emplace_back(GameEntry(sTitle, sFilename, nGameID));
}

void Dlg_GameLibrary::ClearTitles() noexcept
{
    nNumParsed = 0;

    m_vGameEntries.clear();

    ListView_DeleteAllItems(GetDlgItem(m_hDialogBox, IDC_RA_LBX_GAMELIST));
    VisibleResults.clear();
}

// static
void Dlg_GameLibrary::ThreadedScanProc()
{
    Dlg_GameLibrary::ThreadProcessingActive = true;

    while (FilesToScan.size() > 0)
    {
        mtx.lock();
        if (!Dlg_GameLibrary::ThreadProcessingAllowed)
        {
            mtx.unlock();
            break;
        }
        mtx.unlock();

        FILE* pf = nullptr;
        if ((fopen_s(&pf, FilesToScan.front().c_str(), "rb") == 0) && pf)
        {
            // obtain file size:
            fseek(pf, 0, SEEK_END);
            const DWORD nSize = ftell(pf);
            rewind(pf);

            auto pBuf = std::make_unique<unsigned char[]>(6 * 1024 * 1024);

            fread(pBuf.get(), sizeof(unsigned char), nSize, pf); // Check
            Results.insert_or_assign(FilesToScan.front(), RAGenerateMD5(pBuf.get(), nSize));
            pBuf.reset();

            SendMessage(g_GameLibrary.GetHWND(), WM_TIMER, 0U, 0L);

            fclose(pf);
        }

        mtx.lock();
        FilesToScan.pop_front();
        mtx.unlock();
    }

    Dlg_GameLibrary::ThreadProcessingActive = false;
    ExitThread(0);
}

void Dlg_GameLibrary::ScanAndAddRomsRecursive(const std::string& sBaseDir)
{
    char sSearchDir[2048];
    sprintf_s(sSearchDir, 2048, "%s\\*.*", sBaseDir.c_str());

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(NativeStr(sSearchDir).c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        constexpr unsigned int ROM_MAX_SIZE = 6 * 1024 * 1024;
        unsigned char* sROMRawData = new unsigned char[ROM_MAX_SIZE];

        do
        {
            if (KEYDOWN(VK_ESCAPE))
                break;

            memset(sROMRawData, 0, ROM_MAX_SIZE); //?!??

            const std::string sFilename = ra::Narrow(ffd.cFileName);
            if (strcmp(sFilename.c_str(), ".") == 0 || strcmp(sFilename.c_str(), "..") == 0)
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

                    HANDLE hROMReader = CreateFile(NativeStr(sAbsFileDir).c_str(), GENERIC_READ, FILE_SHARE_READ,
                                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

                    if (hROMReader != INVALID_HANDLE_VALUE)
                    {
                        BY_HANDLE_FILE_INFORMATION File_Inf{};
                        int nSize = 0;
                        if (GetFileInformationByHandle(hROMReader, &File_Inf))
                            nSize = (File_Inf.nFileSizeHigh << 16) + File_Inf.nFileSizeLow;

                        DWORD nBytes = 0;
                        if (::ReadFile(hROMReader, sROMRawData, nSize, &nBytes, nullptr) == 0)
                            return;
                        const std::string sHashOut = RAGenerateMD5(sROMRawData, nSize);

                        if (m_GameHashLibrary.find(sHashOut) != m_GameHashLibrary.end())
                        {
                            const unsigned int nGameID = m_GameHashLibrary[std::string(sHashOut)];
                            RA_LOG("Found one! Game ID %u (%s)", nGameID, m_GameTitlesLibrary[nGameID].c_str());

                            const std::string& sGameTitle = m_GameTitlesLibrary[nGameID];
                            AddTitle(sGameTitle, sAbsFileDir, nGameID);
                            SetDlgItemText(m_hDialogBox, IDC_RA_GLIB_NAME, NativeStr(sGameTitle).c_str());
                            InvalidateRect(m_hDialogBox, nullptr, TRUE);
                        }

                        CloseHandle(hROMReader);
                    }
                }
            }
        } while (FindNextFile(hFind, &ffd) != 0);

        delete[](sROMRawData);
        sROMRawData = nullptr;

        FindClose(hFind);
    }

    SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning complete"));
}

void Dlg_GameLibrary::ReloadGameListData()
{
    ClearTitles();

    TCHAR sROMDir[1024];
    GetDlgItemText(m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024);

    mtx.lock();
    while (FilesToScan.size() > 0)
        FilesToScan.pop_front();
    mtx.unlock();

    bool bOK = ListFiles(ra::Narrow(sROMDir), "*.bin", FilesToScan);
    bOK |= ListFiles(ra::Narrow(sROMDir), "*.gen", FilesToScan);

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
                const auto nGameID = m_GameHashLibrary[md5];
                RA_LOG("Found one! Game ID %u (%s)", nGameID, m_GameTitlesLibrary[nGameID].c_str());

                const std::string& sGameTitle = m_GameTitlesLibrary[nGameID];
                AddTitle(sGameTitle, filepath, nGameID);

                SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, NativeStr(sGameTitle).c_str());
                VisibleResults[filepath] = md5; //	Copy to VisibleResults
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
        TCHAR buffer[1024]{};
        ListView_GetItemText(hList, nSel, 1, buffer, 1024);
        SetWindowText(GetDlgItem(m_hDialogBox, IDC_RA_GLIB_NAME), buffer);

        ListView_GetItemText(hList, nSel, 3, buffer, 1024);
        _RA_LoadROM(ra::Narrow(buffer).c_str());

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void Dlg_GameLibrary::LoadAll()
{
    const auto sMyGameLibraryFile = ra::StringPrintf(L"%s%s", g_sHomeDir, RA_MY_GAME_LIBRARY_FILENAME);
    {
        std::scoped_lock lock{mtx};
        std::ifstream ifile{sMyGameLibraryFile, std::ios::binary};
        if (ifile.is_open())
        {
            std::streamsize nCharsRead1 = 0;
            std::streamsize nCharsRead2 = 0;
            do
            {
                nCharsRead1 = 0;
                nCharsRead2 = 0;
                std::array<char, 2048> fileBuf{};
                std::array<char, 64> md5Buf{};

                ifile.getline(fileBuf.data(), 2048);
                nCharsRead1 = ifile.gcount();

                if (nCharsRead1 > 0)
                {
                    ifile.getline(md5Buf.data(), 64);
                    nCharsRead2 = ifile.gcount();
                }

                if (fileBuf.front() != '\0' && md5Buf.front() != '\0' && nCharsRead1 > 0 && nCharsRead2 > 0)
                {
                    // Add
                    std::string file{fileBuf.data()};
                    std::string md5{md5Buf.data()};

                    Results.try_emplace(file, md5);
                }

            } while (nCharsRead1 > 0 && nCharsRead2 > 0);
        }
    }
}

void Dlg_GameLibrary::SaveAll()
{
    std::wstring sMyGameLibraryFile = g_sHomeDir + RA_MY_GAME_LIBRARY_FILENAME;

    mtx.lock();
    FILE* pf = nullptr;
    _wfopen_s(&pf, sMyGameLibraryFile.c_str(), L"wb");
    if (pf != nullptr)
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
    mtx.unlock();
}

// static
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

            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            SetDlgItemText(hDlg, IDC_RA_ROMDIR, NativeStr(pConfiguration.GetRomDirectory()).c_str());
            SetDlgItemText(hDlg, IDC_RA_GLIB_NAME, TEXT(""));

            m_GameHashLibrary.clear();
            m_GameTitlesLibrary.clear();
            m_ProgressLibrary.clear();
            ParseGameHashLibraryFromFile(m_GameHashLibrary);
            ParseGameTitlesFromFile(m_GameTitlesLibrary);
            ParseMyProgressFromFile(m_ProgressLibrary);

            // int msBetweenRefresh = 1000;	//	auto?
            // SetTimer( hDlg, 1, msBetweenRefresh, (TIMERPROC)g_GameLibrary.s_GameLibraryProc );
            RefreshList();

            return FALSE;
        }

        case WM_TIMER:
            if ((g_GameLibrary.GetHWND() != nullptr) && (IsWindowVisible(g_GameLibrary.GetHWND())))
                RefreshList();
            // ReloadGameListData();
            return FALSE;

        case WM_NOTIFY:
            switch (LOWORD(wParam))
            {
                case IDC_RA_LBX_GAMELIST:
                {
#pragma warning(push)
#pragma warning(disable: 26490)
                    GSL_SUPPRESS_TYPE1
                    switch (reinterpret_cast<LPNMHDR>(lParam)->code)
#pragma warning(pop)
                    {
                        case LVN_ITEMCHANGED:
                        {
                            // RA_LOG( "Item Changed\n" );
                            HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST);
                            const int nSel = ListView_GetSelectionMark(hList);
                            if (nSel != -1)
                            {
                                TCHAR buffer[1024]{};
                                ListView_GetItemText(hList, nSel, 1, buffer, 1024);
                                SetWindowText(GetDlgItem(hDlg, IDC_RA_GLIB_NAME), buffer);
                            }
                        }
                        break;

                        case NM_CLICK:
                            // RA_LOG( "Click\n" );
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

                    mtx.lock(); //?
                    SetDlgItemText(m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, TEXT("Scanning..."));
                    mtx.unlock();
                    return FALSE;

                case IDC_RA_PICKROMDIR:
                {
                    std::string sROMDirLocation = GetFolderFromDialog();
                    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
                    pConfiguration.SetRomDirectory(sROMDirLocation);
                    RA_LOG("Selected Folder: %s\n", sROMDirLocation.c_str());
                    SetDlgItemText(hDlg, IDC_RA_ROMDIR, NativeStr(sROMDirLocation).c_str());
                    return FALSE;
                }

                case IDC_RA_LBX_GAMELIST:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_GAMELIST);
                    const int nSel = ListView_GetSelectionMark(hList);
                    if (nSel != -1)
                    {
                        TCHAR sGameTitle[1024]{};
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
// void Dlg_GameLibrary::DoModalDialog( HINSTANCE hInst, HWND hParent )
//{
//	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), hParent, Dlg_GameLibrary::s_GameLibraryProc );
//}
