#include "RA_Dlg_AchievementsReporter.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_httpthread.h"
#include "RA_Resource.h"
#include "RA_User.h"

#include "data\GameContext.hh"
namespace {

inline constexpr std::array<LPCTSTR, 5> COL_TITLE{ _T(""), _T("Title"), _T("Description"), _T("Author"), _T("Achieved?") };
inline constexpr std::array<int, 5> COL_SIZE{ 19, 105, 205, 75, 62 };

const char* PROBLEM_STR[] = { "Unknown", "Triggers at wrong time", "Didn't trigger at all" };

}

int Dlg_AchievementsReporter::ms_nNumOccupiedRows = 0;
char Dlg_AchievementsReporter::ms_lbxData[MAX_ACHIEVEMENTS][NumReporterColumns][MAX_TEXT_LEN];

Dlg_AchievementsReporter g_AchievementsReporterDialog;

void Dlg_AchievementsReporter::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0)) {}

    //	Remove all data.
    ListView_DeleteAllItems(hList);

    auto i = 0;
    for (const auto& title : COL_TITLE)
    {
        ra::tstring str = title; // Hold the temporary object
        LV_COLUMN col
        {
            col.mask       = ra::to_unsigned(LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT),
            col.fmt        = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH,
            col.cx         = COL_SIZE.at(i),
            col.pszText    = str.data(),
            col.cchTextMax = 255,
            col.iSubItem   = i
        };

        if (i == ra::to_signed(COL_TITLE.size() - 1)) // If the last element: fill to the end
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, i, reinterpret_cast<LPARAM>(&col));
        i++;
    }

    ms_nNumOccupiedRows = 0;
}

int Dlg_AchievementsReporter::AddAchievementToListBox(HWND hList, const Achievement* pAch)
{
    for (size_t i = 0; i < NumReporterColumns; ++i)
    {
        switch (i)
        {
            case Checked:
                sprintf_s(ms_lbxData[ms_nNumOccupiedRows][i], MAX_TEXT_LEN, "");
                break;
            case Title:
                sprintf_s(ms_lbxData[ms_nNumOccupiedRows][i], MAX_TEXT_LEN, pAch->Title().c_str());
                break;
            case Desc:
                sprintf_s(ms_lbxData[ms_nNumOccupiedRows][i], MAX_TEXT_LEN, pAch->Description().c_str());
                break;
            case Author:
                sprintf_s(ms_lbxData[ms_nNumOccupiedRows][i], MAX_TEXT_LEN, pAch->Author().c_str());
                break;
            case Achieved:
                sprintf_s(ms_lbxData[ms_nNumOccupiedRows][i], MAX_TEXT_LEN, !pAch->Active() ? "Yes" : "No");
                break;
            default:
                ASSERT(!"Unknown col!");
                break;
        }
    }

    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));

    item.mask = LVIF_TEXT;
    item.cchTextMax = 256;
    item.iItem = ms_nNumOccupiedRows;

    for (size_t i = 0; i < NumReporterColumns; ++i)
    {
        item.iSubItem = i;
        ra::tstring sStr = NativeStr(ms_lbxData[ms_nNumOccupiedRows][i]);	//Scoped cache
        item.pszText = sStr.data();

        if (i == 0)
            item.iItem = ListView_InsertItem(hList, &item);
        else
            ListView_SetItem(hList, &item);
    }

    ASSERT(item.iItem == ms_nNumOccupiedRows);

    ms_nNumOccupiedRows++;	//	Last thing to do!
    return item.iItem;
}

INT_PTR CALLBACK Dlg_AchievementsReporter::AchievementsReporterProc(HWND hDlg, UINT uMsg, WPARAM wParam, _UNUSED LPARAM)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hList = GetDlgItem(hDlg, IDC_RA_REPORTBROKENACHIEVEMENTSLIST);
            SetupColumns(hList);

            for (size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i)
                AddAchievementToListBox(hList, &g_pActiveAchievements->GetAchievement(i));

            ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP);
            SetDlgItemText(hDlg, IDC_RA_BROKENACH_BUGREPORTER, NativeStr(RAUsers::LocalUser().Username()).c_str());
        }
        return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_REPORTBROKENACHIEVEMENTSLIST);

                    const bool bProblem1Sel = (IsDlgButtonChecked(hDlg, IDC_RA_PROBLEMTYPE1) == BST_CHECKED);
                    const bool bProblem2Sel = (IsDlgButtonChecked(hDlg, IDC_RA_PROBLEMTYPE2) == BST_CHECKED);

                    if ((bProblem1Sel == false) && (bProblem2Sel == false))
                    {
                        MessageBox(nullptr, TEXT("Please select a problem type."), TEXT("Warning"), MB_ICONWARNING);
                        return FALSE;
                    }

                    int nProblemType = bProblem1Sel ? 1 : bProblem2Sel ? 2 : 0;	// 0==?
                    const char* sProblemTypeNice = PROBLEM_STR[nProblemType];

                    char sBuggedIDs[1024];
                    sprintf_s(sBuggedIDs, 1024, "");

                    int nReportCount = 0;

                    const size_t nListSize = ListView_GetItemCount(hList);
                    for (size_t i = 0; i < nListSize; ++i)
                    {
                        if (ListView_GetCheckState(hList, i) != 0)
                        {
                            //	NASTY big assumption here...
                            char buffer[1024];
                            sprintf_s(buffer, 1024, "%u,", g_pActiveAchievements->GetAchievement(i).ID());
                            strcat_s(sBuggedIDs, 1024, buffer);

                            //ListView_GetItem( hList );	
                            nReportCount++;
                        }
                    }

                    if (nReportCount > 5)
                    {
                        if (MessageBox(nullptr, TEXT("You have over 5 achievements selected. Is this OK?"), TEXT("Warning"), MB_YESNO) == IDNO)
                            return FALSE;
                    }

                    TCHAR sBugReportCommentIn[4096];
                    GetDlgItemText(hDlg, IDC_RA_BROKENACHIEVEMENTREPORTCOMMENT, sBugReportCommentIn, 4096);
                    std::string sBugReportComment = ra::Narrow(sBugReportCommentIn);

                    //	Intentionally MBCS
                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    std::string sBugReportInFull{ "--New Bug Report--\n\nGame: " };
                    sBugReportInFull += ra::Narrow(pGameContext.GameTitle()).c_str();
                    sBugReportInFull += "\nAchievement IDs: ";
                    sBugReportInFull += sBuggedIDs;
                    sBugReportInFull += "\nProblem: ";
                    sBugReportInFull += sProblemTypeNice;
                    sBugReportInFull += "\nReporter: ";
                    sBugReportInFull += RAUsers::LocalUser().Username();
                    sBugReportInFull += "\nROM Checksum: ";
                    sBugReportInFull += pGameContext.GameHash().c_str();
                    sBugReportInFull += "\n\nComment: "; 
                    sBugReportInFull += sBugReportComment;
                    sBugReportInFull += "\n\nIs this OK?";
                        
                        

                    if (MessageBox(nullptr, NativeStr(sBugReportInFull).c_str(), TEXT("Summary"), MB_YESNO) == IDNO)
                        return FALSE;

                    PostArgs args;
                    args['u'] = RAUsers::LocalUser().Username();
                    args['t'] = RAUsers::LocalUser().Token();
                    args['i'] = sBuggedIDs;
                    args['p'] = std::to_string(nProblemType);
                    args['n'] = sBugReportComment.c_str();
                    args['m'] = pGameContext.GameHash();

                    rapidjson::Document doc;
                    if (RAWeb::DoBlockingRequest(RequestSubmitTicket, args, doc))
                    {
                        if (doc["Success"].GetBool())
                        {
                            char buffer[2048];
                            sprintf_s(buffer, 2048, "Submitted OK!\n"
                                "\n"
                                "Thankyou for reporting that bug(s), and sorry it hasn't worked correctly.\n"
                                "\n"
                                "The development team will investigate this bug as soon as possible\n"
                                "and we will send you a message on RetroAchievements.org\n"
                                "as soon as we have a solution.\n"
                                "\n"
                                "Thanks again!");

                            MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Success!"), MB_OK);
                            EndDialog(hDlg, TRUE);
                            return TRUE;
                        }
                        else
                        {
                            char buffer[2048];
                            sprintf_s(buffer, 2048,
                                "Failed!\n"
                                "\n"
                                "Response From Server:\n"
                                "\n"
                                "Error code: %d", doc.GetParseError());
                            MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Error from server!"), MB_OK);
                            return FALSE;
                        }
                    }
                    else
                    {
                        MessageBox(hDlg,
                            TEXT("Failed!\n")
                            TEXT("\n")
                            TEXT("Cannot reach server... are you online?\n")
                            TEXT("\n"),
                            TEXT("Error!"), MB_OK);
                        return FALSE;
                    }
                }
                break;

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
            }
            return FALSE;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;

        default:
            return FALSE;
    }
}

//static
void Dlg_AchievementsReporter::DoModalDialog(HINSTANCE hInst, HWND hParent)
{
    if (g_pActiveAchievements->NumAchievements() == 0)
        MessageBox(hParent, TEXT("No ROM loaded!"), TEXT("Error"), MB_OK);
    else
        DialogBox(hInst, MAKEINTRESOURCE(IDD_RA_REPORTBROKENACHIEVEMENTS), hParent, AchievementsReporterProc);
}
