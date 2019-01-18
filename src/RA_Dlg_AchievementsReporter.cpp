#include "RA_Dlg_AchievementsReporter.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "data\GameContext.hh"

inline constexpr std::array<const char*, 3> PROBLEM_STR{"Unknown", "Triggers at wrong time", "Didn't trigger at all"};
gsl::index Dlg_AchievementsReporter::ms_nNumOccupiedRows = 0;
char Dlg_AchievementsReporter::ms_lbxData[MAX_ACHIEVEMENTS][Dlg_AchievementsReporter::COL_SIZE.size()][MAX_TEXT_LEN]{};

Dlg_AchievementsReporter g_AchievementsReporterDialog;

void Dlg_AchievementsReporter::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0))
    {
    }

    //	Remove all data.
    ListView_DeleteAllItems(hList);

    auto i = 0U;
    for (const auto& sTitle : COL_TITLE)
    {
        ra::tstring str{sTitle};
        LV_COLUMN col{col.mask = ra::to_unsigned(LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT),
                      col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH,
                      col.cx = COL_SIZE.at(i),
                      col.pszText = str.data(),
                      col.cchTextMax = 255,
                      col.iSubItem = ra::to_signed(i)};

        if (i == (COL_TITLE.size() - 1)) // If the last element: fill to the end
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, i, &col);
        i++;
    }

    ms_nNumOccupiedRows = 0;
}

_Use_decl_annotations_
void Dlg_AchievementsReporter::AddAchievementToListBox(HWND hList, const Achievement* restrict pAch)
{
    Expects(pAch != nullptr);
    // We aren't actually using the value so we're using iterators
    for (auto it = COL_TITLE.cbegin(); it != COL_TITLE.cend(); ++it)
    {
        const auto nPos{std::distance(COL_TITLE.cbegin(), it)};
        switch (ra::itoe<Column>(nPos))
        {
            case Column::Checked:
                sprintf_s(LbxDataAt(ms_nNumOccupiedRows, nPos), MAX_TEXT_LEN, "%s", "");
                break;
            case Column::Title:
                sprintf_s(LbxDataAt(ms_nNumOccupiedRows, nPos), MAX_TEXT_LEN, "%s", pAch->Title().c_str());
                break;
            case Column::Desc:
                sprintf_s(LbxDataAt(ms_nNumOccupiedRows, nPos), MAX_TEXT_LEN, "%s", pAch->Description().c_str());
                break;
            case Column::Author:
                sprintf_s(LbxDataAt(ms_nNumOccupiedRows, nPos), MAX_TEXT_LEN, "%s", pAch->Author().c_str());
                break;
            case Column::Achieved:
                sprintf_s(LbxDataAt(ms_nNumOccupiedRows, nPos), MAX_TEXT_LEN, "%s", !pAch->Active() ? "Yes" : "No");
                break;
            default:
                ASSERT(!"Unknown col!");
        }
    }

    for (auto it = COL_TITLE.cbegin(); it != COL_TITLE.cend(); ++it)
    {
        // difference_type could be 8 bytes.
        const auto nPos{gsl::narrow<int>(std::distance(COL_TITLE.cbegin(), it))};
        ra::tstring sStr{NativeStr(LbxDataAt(ms_nNumOccupiedRows, nPos))}; // Scoped cache
        LV_ITEM item{};
        item.mask = ra::to_unsigned(LVIF_TEXT);
        item.iItem = ms_nNumOccupiedRows;
        item.iSubItem = nPos;
        item.pszText = sStr.data();
        item.cchTextMax = 256;

        if (nPos == 0)
            item.iItem = ListView_InsertItem(hList, &item);
        else
            ListView_SetItem(hList, &item);
        Ensures(item.iItem == ms_nNumOccupiedRows);
    }

    ms_nNumOccupiedRows++; // Last thing to do!
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
            {
                AddAchievementToListBox(hList, &g_pActiveAchievements->GetAchievement(i));
            }

            ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP);
            SetDlgItemText(hDlg, IDC_RA_BROKENACH_BUGREPORTER, NativeStr(RAUsers::LocalUser().Username()).c_str());
        }
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    const auto hList{::GetDlgItem(hDlg, IDC_RA_REPORTBROKENACHIEVEMENTSLIST)};

                    const auto bProblem1Sel{Button_GetCheck(GetDlgItem(hDlg, IDC_RA_PROBLEMTYPE1))};
                    const auto bProblem2Sel{Button_GetCheck(GetDlgItem(hDlg, IDC_RA_PROBLEMTYPE2))};

                    if ((bProblem1Sel == false) && (bProblem2Sel == false))
                    {
                        MessageBox(nullptr, TEXT("Please select a problem type."), TEXT("Warning"), MB_ICONWARNING);
                        return FALSE;
                    }

                    std::string sBuggedIDs;

                    int nReportCount = 0;
                    const int nListSize = ListView_GetItemCount(hList);
                    for (auto i = 0; i < nListSize; ++i)
                    {
                        if (ListView_GetCheckState(hList, i) != 0)
                        {
                            sBuggedIDs.append(ra::StringPrintf("%zu,", g_pActiveAchievements->GetAchievement(i).ID()));
                            nReportCount++;
                        }
                    }
                    sBuggedIDs.pop_back(); // gets rid of extra comma
                    if (nReportCount > 5)
                    {
                        if (MessageBox(nullptr, TEXT("You have over 5 achievements selected. Is this OK?"),
                                       TEXT("Warning"), MB_YESNO) == IDNO)
                            return FALSE;
                    }

                    TCHAR sBugReportCommentIn[4096];
                    GetDlgItemText(hDlg, IDC_RA_BROKENACHIEVEMENTREPORTCOMMENT, sBugReportCommentIn, 4096);
                    std::string sBugReportComment = ra::Narrow(sBugReportCommentIn);

                    const auto nProblemType = bProblem1Sel ? 1 : bProblem2Sel ? 2U : 0u; // 0==?
                    const auto sProblemTypeNice = PROBLEM_STR.at(nProblemType);

                    //	Intentionally MBCS
                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    const auto sBugReportInFull = ra::StringPrintf(
                        "--New Bug Report--\n\nGame: %s\n"
                        "Achievement IDs: %s\n"
                        "Problem: %s\n"
                        "Reporter: %s\n"
                        "ROM Checksum: %s\n\n"
                        "Comment: %s\n\n"
                        "Is this OK?",
                        ra::Narrow(pGameContext.GameTitle()).c_str(), sBuggedIDs.c_str(), sProblemTypeNice,
                        RAUsers::LocalUser().Username().c_str(), pGameContext.GameHash().c_str(),
                        sBugReportComment.c_str());

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
                            sprintf_s(buffer, 2048,
                                      "Submitted OK!\n"
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
                                      "Error code: %d",
                                      doc.GetParseError());
                            MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Error from server!"), MB_OK);
                            return FALSE;
                        }
                    }
                    else
                    {
                        MessageBox(hDlg,
                                   TEXT("Failed!\n") TEXT("\n") TEXT("Cannot reach server... are you online?\n")
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

// static
void Dlg_AchievementsReporter::DoModalDialog(HINSTANCE hInst, HWND hParent) noexcept
{
    if (g_pActiveAchievements->NumAchievements() == 0)
        MessageBox(hParent, TEXT("No ROM loaded!"), TEXT("Error"), MB_OK);
    else
        DialogBox(hInst, MAKEINTRESOURCE(IDD_RA_REPORTBROKENACHIEVEMENTS), hParent, AchievementsReporterProc);
}
