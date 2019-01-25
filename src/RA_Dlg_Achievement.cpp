#include "RA_Dlg_Achievement.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_md5factory.h"

#include "data\GameContext.hh"

#include "services\AchievementRuntime.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

inline constexpr std::array<LPCTSTR, 6> COLUMN_TITLES_CORE{_T("ID"),     _T("Title"),     _T("Points"),
                                                           _T("Author"), _T("Achieved?"), _T("Modified?")};
inline constexpr std::array<LPCTSTR, 6> COLUMN_TITLES_UNOFFICIAL{_T("ID"),     _T("Title"),  _T("Points"),
                                                                 _T("Author"), _T("Active"), _T("Votes")};
inline constexpr std::array<LPCTSTR, 6> COLUMN_TITLES_LOCAL{_T("ID"),     _T("Title"),  _T("Points"),
                                                            _T("Author"), _T("Active"), _T("Votes")};

inline constexpr std::array<int, 6> COLUMN_SIZE{45, 200, 45, 80, 65, 65};
inline constexpr auto NUM_COLS = ra::to_signed(COLUMN_SIZE.size());

auto iSelect = -1;

Dlg_Achievements g_AchievementsDialog;

void Dlg_Achievements::SetupColumns(HWND hList)
{
    //  Remove all columns and data.
    while (ListView_DeleteColumn(hList, 0) == TRUE)
    {
    }
    ListView_DeleteAllItems(hList);

    auto i = 0;
    for (const auto& col_size : COLUMN_SIZE)
    {
        LPCTSTR sColTitle{_T("")};
        if (g_nActiveAchievementSet == AchievementSet::Type::Core)
            sColTitle = COLUMN_TITLES_CORE.at(i);
        else if (g_nActiveAchievementSet == AchievementSet::Type::Unofficial)
            sColTitle = COLUMN_TITLES_UNOFFICIAL.at(i);
        else if (g_nActiveAchievementSet == AchievementSet::Type::Local)
            sColTitle = COLUMN_TITLES_LOCAL.at(i);

        ra::tstring sColTitleStr = sColTitle; // Take a copy
        LV_COLUMN newColumn{newColumn.mask = ra::to_unsigned(LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT),
                            newColumn.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH,
                            newColumn.cx = col_size,
                            newColumn.pszText = sColTitleStr.data(),
                            newColumn.cchTextMax = 255,
                            newColumn.iSubItem = i};

        if (i == (::NUM_COLS - 1))
            newColumn.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, i, &newColumn);
        i++;
    }

    m_lbxData.clear();
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
}

LRESULT ProcessCustomDraw(LPARAM lParam)
{
#pragma warning(push)
#pragma warning(disable: 26490)
    GSL_SUPPRESS_TYPE1 LPNMLVCUSTOMDRAW lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
#pragma warning(pop)
    switch (lplvcd->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            //  Before the paint cycle begins
            //  request notifications for individual listview items
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT: // Before an item is drawn
        {
            const auto nNextItem = ra::to_signed(lplvcd->nmcd.dwItemSpec);

            if (ra::to_unsigned(nNextItem) < g_pActiveAchievements->NumAchievements())
            {
                // if (((int)lplvcd->nmcd.dwItemSpec%2)==0)
                const BOOL bSelected =
                    &g_pActiveAchievements->GetAchievement(nNextItem) == g_AchievementEditorDialog.ActiveAchievement();
                const BOOL bModified = g_pActiveAchievements->GetAchievement(nNextItem).Modified();

                lplvcd->clrText = bModified ? RGB(255, 0, 0) : RGB(0, 0, 0);
                lplvcd->clrTextBk = bSelected ? RGB(222, 222, 222) : RGB(255, 255, 255);
            }
            return CDRF_NEWFONT;
        }
        break;

            // Before a subitem is drawn
            //  case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
            //      {
            //          if (iSelect == (int)lplvcd->nmcd.dwItemSpec)
            //          {
            //              if (0 == lplvcd->iSubItem)
            //              {
            //                  //customize subitem appearance for column 0
            //                  lplvcd->clrText   = RGB(255,128,0);
            //                  lplvcd->clrTextBk = RGB(255,255,255);
            //
            //                  //To set a custom font:
            //                  //SelectObject(lplvcd->nmcd.hdc,
            //                  //    <your custom HFONT>);
            //
            //                  return CDRF_NEWFONT;
            //              }
            //              else if (1 == lplvcd->iSubItem)
            //              {
            //                  //customize subitem appearance for columns 1..n
            //                  //Note: setting for column i
            //                  //carries over to columnn i+1 unless
            //                  //      it is explicitly reset
            //                  lplvcd->clrTextBk = RGB(255,0,0);
            //                  lplvcd->clrTextBk = RGB(255,255,255);
            //
            //                  return CDRF_NEWFONT;
            //              }
            //          }
            //      }
    }
    return CDRF_DODEFAULT;
}

void Dlg_Achievements::RemoveAchievement(HWND hList, int nIter)
{
    ASSERT(nIter < ListView_GetItemCount(hList));
    ListView_DeleteItem(hList, nIter);
    m_lbxData.erase(m_lbxData.begin() + nIter);

    char buffer[16];
    sprintf_s(buffer, 16, " %u", g_pActiveAchievements->NumAchievements());
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                   NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

    UpdateSelectedAchievementButtons(nullptr);

    g_AchievementEditorDialog.LoadAchievement(nullptr, FALSE);
}

size_t Dlg_Achievements::AddAchievement(HWND hList, const Achievement& Ach)
{
    AddAchievementRow(Ach);

    LV_ITEM item{};
    item.mask = ra::to_unsigned(LVIF_TEXT);
    item.iItem = ra::to_signed(m_lbxData.size());
    item.cchTextMax = 256;

    for (item.iSubItem = 0; item.iSubItem < NUM_COLS; ++item.iSubItem)
    {
        // Cache this (stack) to ensure it lives until after ListView_*Item
        // SD: Is this necessary?
        ra::tstring sTextData{NativeStr(m_lbxData.back().at(item.iSubItem))};
        item.pszText = sTextData.data();

        if (item.iSubItem == 0)
            item.iItem = ListView_InsertItem(hList, &item);
        else
            ListView_SetItem(hList, &item);
    }

    ASSERT(item.iItem == (ra::to_signed(m_lbxData.size()) - 1));
    return ra::to_unsigned(item.iItem);
}

void Dlg_Achievements::AddAchievementRow(const Achievement& Ach)
{
    AchievementDlgRow newRow;

    // Add to our local array:
    newRow.emplace_back((Ach.Category() == ra::etoi(AchievementSet::Type::Local)) ? "0" : std::to_string(Ach.ID()));
    newRow.emplace_back(Ach.Title());
    newRow.emplace_back(std::to_string(Ach.Points()));
    newRow.emplace_back(Ach.Author());

    if (g_nActiveAchievementSet == AchievementSet::Type::Core)
    {
        newRow.emplace_back(!Ach.Active() ? "Yes" : "No");
        newRow.emplace_back(Ach.Modified() ? "Yes" : "No");
    }
    else
    {
        newRow.emplace_back(Ach.Active() ? "Yes" : "No");
        newRow.emplace_back("N/A");
    }
    static_assert(std::is_nothrow_move_constructible_v<AchievementDlgRow>);
    m_lbxData.emplace_back(std::move_if_noexcept(newRow));
}

_Success_(return ) _NODISCARD BOOL
    LocalValidateAchievementsBeforeCommit(_In_reads_(1) const std::array<int, 1> nLbxItems)
{
    char buffer[2048];
    for (auto& nIter : nLbxItems)
    {
        const Achievement& Ach = g_pActiveAchievements->GetAchievement(nIter);
        if (Ach.Title().length() < 2)
        {
            sprintf_s(buffer, 2048, "Achievement title too short:\n%s\nMust be greater than 2 characters.",
                      Ach.Title().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }
        if (Ach.Title().length() > 63)
        {
            sprintf_s(buffer, 2048, "Achievement title too long:\n%s\nMust be fewer than 80 characters.",
                      Ach.Title().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }

        if (Ach.Description().length() < 2)
        {
            sprintf_s(buffer, 2048, "Achievement description too short:\n%s\nMust be greater than 2 characters.",
                      Ach.Description().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }
        if (Ach.Description().length() > 255)
        {
            sprintf_s(buffer, 2048, "Achievement description too long:\n%s\nMust be fewer than 255 characters.",
                      Ach.Description().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }
    }

    return TRUE;
}

// static
INT_PTR CALLBACK Dlg_Achievements::s_AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //  TBD: intercept any msgs?
    return g_AchievementsDialog.AchievementsProc(hDlg, nMsg, wParam, lParam);
}

BOOL AttemptUploadAchievementBlocking(const Achievement& Ach, unsigned int nFlags, rapidjson::Document& doc)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const std::string sMem = Ach.CreateMemString();

    const unsigned int nId = Ach.Category() == ra::etoi(AchievementSet::Type::Local) ? 0 : Ach.ID();

    //  Deal with secret:
    char sPostCode[2048];
    sprintf_s(sPostCode, "%sSECRET%uSEC%s%uRE2%u", RAUsers::LocalUser().Username().c_str(), nId, sMem.c_str(),
              Ach.Points(), Ach.Points() * 3);

    std::string sPostCodeHash = RAGenerateMD5(std::string(sPostCode));

    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['a'] = std::to_string(nId);
    args['g'] = std::to_string(pGameContext.GameId());
    args['n'] = Ach.Title();
    args['d'] = Ach.Description();
    args['m'] = sMem;
    args['z'] = std::to_string(Ach.Points());
    args['f'] = std::to_string(nFlags);
    args['b'] = Ach.BadgeImageURI();
    args['h'] = sPostCodeHash;

    return (RAWeb::DoBlockingRequest(RequestSubmitAchievementData, args, doc));
}

_Use_decl_annotations_ void Dlg_Achievements::OnClickAchievementSet(AchievementSet::Type nAchievementSet)
{
    RASetAchievementCollection(nAchievementSet);

    if (nAchievementSet == AchievementSet::Type::Core)
    {
        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Demote from Core"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Commit Selected"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Server"));

        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE); // Cannot add direct to Core

        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);

        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, TRUE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, FALSE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, FALSE);
    }
    else if (nAchievementSet == AchievementSet::Type::Unofficial)
    {
        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Promote to Core"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Commit Selected"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Server"));

        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE); // Cannot add direct to Unofficial

        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);

        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, FALSE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, TRUE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, FALSE);
    }
    else if (nAchievementSet == AchievementSet::Type::Local)
    {
        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Promote to Unofficial"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Save All Local"));
        SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Disk"));

        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), TRUE); // Can add to Local

        ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);

        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, FALSE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, FALSE);
        CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, TRUE);
    }

    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                   NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    CheckDlgButton(m_hAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, !pRuntime.IsPaused());

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    OnLoad_NewRom(pGameContext.GameId()); // assert: calls UpdateSelectedAchievementButtons

    g_AchievementEditorDialog.OnLoad_NewRom();
}

INT_PTR Dlg_Achievements::AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            m_hAchievementsDlg = hDlg;

            SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, 0U, 0L);
            SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, 0U, 0L);
            SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, 0U, 0L);

            switch (g_nActiveAchievementSet)
            {
                case AchievementSet::Type::Core:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, 1U, 0L);
                    break;
                case AchievementSet::Type::Unofficial:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, 1U, 0L);
                    break;
                case AchievementSet::Type::Local:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, 1U, 0L);
                    break;
                default:
                    ASSERT(!"Unknown achievement set!");
                    break;
            }

            //  Continue as if a new rom had been loaded
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
            OnLoad_NewRom(pGameContext.GameId());

            const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
            CheckDlgButton(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE, !pRuntime.IsPaused());

            //  Click the core
            OnClickAchievementSet(AchievementSet::Type::Core);

            RestoreWindowPosition(hDlg, "Achievements", false, true);
        }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RA_ACTIVE_CORE:
                    OnClickAchievementSet(AchievementSet::Type::Core);
                    return TRUE;

                case IDC_RA_ACTIVE_UNOFFICIAL:
                    OnClickAchievementSet(AchievementSet::Type::Unofficial);
                    return TRUE;

                case IDC_RA_ACTIVE_LOCAL:
                    OnClickAchievementSet(AchievementSet::Type::Local);
                    return TRUE;

                case IDCLOSE:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_PROMOTE_ACH:
                    //  Replace with background upload?
                    if (!RA_GameIsActive())
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                    }
                    else
                    {
                        if (g_nActiveAchievementSet == AchievementSet::Type::Local)
                        {
                            // Promote from Local to Unofficial is just a commit to Unofficial
                            CommitAchievements(hDlg);
                        }
                        else if (g_nActiveAchievementSet == AchievementSet::Type::Unofficial)
                        {
                            HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                            const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSel == -1)
                                return FALSE;

                            //  Promote to Core

                            //  Note: specify that this is a one-way operation
                            if (MessageBox(
                                    hDlg,
                                    TEXT("Promote this achievement to the Core Achievement set.\n\n")
                                        TEXT("Please note this is a one-way operation, and will allow players\n")
                                            TEXT("to officially obtain this achievement and the points for it.\n"),
                                    // TEXT("Note: all players who have achieved it while it has been unofficial\n")
                                    // TEXT("will have to earn this again now it is in the core group.\n"),
                                    TEXT("Are you sure?"), MB_YESNO | MB_ICONWARNING) == IDYES)
                            {
                                Achievement& selectedAch = g_pActiveAchievements->GetAchievement(nSel);

                                unsigned int nFlags = 1 << 0; //  Active achievements! : 1
                                if (g_nActiveAchievementSet == AchievementSet::Type::Unofficial)
                                    nFlags |= 1 << 1; //  Official achievements: 3

                                rapidjson::Document response;
                                if (AttemptUploadAchievementBlocking(selectedAch, nFlags, response))
                                {
                                    if (response["Success"].GetBool())
                                    {
                                        const unsigned int nID = response["AchievementID"].GetUint();

                                        //  Remove the achievement from the local/user achievement set,
                                        //   add it to the unofficial set.
                                        g_pUnofficialAchievements->RemoveAchievement(&selectedAch);
                                        g_pCoreAchievements->AddAchievement(&selectedAch);
                                        selectedAch.SetCategory(ra::etoi(AchievementSet::Type::Core));
                                        RemoveAchievement(hList, nSel);

                                        selectedAch.SetActive(TRUE); //  Disable it: all promoted ach must be reachieved

                                        // CoreAchievements->Save();
                                        // UnofficialAchievements->Save();

                                        MessageBox(hDlg, TEXT("Successfully uploaded achievement!"), TEXT("Success!"),
                                                   MB_OK);
                                    }
                                    else
                                    {
                                        MessageBox(hDlg,
                                                   NativeStr(std::string("Error in upload: response from server:") +
                                                             std::string(response["Error"].GetString()))
                                                       .c_str(),
                                                   TEXT("Error in upload!"), MB_OK);
                                    }
                                }
                                else
                                {
                                    MessageBox(hDlg, TEXT("Error connecting to server... are you online?"),
                                               TEXT("Error in upload!"), MB_OK);
                                }
                            }
                        }
                        //                  else if( g_nActiveAchievementSet == Core )
                        //                  {
                        //                      //  Demote from Core
                        //
                        //                  }
                    }

                    break;
                case IDC_RA_DOWNLOAD_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    if (g_nActiveAchievementSet == AchievementSet::Type::Local)
                    {
                        if (MessageBox(hDlg,
                                       TEXT("Are you sure that you want to reload achievements from disk?\n")
                                           TEXT("This will overwrite any changes that you have made.\n"),
                                       TEXT("Refresh from Disk"),
                                       MB_YESNO | MB_ICONWARNING) == IDYES)
                        {
                            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                            const auto nGameID = pGameContext.GameId();
                            if (nGameID != 0)
                            {
                                pGameContext.ReloadAchievements(ra::etoi(AchievementSet::Type::Local));

                                //  Refresh dialog contents:
                                OnLoad_NewRom(nGameID);

                                //  Cause it to reload!
                                OnClickAchievementSet(g_nActiveAchievementSet);
                            }
                        }
                    }
                    else
                    {
                        std::ostringstream oss;
                        oss << "Are you sure that you want to download fresh achievements from " << _RA_HostName()
                            << "?\n"
                            << "This will overwrite any changes that you have made with fresh achievements from the "
                               "server";
                        if (MessageBox(hDlg, NativeStr(oss.str()).c_str(), TEXT("Refresh from Server"),
                                       MB_YESNO | MB_ICONWARNING) == IDYES)
                        {
                            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                            const auto nGameID = pGameContext.GameId();
                            if (nGameID != 0)
                            {
                                pGameContext.LoadGame(nGameID);

                                //  Refresh dialog contents:
                                OnLoad_NewRom(nGameID);

                                //  Cause it to reload!
                                OnClickAchievementSet(g_nActiveAchievementSet);
                            }
                        }
                    }
                }
                break;

                case IDC_RA_ADD_ACH:
                {
                    if (!RA_GameIsActive())
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                        break;
                    }

                    //  Add a new achievement with default params
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    Achievement& Cheevo = pGameContext.NewAchievement(AchievementSet::Type::Local);
                    Cheevo.SetAuthor(RAUsers::LocalUser().Username());
                    Cheevo.SetBadgeImage("00000");

                    //  Reverse find where I am in the list:
                    unsigned int nOffset = 0;
                    for (; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
                    {
                        if (&Cheevo == &g_pActiveAchievements->GetAchievement(nOffset))
                            break;
                    }
                    ASSERT(nOffset < g_pActiveAchievements->NumAchievements());
                    if (nOffset < g_pActiveAchievements->NumAchievements())
                        OnEditData(nOffset, Column::Author, Cheevo.Author());

                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nNewID = AddAchievement(hList, Cheevo);
                    ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                    ListView_EnsureVisible(hList, nNewID, FALSE);

                    char buffer[16];
                    sprintf_s(buffer, 16, " %u", g_pActiveAchievements->NumAchievements());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());

                    Cheevo.SetModified(TRUE);
                    UpdateSelectedAchievementButtons(&Cheevo);
                }
                break;

                case IDC_RA_CLONE_ACH:
                {
                    if (!RA_GameIsActive())
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                        break;
                    }

                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel == -1)
                        return FALSE;

                    //  switch to LocalAchievements
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    Achievement& NewClone = pGameContext.NewAchievement(AchievementSet::Type::Local);

                    //  Clone TO the user achievements
                    const Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);

                    NewClone.CopyFrom(Ach);
                    NewClone.SetAuthor(RAUsers::LocalUser().Username());
                    NewClone.SetTitle(ra::StringPrintf("%s (copy)", Ach.Title()));

                    OnClickAchievementSet(AchievementSet::Type::Local);

                    ListView_SetItemState(hList, g_pLocalAchievements->NumAchievements() - 1,
                                          LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                    ListView_EnsureVisible(hList, g_pLocalAchievements->NumAchievements() - 1, FALSE);

                    char buffer2[16];
                    sprintf_s(buffer2, 16, " %u", g_pActiveAchievements->NumAchievements());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer2).c_str());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                                   NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

                    NewClone.SetModified(TRUE);
                    UpdateSelectedAchievementButtons(&NewClone);
                }

                break;
                case IDC_RA_DEL_ACH:
                {
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    if (pGameContext.GameId() == 0U)
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                        break;
                    }

                    //  Attempt to remove from list, but if it has an Id > 0,
                    //   attempt to remove from DB
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        const Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);
                        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
                        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);

                        if (Ach.Category() == ra::etoi(AchievementSet::Type::Local))
                        {
                            vmMessageBox.SetMessage(L"Are you sure that you want to delete this achievement?");
                        }
                        else
                        {
                            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Deleting achievements from the server is not supported.");
                            break;

                            //vmMessageBox.SetHeader(L"Are you sure that you want to delete this achievement?");
                            //vmMessageBox.SetMessage(ra::StringPrintf(L"This achievement exists on %s. Removing it will affect other players.", _RA_HostName()));
                        }
                        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

                        if (vmMessageBox.ShowModal() == ra::ui::DialogResult::Yes)
                        {
                            pGameContext.RemoveAchievement(Ach.ID());
                            RemoveAchievement(hList, nSel);
                        }
                    }
                }
                break;

                case IDC_RA_COMMIT_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    if (pGameContext.ActiveAchievementType() == AchievementSet::Type::Local)
                    {
                        // Local save is to disk
                        if (pGameContext.SaveLocal())
                        {
                            ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Saved OK!");

                            for (unsigned int i = 0; i < g_pActiveAchievements->NumAchievements(); i++)
                                g_pActiveAchievements->GetAchievement(i).SetModified(FALSE);

                            InvalidateRect(hDlg, nullptr, FALSE);
                        }
                        else
                        {
                            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Error during save!");
                        }
                    }
                    else
                    {
                        CommitAchievements(hDlg);
                    }

                    return TRUE;
                }

                case IDC_RA_REVERTSELECTED:
                {
                    if (!RA_GameIsActive())
                        break;

                    //  Attempt to remove from list, but if it has an Id > 0,
                    //   attempt to remove from DB
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nSel);

                        //  Ignore if it's not modified... no changes should be present...
                        if (!Cheevo.Modified())
                            break;

                        if (MessageBox(hDlg, TEXT("Attempt to revert this achievement from file?"),
                                       TEXT("Revert from file?"), MB_YESNO) == IDYES)
                        {
                            // Find Achievement with Ach.Id()
                            const unsigned int nID = Cheevo.ID();

                            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                            if (pGameContext.ReloadAchievement(nID))
                            {
                                // Reverse find where I am in the list:
                                const size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                assert(nIndex < g_pActiveAchievements->NumAchievements());
                                if (nIndex < g_pActiveAchievements->NumAchievements())
                                    ReloadLBXData(nIndex);

                                // Finally, reselect to update AchEditor
                                g_AchievementEditorDialog.LoadAchievement(&Cheevo, FALSE);
                            }
                            else
                            {
                                MessageBox(hDlg, TEXT("Couldn't find this achievement!"), TEXT("Error!"), MB_OK);
                            }
                        }
                    }
                }
                break;

                case IDC_RA_CHKACHPROCESSINGACTIVE:
                {
                    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                    pRuntime.SetPaused(IsDlgButtonChecked(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE) == FALSE);
                    return TRUE;
                }

                case IDC_RA_RESET_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    // this could fuck up in so, so many ways. But fuck it, just reset achieved status.
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nSel);
                        if (!Cheevo.Active())
                        {
                            const TCHAR* sMessage = TEXT("Temporarily reset 'achieved' state of this achievement?");
                            if (g_nActiveAchievementSet != AchievementSet::Type::Core)
                                sMessage = TEXT("Activate this achievement?");

                            if (MessageBox(hDlg, sMessage, TEXT("Activate Achievement"), MB_YESNO) == IDYES)
                            {
                                Cheevo.Reset();
                                Cheevo.SetActive(true);

                                const size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                                if (nIndex < g_pActiveAchievements->NumAchievements())
                                {
                                    if (g_nActiveAchievementSet == AchievementSet::Type::Core)
                                        OnEditData(nIndex, Column::Achieved, "No");
                                    else
                                        OnEditData(nIndex, Column::Active, "Yes");
                                }

                                //  Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
                            }
                        }
                        else if (g_nActiveAchievementSet != AchievementSet::Type::Core)
                        {
                            if (MessageBox(hDlg, TEXT("Deactivate this achievement?"), TEXT("Deactivate Achievement"),
                                           MB_YESNO) == IDYES)
                            {
                                Cheevo.SetActive(false);

                                const size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                                if (nIndex < g_pActiveAchievements->NumAchievements())
                                {
                                    OnEditData(nIndex, Column::Active, "No");
                                }

                                //  Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
                            }
                        }

                        UpdateSelectedAchievementButtons(&Cheevo);
                    }

                    return TRUE;
                }

                case IDC_RA_ACTIVATE_ALL_ACH:
                {
                    if (!RA_GameIsActive() || g_pActiveAchievements->NumAchievements() == 0)
                        break;

                    if (MessageBox(hDlg, TEXT("Activate all achievements?"), TEXT("Activate Achievements"), MB_YESNO) ==
                        IDYES)
                    {
                        HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                        const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                        for (size_t nIndex = 0; nIndex < g_pActiveAchievements->NumAchievements(); ++nIndex)
                        {
                            Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nIndex);
                            if (!Cheevo.Active())
                            {
                                Cheevo.Reset();
                                Cheevo.SetActive(true);

                                if (g_nActiveAchievementSet == AchievementSet::Type::Core)
                                    OnEditData(nIndex, Column::Achieved, "No");
                                else
                                    OnEditData(nIndex, Column::Active, "Yes");

                                if (ra::to_signed(nIndex) == nSel)
                                    UpdateSelectedAchievementButtons(&Cheevo);
                            }
                        }
                    }

                    return TRUE;
                }
            }
            break;

        case WM_NOTIFY:
        {
#pragma warning(push)
#pragma warning(disable: 26490)
            GSL_SUPPRESS_TYPE1
            switch (reinterpret_cast<LPNMHDR>(lParam)->code)
            {
                case LVN_ITEMCHANGED: //!?? LVN on a LPNMHDR?
                {
                    iSelect = -1;
                    // MessageBox( nullptr, "Item changed!", "TEST", MB_OK );
                    GSL_SUPPRESS_TYPE1 LPNMLISTVIEW pLVInfo = reinterpret_cast<LPNMLISTVIEW>(lParam);
#pragma warning(pop)
                    if (pLVInfo->iItem != -1)
                    {
                        iSelect = pLVInfo->iItem;
                        if ((pLVInfo->uNewState &= LVIS_SELECTED) != 0)
                        {
                            const int nNewIndexSelected = pLVInfo->iItem;
                            Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nNewIndexSelected);
                            g_AchievementEditorDialog.LoadAchievement(&Cheevo, FALSE);

                            UpdateSelectedAchievementButtons(&Cheevo);
                        }
                    }
                    else
                    {
                        UpdateSelectedAchievementButtons(nullptr);
                    }
                }
                break;

                case NM_DBLCLK:
#pragma warning(push)
#pragma warning(disable: 26490)
                    GSL_SUPPRESS_TYPE1
                    if (reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem != -1)
                    {
                        SendMessage(g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_ACHIEVEMENTEDITOR, 0);
                        GSL_SUPPRESS_TYPE1 g_AchievementEditorDialog.LoadAchievement(
                            &g_pActiveAchievements->GetAchievement(reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem),
                            FALSE);
#pragma warning(pop)
                    }
                    return FALSE; //? TBD ##SD

                case NM_CUSTOMDRAW:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, ProcessCustomDraw(lParam));
                    return TRUE;
            }

            break;
        }

        case WM_CLOSE:
            EndDialog(hDlg, TRUE);
            return TRUE;

        case WM_MOVE:
            RememberWindowPosition(hDlg, "Achievements");
            break;

        case WM_SIZE:
            RememberWindowSize(hDlg, "Achievements");
            break;
    }

    return FALSE; //  Unhandled
}

INT_PTR Dlg_Achievements::CommitAchievements(HWND hDlg)
{
    constexpr int nMaxUploadLimit = 1;

    size_t nNumChecked = 0;
    std::array<int, nMaxUploadLimit> nIDsChecked{};
    std::array<int, nMaxUploadLimit> nLbxItemsChecked{};

    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (nSel != -1)
    {
        const Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);
        nLbxItemsChecked.front() = nSel;
        nIDsChecked.front() = Ach.ID();

        nNumChecked++;
    }

    if (nNumChecked == 0)
        return FALSE;

    if (LocalValidateAchievementsBeforeCommit(nLbxItemsChecked) == FALSE)
        return FALSE;

    std::string title;

    if (nNumChecked == 1)
    {
        title = "Upload Achievement";
    }
    // else
    //{
    //  title = std::string("Upload ") + std::to_string( nNumChecked ) + std::string(" Achievements");
    //}

    char message[1024];
    sprintf_s(message, 1024,
              "Uploading the selected %u achievement(s).\n"
              "Are you sure? This will update the server with your new achievements\n"
              "and players will be able to download them into their games immediately.",
              nNumChecked);

    BOOL bErrorsEncountered = FALSE;

    if (MessageBox(hDlg, NativeStr(message).c_str(), NativeStr(title).c_str(), MB_YESNO | MB_ICONWARNING) == IDYES)
    {
        for (const auto check : nLbxItemsChecked)
        {
            Achievement& NextAch = g_pActiveAchievements->GetAchievement(check);

            const BOOL bMovedFromUserToUnofficial = (g_nActiveAchievementSet == AchievementSet::Type::Local);

            unsigned int nFlags = 1 << 0; //  Active achievements! : 1
            if (g_nActiveAchievementSet == AchievementSet::Type::Core)
                nFlags |= 1 << 1; //  Core: 3
            else if (g_nActiveAchievementSet == AchievementSet::Type::Unofficial)
                nFlags |= 1 << 2; //  Retain at Unofficial: 5
            else if (g_nActiveAchievementSet == AchievementSet::Type::Local)
                nFlags |= 1 << 2; //  Promote to Unofficial: 5

            rapidjson::Document response;
            if (AttemptUploadAchievementBlocking(NextAch, nFlags, response))
            {
                if (response["Success"].GetBool())
                {
                    const ra::AchievementID nAchID = response["AchievementID"].GetUint();
                    NextAch.SetID(nAchID);

                    // Update listbox on achievements dlg

                    LbxDataAt(check, Column::Id) = std::to_string(nAchID);

                    if (bMovedFromUserToUnofficial)
                    {
                        //  Remove the achievement from the local/user achievement set,
                        //  add it to the unofficial set.
                        g_pLocalAchievements->RemoveAchievement(&NextAch);
                        g_pUnofficialAchievements->AddAchievement(&NextAch);
                        NextAch.SetCategory(ra::etoi(AchievementSet::Type::Unofficial));
                        NextAch.SetModified(FALSE);
                        RemoveAchievement(hList, nLbxItemsChecked.front());

                        // LocalAchievements->Save();
                        // UnofficialAchievements->Save();
                    }
                    else
                    {
                        //  Updated an already existing achievement, still the same position/Id.
                        NextAch.SetModified(FALSE);

                        //  Reverse find where I am in the list:
                        const size_t nIndex =
                            g_pActiveAchievements->GetAchievementIndex(*g_AchievementEditorDialog.ActiveAchievement());
                        ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                        if (nIndex < g_pActiveAchievements->NumAchievements())
                        {
                            if (g_nActiveAchievementSet == AchievementSet::Type::Core)
                                OnEditData(nIndex, Column::Modified, "No");
                        }

                        //  Save em all - we may have changed any of them :S
                        // CoreAchievements->Save();
                        // UnofficialAchievements->Save();
                        // LocalAchievements->Save();    // Will this one have changed? Maybe
                    }
                }
                else
                {
                    char buffer[1024];
                    sprintf_s(buffer, 1024, "Error!!\n%s", std::string(response["Error"].GetString()).c_str());
                    MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
                    bErrorsEncountered = TRUE;
                }
            }
        }

        if (bErrorsEncountered)
        {
            MessageBox(hDlg, TEXT("Errors encountered!\nPlease recheck your data, or get latest."), TEXT("Errors!"),
                       MB_OK);
        }
        else
        {
            char buffer[512];
            sprintf_s(buffer, 512, "Successfully uploaded data for %u achievements!", nNumChecked);
            MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Success!"), MB_OK);

            RECT rcBounds;
            GetClientRect(hList, &rcBounds);
            InvalidateRect(hList, &rcBounds, FALSE);
        }
    }
    return TRUE;
}

void Dlg_Achievements::UpdateSelectedAchievementButtons(const Achievement* restrict Cheevo) noexcept
{
    if (Cheevo == nullptr)
    {
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH),
                     g_nActiveAchievementSet == AchievementSet::Type::Local);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH),
                     g_nActiveAchievementSet == AchievementSet::Type::Local ? TRUE : Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH),
                     g_nActiveAchievementSet == AchievementSet::Type::Local);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);

        if (g_nActiveAchievementSet != AchievementSet::Type::Core)
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), TRUE);
            SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH),
                          Cheevo->Active() ? TEXT("Deactivate Selected") : TEXT("Activate Selected"));
        }
        else
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), !Cheevo->Active());
            SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), TEXT("Activate Selected"));
        }
    }
}

void Dlg_Achievements::LoadAchievements(HWND hList)
{
    SetupColumns(hList);

    for (size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i)
        AddAchievement(hList, g_pActiveAchievements->GetAchievement(i));
}

void Dlg_Achievements::OnLoad_NewRom(unsigned int nGameID)
{
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);

    HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
    if (hList != nullptr)
    {
        // SetupColumns( hList );
        LoadAchievements(hList);

        TCHAR buffer[256];
        _stprintf_s(buffer, 256, _T(" %u"), nGameID);
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_GAMEHASH, NativeStr(buffer).c_str());

        if (nGameID != 0)
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH),
                         g_nActiveAchievementSet == AchievementSet::Type::Local);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
        }

        _stprintf_s(buffer, _T(" %u"), g_pActiveAchievements->NumAchievements());
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                       NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());
    }

    UpdateSelectedAchievementButtons(nullptr);
}

void Dlg_Achievements::OnEditAchievement(const Achievement& ach)
{
    const size_t nIndex = g_pActiveAchievements->GetAchievementIndex(ach);
    ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
    if (nIndex < g_pActiveAchievements->NumAchievements())
    {
        OnEditData(nIndex, Column::Points, std::to_string(ach.Points()));

        SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                       NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

        if (g_nActiveAchievementSet == AchievementSet::Type::Core)
            OnEditData(nIndex, Column::Modified, "Yes");

        // Achievement stays active after edit, so this print is unnecessary.
        /*else
            OnEditData( nIndex, Dlg_Achievements::Active, "No" );*/
    }

    UpdateSelectedAchievementButtons(&ach);
}

void Dlg_Achievements::ReloadLBXData(int nOffset)
{
    // const char* g_sColTitles[]            = { "Id", "Title", "Author", "Achieved?", "Modified?" };
    // const char* g_sColTitlesUnofficial[]  = { "Id", "Title", "Author", "Active", "Votes" };

    const Achievement& Ach = g_pActiveAchievements->GetAchievement(nOffset);
    if (g_nActiveAchievementSet == AchievementSet::Type::Core)
    {
        OnEditData(nOffset, Column::Title, Ach.Title());
        OnEditData(nOffset, Column::Author, Ach.Author());
        OnEditData(nOffset, Column::Achieved, !Ach.Active() ? "Yes" : "No");
        OnEditData(nOffset, Column::Modified, Ach.Modified() ? "Yes" : "No");
    }
    else
    {
        OnEditData(nOffset, Column::Title, Ach.Title());
        OnEditData(nOffset, Column::Author, Ach.Author());
        OnEditData(nOffset, Column::Active, Ach.Active() ? "Yes" : "No");
        OnEditData(nOffset, Column::Votes, "N/A");
    }

    UpdateSelectedAchievementButtons(&Ach);
}

void Dlg_Achievements::OnEditData(size_t nItem, Column nColumn, const std::string& sNewData)
{
    if (nItem >= m_lbxData.size())
        return;

    m_lbxData.at(nItem).at(ra::etoi(nColumn)) = sNewData;

    HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
    if (hList != nullptr)
    {
        ra::tstring sStr{NativeStr(LbxDataAt(nItem, nColumn))}; // scoped cache
        LV_ITEM item{};
        item.mask = UINT{LVIF_TEXT};
        item.iItem = nItem;
        item.iSubItem = ra::to_signed(ra::etoi(nColumn));
        item.pszText = sStr.data();
        item.cchTextMax = 256;

        if (ListView_SetItem(hList, &item) == FALSE)
        {
            ASSERT(!"Failed to ListView_SetItem!");
        }
        else
        {
            RECT rcBounds{};
            GSL_SUPPRESS_ES47 ListView_GetItemRect(hList, nItem, &rcBounds, LVIR_BOUNDS);
            InvalidateRect(hList, &rcBounds, FALSE);
        }
    }
}

int Dlg_Achievements::GetSelectedAchievementIndex() noexcept
{
    HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
    return ListView_GetSelectionMark(hList);
}
