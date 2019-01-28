#include "RA_Dlg_GameTitle.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "api\FetchGamesList.hh"
#include "api\SubmitNewTitle.hh"

#include "data\UserContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

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
            int nSel = ComboBox_AddString(hKnownGamesCbo, NativeStr("<New Title>").c_str());
            ComboBox_SetCurSel(hKnownGamesCbo, nSel);

            //	***Do blocking fetch of all game titles.***
            ra::api::FetchGamesList::Request request;
            request.ConsoleId = g_ConsoleID;

            auto response = request.Call();
            if (response.Failed())
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::Widen(response.ErrorMessage));
            }
            else
            {
                for (const auto& pGame : response.Games)
                {
                    auto sTitle = NativeStr(pGame.Name);
                    nSel = ComboBox_AddString(hKnownGamesCbo, sTitle.c_str());

                    //	Attempt to find this game and select it by default: case insensitive comparison
                    if (sGameTitleTidy == sTitle)
                        ComboBox_SetCurSel(hKnownGamesCbo, nSel);
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

                    ra::api::SubmitNewTitle::Request request;
                    request.ConsoleId = g_ConsoleID;
                    request.Hash = m_sMD5;
                    request.GameName = ra::Widen(sSelectedTitle);

                    auto response = request.Call();
                    if (response.Succeeded())
                    {
                        g_GameTitleDialog.m_nReturnedGameID = response.GameId;

                        //	Close this dialog
                        EndDialog(hDlg, TRUE);
                        return TRUE;
                    }
                    else
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not add new title", ra::Widen(response.ErrorMessage));
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
