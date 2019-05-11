#include "RA_Dlg_AchievementsReporter.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_httpthread.h"

#include "api\SubmitTicket.hh"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

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
            SetDlgItemText(hDlg, IDC_RA_BROKENACH_BUGREPORTER, NativeStr(ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername()).c_str());
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

                    ra::api::SubmitTicket::Request request;
                    std::string sBuggedIDs;

                    int nReportCount = 0;
                    const int nListSize = ListView_GetItemCount(hList);
                    for (auto i = 0; i < nListSize; ++i)
                    {
                        if (ListView_GetCheckState(hList, i) != 0)
                        {
                            const auto nId = g_pActiveAchievements->GetAchievement(i).ID();
                            request.AchievementIds.insert(nId);
                            sBuggedIDs.append(ra::StringPrintf("%zu,", nId));
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

                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();

                    ra::ui::viewmodels::MessageBoxViewModel vmConfirm;
                    vmConfirm.SetHeader(ra::StringPrintf(L"Are you sure that you want to create %u tickets for %s?",
                        request.AchievementIds.size(), pGameContext.GameTitle()));
                    vmConfirm.SetMessage(ra::StringPrintf(L""
                        "Achievement IDs: %s\n"
                        "Problem: %s\n"
                        "Reporter: %s\n"
                        "Game Checksum: %s\n"
                        "Comment: %s",
                        sBuggedIDs, sProblemTypeNice, pUserContext.GetUsername(), pGameContext.GameHash(),
                        sBugReportComment
                    ));
                    vmConfirm.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

                    if (vmConfirm.ShowModal() != ra::ui::DialogResult::Yes)
                        return FALSE;

                    request.Problem = ra::itoe<ra::api::SubmitTicket::ProblemType>(nProblemType);
                    request.GameHash = pGameContext.GameHash();
                    request.Comment = sBugReportComment;

                    const auto response = request.Call();
                    if (response.Succeeded())
                    {
                        if (response.TicketsCreated == 1)
                        {
                            ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
                            vmMessageBox.SetHeader(L"1 ticket created");
                            vmMessageBox.SetMessage(L"Thank you for reporting the issue. The development team will investigate and you will be notified when the ticket is updated or resolved.");
                            vmMessageBox.ShowModal();
                        }
                        else
                        {
                            ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
                            vmMessageBox.SetHeader(ra::StringPrintf(L"%u tickets created", response.TicketsCreated));
                            vmMessageBox.SetMessage(L"Thank you for reporting the issues. The development team will investigate and you will be notified when the tickets are updated or resolved.");
                            vmMessageBox.ShowModal();
                        }

                        EndDialog(hDlg, TRUE);
                    }
                    else
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to submit tickets",
                            !response.ErrorMessage.empty() ? ra::Widen(response.ErrorMessage) : L"Unknown error");
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
void Dlg_AchievementsReporter::DoModalDialog(HINSTANCE hInst, HWND hParent)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You must load a game before you can report broken achievements.");
    else if (pGameContext.ActiveAchievementType() == AchievementSet::Type::Local)
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report broken local achievements.");
    else if (pGameContext.GetMode() == ra::data::GameContext::Mode::CompatibilityTest)
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report broken achievements in compatibility test mode.");
    else if (g_pActiveAchievements->NumAchievements() == 0)
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"There are no active achievements to report.");
    else
        DialogBox(hInst, MAKEINTRESOURCE(IDD_RA_REPORTBROKENACHIEVEMENTS), hParent, AchievementsReporterProc);
}
