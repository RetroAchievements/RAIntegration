#include "RA_Dlg_GameTitle.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "data\UserContext.hh"

Dlg_GameTitle g_GameTitleDialog;

INT_PTR CALLBACK s_GameTitleProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_GameTitleDialog.GameTitleProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR Dlg_GameTitle::GameTitleProc(HWND hDlg, UINT uMsg, WPARAM wParam, _UNUSED LPARAM)
{
    static bool bUpdatingTextboxTitle = false;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            const HWND hKnownGamesCbo = GetDlgItem(hDlg, IDC_RA_KNOWNGAMES);
            std::string sGameTitleTidy = Dlg_GameTitle::CleanRomName(g_GameTitleDialog.m_sEstimatedGameTitle);
            SetDlgItemText(hDlg, IDC_RA_GAMETITLE, NativeStr(sGameTitleTidy).c_str());

            //	Load in the checksum
            SetDlgItemText(hDlg, IDC_RA_CHECKSUM, NativeStr(g_GameTitleDialog.m_sMD5).c_str());

            //	Populate the dropdown
            //	***Do blocking fetch of all game titles.***
            int nSel = ComboBox_AddString(hKnownGamesCbo, NativeStr("<New Title>").c_str());
            ComboBox_SetCurSel(hKnownGamesCbo, nSel);

            PostArgs args;
            args['c'] = std::to_string(g_ConsoleID);

            rapidjson::Document doc;
            if (RAWeb::DoBlockingRequest(RequestGamesList, args, doc))
            {
                const rapidjson::Value& Data = doc["Response"];

                //	For all data responses to this request, populate our m_aGameTitles map
                {
                    rapidjson::Value::ConstMemberIterator iter = Data.MemberBegin();
                    while (iter != Data.MemberEnd())
                    {
                        if (iter->name.IsNull() || iter->value.IsNull())
                        {
                            iter++;
                            continue;
                        }

                        // Keys cannot be anything but strings
                        const auto nGameID = std::strtoul(iter->name.GetString(), nullptr, 10); 
                        const std::string& sTitle = iter->value.GetString();
                        m_aGameTitles[sTitle] = nGameID;

                        iter++;
                    }
                }

                {
                    std::map<std::string, unsigned int>::const_iterator iter = m_aGameTitles.begin();
                    while (iter != m_aGameTitles.end())
                    {
                        const std::string& sTitle = iter->first;

                        nSel = ComboBox_AddString(hKnownGamesCbo, NativeStr(sTitle).c_str());

                        //	Attempt to find this game and select it by default: case insensitive comparison
                        if (sGameTitleTidy.compare(sTitle) == 0)
                        {
                            ComboBox_SetCurSel(hKnownGamesCbo, nSel);
                        }

                        iter++;
                    }
                }
            }

            return TRUE;
        }
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    //	Fetch input data:
                    TCHAR sSelectedTitleBuffer[512];
                    ComboBox_GetText(GetDlgItem(hDlg, IDC_RA_KNOWNGAMES), sSelectedTitleBuffer, 512);
                    ra::tstring sSelectedTitle = sSelectedTitleBuffer;

                    unsigned int nGameID = 0U;
                    if (sSelectedTitle == _T("<New Title>"))
                    {
                        //	Add a new title!
                        GetDlgItemText(hDlg, IDC_RA_GAMETITLE, sSelectedTitleBuffer, 512);
                        sSelectedTitle = sSelectedTitleBuffer;
                    }
                    else
                    {
                        //	Existing title
                        ASSERT(m_aGameTitles.find(ra::Narrow(sSelectedTitle)) != m_aGameTitles.end());
                        nGameID = m_aGameTitles[std::string(ra::Narrow(sSelectedTitle))];
                    }

                    PostArgs args;
                    args['u'] = RAUsers::LocalUser().Username();
                    args['t'] = RAUsers::LocalUser().Token();
                    args['m'] = m_sMD5;
                    args['i'] = ra::Narrow(sSelectedTitle);
                    args['c'] = std::to_string(g_ConsoleID);

                    rapidjson::Document doc;
                    if (RAWeb::DoBlockingRequest(RequestSubmitNewTitle, args, doc) && 
                        doc.HasMember("Success") && doc["Success"].GetBool())
                    {
                        const rapidjson::Value& Response = doc["Response"];

                        nGameID = Response["GameID"].GetUint();

                        g_GameTitleDialog.m_nReturnedGameID = nGameID;

                        //	Close this dialog
                        EndDialog(hDlg, TRUE);
                        return TRUE;
                    }
                    else
                    {
                        if (!doc.HasParseError() && doc.HasMember("Error"))
                        {
                            // Error given
                            MessageBox(hDlg,
                                       NativeStr(std::string("Could not add new title: ") +
                                                 std::string(doc["Error"].GetString())).c_str(),
                                       TEXT("Errors encountered"),
                                       MB_OK);
                        }
                        else
                        {
                            MessageBox(hDlg, TEXT("Cannot contact server!"), TEXT("Error in connection"), MB_OK);
                        }

                        return TRUE;
                    }
                }

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_GAMETITLE:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            if (!bUpdatingTextboxTitle) // Note: use barrier to prevent automatic switching off.
                            {
                                //	If the user has started to enter a value, set the upper combo to 'new entry'
                                HWND hKnownGamesCbo = GetDlgItem(hDlg, IDC_RA_KNOWNGAMES);
                                ComboBox_SetCurSel(hKnownGamesCbo, 0);
                            }
                            break;
                    }
                    break;

                case IDC_RA_KNOWNGAMES:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            //	If the user has selected a value, copy this text to the bottom textbox.
                            bUpdatingTextboxTitle = TRUE;

                            TCHAR sSelectedTitle[512];
                            GetDlgItemText(hDlg, IDC_RA_KNOWNGAMES, sSelectedTitle, 512);
                            SetDlgItemText(hDlg, IDC_RA_GAMETITLE, sSelectedTitle);

                            bUpdatingTextboxTitle = FALSE;
                        }
                        break;
                    }
                    break;
            }
            break;

        case WM_KEYDOWN:
            break;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;
    }

    return FALSE;
}

void Dlg_GameTitle::DoModalDialog(HINSTANCE hInst,
                                  HWND hParent,
                                  std::string& sMD5InOut,
                                  std::string& sEstimatedGameTitleInOut,
                                  unsigned int& nGameIDOut)
{
    if (sMD5InOut.length() == 0)
        return;

    if (!ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
        return;

    g_GameTitleDialog.m_sMD5 = sMD5InOut;
    g_GameTitleDialog.m_sEstimatedGameTitle = sEstimatedGameTitleInOut;
    g_GameTitleDialog.m_nReturnedGameID = 0;

    DialogBox(hInst, MAKEINTRESOURCE(IDD_RA_GAMETITLESEL), hParent, s_GameTitleProc);

    sMD5InOut = g_GameTitleDialog.m_sMD5;
    sEstimatedGameTitleInOut = g_GameTitleDialog.m_sEstimatedGameTitle;
    nGameIDOut = g_GameTitleDialog.m_nReturnedGameID;
}

//	static
std::string Dlg_GameTitle::CleanRomName(const std::string& sTryName)
{
    if (sTryName.empty())
        return "";

    //	Scan through, reform sRomNameRef using all logical characters
    std::string ret;
    for (auto& c : sTryName)
    {
        if (__isascii(c))
            ret.push_back(c);
    }
    return ret;
}
