#include "RA_Dlg_Achievement.h"

#include "RA_Achievement.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Resource.h"
#include "RA_md5factory.h"
#include "RA_BuildVer.h"
#include "RA_StringUtils.h"

#include "Exports.hh"

#include "api\UpdateAchievement.hh"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"

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
    m_vAchievementIDs.clear();
    m_lbxData.clear();

    auto i = 0;
    for (const auto& col_size : COLUMN_SIZE)
    {
        LPCTSTR sColTitle{_T("")};
        if (m_nActiveCategory == Achievement::Category::Core)
            sColTitle = COLUMN_TITLES_CORE.at(i);
        else if (m_nActiveCategory == Achievement::Category::Unofficial)
            sColTitle = COLUMN_TITLES_UNOFFICIAL.at(i);
        else if (m_nActiveCategory == Achievement::Category::Local)
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
            const auto* pAchievement = g_AchievementsDialog.GetAchievementAt(nNextItem);

            if (pAchievement != nullptr)
            {
                // if (((int)lplvcd->nmcd.dwItemSpec%2)==0)
                const BOOL bSelected = pAchievement == g_AchievementEditorDialog.ActiveAchievement();
                const BOOL bModified = pAchievement->Modified();

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
    m_vAchievementIDs.erase(m_vAchievementIDs.begin() + nIter);

    UpdateAchievementCounters();

    UpdateSelectedAchievementButtons(nullptr);

    g_AchievementEditorDialog.LoadAchievement(nullptr, FALSE);

}

void Dlg_Achievements::UpdateAchievementCounters()
{
    char buffer[16];
    sprintf_s(buffer, 16, " %zu", m_vAchievementIDs.size());
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    int nTotalPoints = 0;
    for (const auto& pAchievementId : m_vAchievementIDs)
    {
        const auto* pAchievement = pGameContext.FindAchievement(pAchievementId);
        if (pAchievement != nullptr)
            nTotalPoints += pAchievement->Points();
    }
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL,
                   NativeStr(std::to_string(nTotalPoints)).c_str());
}

int Dlg_Achievements::AddAchievement(HWND hList, const Achievement& Ach)
{
    AddAchievementRow(Ach);

    LV_ITEM item{};
    item.mask = ra::to_unsigned(LVIF_TEXT);
    item.iItem = gsl::narrow_cast<int>(m_lbxData.size() - 1);
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

    return ra::to_unsigned(item.iItem);
}

void Dlg_Achievements::AddAchievementRow(const Achievement& Ach)
{
    AchievementDlgRow newRow;

    // Add to our local array:
    newRow.emplace_back((Ach.GetCategory() == Achievement::Category::Local) ? "0" : std::to_string(Ach.ID()));
    newRow.emplace_back(Ach.Title());
    newRow.emplace_back(std::to_string(Ach.Points()));
    newRow.emplace_back(Ach.Author());

    if (Ach.GetCategory() == Achievement::Category::Core)
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

    m_vAchievementIDs.emplace_back(Ach.ID());
}

void Dlg_Achievements::UpdateAchievementList()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    std::vector<ra::AchievementID> vAchievementIDs;

    pGameContext.EnumerateAchievements([&vAchievementIDs, nActiveCategory = m_nActiveCategory](const Achievement& pAchievement)
    {
        if (pAchievement.GetCategory() == nActiveCategory)
            vAchievementIDs.push_back(pAchievement.ID());

        return true;
    });

    HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
    if (!hList)
    {
        m_vAchievementIDs.swap(vAchievementIDs);
        return;
    }

    size_t nInsertIndex = 0;
    for (auto nAchievementID : vAchievementIDs)
    {
        if (nInsertIndex == m_vAchievementIDs.size() || m_vAchievementIDs.at(nInsertIndex) != nAchievementID)
            break;

        ++nInsertIndex;
    }

    if (nInsertIndex != vAchievementIDs.size() || nInsertIndex != m_vAchievementIDs.size())
    {
        if (nInsertIndex < m_vAchievementIDs.size())
        {
            if (nInsertIndex == 0)
            {
                ListView_DeleteAllItems(hList);
            }
            else
            {
                for (auto nIndex = m_vAchievementIDs.size() - 1; nIndex >= nInsertIndex; --nIndex)
                    ListView_DeleteItem(hList, nIndex);
            }

            m_lbxData.erase(m_lbxData.begin() + nInsertIndex, m_lbxData.end());
            m_vAchievementIDs.erase(m_vAchievementIDs.begin() + nInsertIndex, m_vAchievementIDs.end());
        }

        while (nInsertIndex < vAchievementIDs.size())
        {
            AddAchievement(hList, *pGameContext.FindAchievement(vAchievementIDs.at(nInsertIndex)));
            ++nInsertIndex;
        }

        assert(ListView_GetItemCount(hList) == gsl::narrow_cast<int>(m_vAchievementIDs.size()));
    }
}

_Success_(return ) _NODISCARD BOOL
    LocalValidateAchievementsBeforeCommit(_In_reads_(1) const std::array<int, 1> nLbxItems)
{
    for (auto& nIter : nLbxItems)
    {
        const Achievement& Ach = *g_AchievementsDialog.GetAchievementAt(nIter);
        if (Ach.Title().length() < 2)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement title too short.",
                ra::StringPrintf(L"Must be greater than 2 characters:\n\n%s", Ach.Title()));
            return FALSE;
        }
        if (Ach.Title().length() > 63)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement title too long.",
                ra::StringPrintf(L"Must be fewer than 64 characters:\n\n%s", Ach.Title()));
            return FALSE;
        }

        if (Ach.Description().length() < 2)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement description too short.",
                ra::StringPrintf(L"Must be greater than 2 characters:\n\n%s", Ach.Description()));
            return FALSE;
        }
        if (Ach.Description().length() > 255)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement description too long.",
                ra::StringPrintf(L"Must be fewer than 256 characters:\n\n%s", Ach.Description()));
            return FALSE;
        }

#if RA_INTEGRATION_VERSION_MINOR < 78
        if (!ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsCustomHost())
        {
            for (size_t nGroup = 0; nGroup < Ach.NumConditionGroups(); ++nGroup)
            {
                for (size_t nCondition = 0; nCondition < Ach.NumConditions(nGroup); ++nCondition)
                {
                    const auto& pCondition = Ach.GetCondition(nGroup, nCondition);

                    if (pCondition.GetConditionType() == Condition::Type::OrNext)
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement contains pre-release logic.", L"* OrNext");
                        return FALSE;
                    }

                    if (pCondition.GetConditionType() == Condition::Type::MeasuredIf)
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement contains pre-release logic.", L"* MeasuredIf");
                        return FALSE;
                    }

                    if (pCondition.CompSource().GetSize() == MemSize::BitCount ||
                        pCondition.CompTarget().GetSize() == MemSize::BitCount)
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Achievement contains pre-release logic.", L"* BitCount memory size");
                        return FALSE;
                    }
                }
            }
        }
#endif
    }

    return TRUE;
}

// static
INT_PTR CALLBACK Dlg_Achievements::s_AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //  TBD: intercept any msgs?
    return g_AchievementsDialog.AchievementsProc(hDlg, nMsg, wParam, lParam);
}

static ra::AchievementID AttemptUploadAchievementBlocking(const Achievement& Ach, unsigned int nFlags)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const unsigned int nId = Ach.GetCategory() == Achievement::Category::Local ? 0 : Ach.ID();

    ra::api::UpdateAchievement::Request request;
    request.GameId = pGameContext.GameId();
    request.AchievementId = nId;
    request.Title = ra::Widen(Ach.Title());
    request.Description = ra::Widen(Ach.Description());
    request.Trigger = Ach.CreateMemString();
    request.Points = Ach.Points();
    request.Category = nFlags;
    request.Badge = Ach.BadgeImageURI();

    const auto& response = request.Call();
    if (response.Succeeded())
        return response.AchievementId;

    ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
        ra::StringPrintf(L"Failed to upload '%s'", Ach.Title()), ra::Widen(response.ErrorMessage));
    return 0U;
}

void Dlg_Achievements::OnClickAchievementSet(Achievement::Category nAchievementSet)
{
    m_nActiveCategory = nAchievementSet;
    SetupColumns(GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS));

    if (nAchievementSet == Achievement::Category::Core)
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
    else if (nAchievementSet == Achievement::Category::Unofficial)
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
    else if (nAchievementSet == Achievement::Category::Local)
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

    UpdateAchievementCounters();

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

            switch (m_nActiveCategory)
            {
                case Achievement::Category::Core:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, 1U, 0L);
                    break;
                case Achievement::Category::Unofficial:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, 1U, 0L);
                    break;
                case Achievement::Category::Local:
                    SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, 1U, 0L);
                    break;
                default:
                    ASSERT(!"Unknown achievement set!");
                    break;
            }

            SetupColumns(GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS));

            //  Continue as if a new rom had been loaded
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
            OnLoad_NewRom(pGameContext.GameId());

            const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
            CheckDlgButton(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE, !pRuntime.IsPaused());

            //  Click the core
            OnClickAchievementSet(Achievement::Category::Core);

            RestoreWindowPosition(hDlg, "Achievements", false, true);
        }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RA_ACTIVE_CORE:
                    OnClickAchievementSet(Achievement::Category::Core);
                    return TRUE;

                case IDC_RA_ACTIVE_UNOFFICIAL:
                    OnClickAchievementSet(Achievement::Category::Unofficial);
                    return TRUE;

                case IDC_RA_ACTIVE_LOCAL:
                    OnClickAchievementSet(Achievement::Category::Local);
                    return TRUE;

                case IDCLOSE:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_PROMOTE_ACH:
                    //  Replace with background upload?
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                    }
                    else
                    {
                        if (m_nActiveCategory == Achievement::Category::Local)
                        {
                            // Promote from Local to Unofficial is just a commit to Unofficial
                            CommitAchievements(hDlg);
                        }
                        else if (m_nActiveCategory == Achievement::Category::Unofficial)
                        {
                            HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                            const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSel == -1)
                                return FALSE;

                            //  Promote to Core
                            Achievement& selectedAch = *GetAchievementAt(nSel);

                            //  Note: specify that this is a one-way operation
                            ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
                            vmPrompt.SetHeader(ra::StringPrintf(L"Promote '%s' to core?", selectedAch.Title()));
                            vmPrompt.SetMessage(L"Please note that this is a one-way operaton, and will allow players to officially obtain the achievement and the points for it.");
                            vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
                            vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

                            if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
                            {
                                unsigned int nFlags = 1 << 0; //  Active achievements! : 1
                                if (m_nActiveCategory == Achievement::Category::Unofficial)
                                    nFlags |= 1 << 1; //  Official achievements: 3

                                const auto nID = AttemptUploadAchievementBlocking(selectedAch, nFlags);
                                if (nID > 0)
                                {
                                    //  Remove the achievement from the local/user achievement set,
                                    //   add it to the unofficial set.
                                    selectedAch.SetCategory(Achievement::Category::Core);
                                    RemoveAchievement(hList, nSel);

                                    selectedAch.SetActive(TRUE); //  Disable it: all promoted ach must be reachieved

                                    // CoreAchievements->Save();
                                    // UnofficialAchievements->Save();

                                    ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(
                                        ra::StringPrintf(L"Successfully uploaded '%s'", selectedAch.Title()));
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
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                        break;

                    if (m_nActiveCategory == Achievement::Category::Local)
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
                                pGameContext.ReloadAchievements(Achievement::Category::Local);

                                //  Refresh dialog contents:
                                OnLoad_NewRom(nGameID);

                                //  Cause it to reload!
                                OnClickAchievementSet(m_nActiveCategory);
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
                                OnClickAchievementSet(m_nActiveCategory);
                            }
                        }
                    }
                }
                break;

                case IDC_RA_ADD_ACH:
                {
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                        break;
                    }

                    //  Add a new achievement with default params
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    Achievement& Cheevo = pGameContext.NewAchievement(Achievement::Category::Local);
                    Cheevo.SetAuthor(ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername());
                    Cheevo.SetBadgeImage("00000");

                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nNewID = AddAchievement(hList, Cheevo);
                    ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                    ListView_EnsureVisible(hList, nNewID, FALSE);

                    char buffer[16];
                    sprintf_s(buffer, 16, " %zu", m_vAchievementIDs.size());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());

                    Cheevo.SetModified(TRUE);
                    UpdateSelectedAchievementButtons(&Cheevo);
                }
                break;

                case IDC_RA_CLONE_ACH:
                {
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
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
                    Achievement& NewClone = pGameContext.NewAchievement(Achievement::Category::Local);

                    //  Clone TO the user achievements
                    const Achievement& Ach = *g_AchievementsDialog.GetAchievementAt(nSel);

                    NewClone.CopyFrom(Ach);
                    NewClone.SetAuthor(ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername());
                    NewClone.SetTitle(ra::StringPrintf("%s (copy)", Ach.Title()));

                    OnClickAchievementSet(Achievement::Category::Local);

                    ListView_SetItemState(hList, m_vAchievementIDs.size() - 1,
                                          LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                    ListView_EnsureVisible(hList, m_vAchievementIDs.size() - 1, FALSE);

                    UpdateAchievementCounters();

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
                        const Achievement& Ach = *g_AchievementsDialog.GetAchievementAt(nSel);

                        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
                        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);

                        if (Ach.GetCategory() == Achievement::Category::Local)
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
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                        break;

                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    if (m_nActiveCategory == Achievement::Category::Local)
                    {
                        // Local save is to disk
                        if (pGameContext.SaveLocal())
                        {
                            ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Saved OK!");

                            for (auto nAchievementId : m_vAchievementIDs)
                            {
                                auto* pAchievement = pGameContext.FindAchievement(nAchievementId);
                                if (pAchievement)
                                    pAchievement->SetModified(FALSE);
                            }

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
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                        break;

                    //  Attempt to remove from list, but if it has an Id > 0,
                    //   attempt to remove from DB
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = *g_AchievementsDialog.GetAchievementAt(nSel);

                        //  Ignore if it's not modified... no changes should be present...
                        if (!Cheevo.Modified())
                            break;

                        if (MessageBox(hDlg, TEXT("Attempt to revert this achievement from file?"),
                                       TEXT("Revert from file?"), MB_YESNO) == IDYES)
                        {
                            // Find Achievement with Ach.Id()
                            const ra::AchievementID nID = Cheevo.ID();

                            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                            if (pGameContext.ReloadAchievement(nID))
                            {
                                Cheevo.SetModified(FALSE);

                                ReloadLBXData(nID);

                                // reselect to update AchEditor
                                if (g_AchievementEditorDialog.ActiveAchievement() &&
                                    g_AchievementEditorDialog.ActiveAchievement()->ID() == nID)
                                {
                                    g_AchievementEditorDialog.LoadAchievement(&Cheevo, FALSE);
                                }
                            }
                            else
                            {
                                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Revert failed.", L"Could not find the selected achievement in the local file.");
                            }
                        }
                    }
                }
                break;

                case IDC_RA_CHKACHPROCESSINGACTIVE:
                {
                    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                    pRuntime.SetPaused(IsDlgButtonChecked(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE) == FALSE);
                    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
                    pEmulatorContext.SetMemoryModified();
                    return TRUE;
                }

                case IDC_RA_RESET_ACH:
                {
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U)
                        break;

                    // this could fuck up in so, so many ways. But fuck it, just reset achieved status.
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    const int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = *g_AchievementsDialog.GetAchievementAt(nSel);
                        if (!Cheevo.Active())
                        {
                            const TCHAR* sMessage = TEXT("Temporarily reset 'achieved' state of this achievement?");
                            if (m_nActiveCategory != Achievement::Category::Core)
                                sMessage = TEXT("Activate this achievement?");

                            if (MessageBox(hDlg, sMessage, TEXT("Activate Achievement"), MB_YESNO) == IDYES)
                            {
                                Cheevo.SetActive(true);

                                // SetActive will only set the state to WAITING. We want it to be immediately active
                                // (i.e. it should immediately trigger if all the conditions are true).
                                auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                                auto* pTrigger = pRuntime.GetAchievementTrigger(Cheevo.ID());
                                if (pTrigger->state == RC_TRIGGER_STATE_WAITING)
                                    pTrigger->state = RC_TRIGGER_STATE_ACTIVE;

                                if (m_nActiveCategory == Achievement::Category::Core)
                                    OnEditData(nSel, Column::Achieved, "No");
                                else
                                    OnEditData(nSel, Column::Active, "Yes");

                                //  Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
                            }
                        }
                        else if (m_nActiveCategory != Achievement::Category::Core)
                        {
                            if (MessageBox(hDlg, TEXT("Deactivate this achievement?"), TEXT("Deactivate Achievement"),
                                           MB_YESNO) == IDYES)
                            {
                                Cheevo.SetActive(false);

                                OnEditData(nSel, Column::Active, "No");
                            }
                        }

                        UpdateSelectedAchievementButtons(&Cheevo);
                    }

                    return TRUE;
                }

                case IDC_RA_ACTIVATE_ALL_ACH:
                {
                    if (ra::services::ServiceLocator::Get<ra::data::GameContext>().GameId() == 0U || m_vAchievementIDs.empty())
                        break;

                    if (MessageBox(hDlg, TEXT("Activate all achievements?"), TEXT("Activate Achievements"), MB_YESNO) ==
                        IDYES)
                    {
                        HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                        const size_t nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                        size_t nIndex = 0;
                        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                        for (auto nAchievementID : m_vAchievementIDs)
                        {
                            Achievement& Cheevo = *pGameContext.FindAchievement(nAchievementID);
                            if (!Cheevo.Active())
                            {
                                Cheevo.SetActive(true);

                                // SetActive will only set the state to WAITING. We want it to be immediately active
                                // (i.e. it should immediately trigger if all the conditions are true).
                                auto* pTrigger = pRuntime.GetAchievementTrigger(Cheevo.ID());
                                if (pTrigger->state == RC_TRIGGER_STATE_WAITING)
                                    pTrigger->state = RC_TRIGGER_STATE_ACTIVE;

                                if (m_nActiveCategory == Achievement::Category::Core)
                                    OnEditData(nIndex, Column::Achieved, "No");
                                else
                                    OnEditData(nIndex, Column::Active, "Yes");

                                if (nIndex == nSel)
                                    UpdateSelectedAchievementButtons(&Cheevo);
                            }

                            ++nIndex;
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
                            Achievement& Cheevo = *g_AchievementsDialog.GetAchievementAt(nNewIndexSelected);
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
                        _RA_InvokeDialog(IDM_RA_FILES_ACHIEVEMENTEDITOR);
                        GSL_SUPPRESS_TYPE1 g_AchievementEditorDialog.LoadAchievement(
                            g_AchievementsDialog.GetAchievementAt(reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem),
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
        nLbxItemsChecked.front() = nSel;
        nIDsChecked.front() = m_vAchievementIDs.at(nSel);

        nNumChecked++;
    }

    if (nNumChecked == 0)
        return FALSE;

    if (LocalValidateAchievementsBeforeCommit(nLbxItemsChecked) == FALSE)
        return FALSE;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    std::string title;

    ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
    vmPrompt.SetMessage(L"You are about to update data on the server. Doing so will immediately make that data available for other players.");
    vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

    if (nNumChecked == 1)
    {
        const Achievement& pAch = *pGameContext.FindAchievement(nIDsChecked.front());
        if (pAch.GetCategory() == Achievement::Category::Local)
            vmPrompt.SetHeader(ra::StringPrintf(L"Are you sure you want to upload '%s'?", pAch.Title()));
        else
            vmPrompt.SetHeader(ra::StringPrintf(L"Are you sure you want to update '%s'?", pAch.Title()));
    }
    // else
    //{
    //  title = std::string("Upload ") + std::to_string( nNumChecked ) + std::string(" Achievements");
    //}

    if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
    {
        for (size_t nIndex = 0; nIndex < nNumChecked; ++nIndex)
        {
            Achievement& NextAch = *pGameContext.FindAchievement(nIDsChecked.at(nIndex));

            BOOL bMovedFromUserToUnofficial = false;

            unsigned int nFlags = ra::etoi(NextAch.GetCategory());
            if (NextAch.GetCategory() == Achievement::Category::Local)
            {
                bMovedFromUserToUnofficial = true;
                nFlags = ra::etoi(Achievement::Category::Unofficial); // promote to unofficial
            }

            const auto nAchID = AttemptUploadAchievementBlocking(NextAch, nFlags);
            if (nAchID > 0)
            {
                const auto nCurrentId = NextAch.ID();
                if (nCurrentId != nAchID)
                {
                    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().UpdateAchievementId(nCurrentId, nAchID);
                    NextAch.SetID(nAchID);

                    LbxDataAt(nLbxItemsChecked.at(nIndex), Column::Id) = std::to_string(nAchID);
                }

                // Update listbox on achievements dlg

                if (bMovedFromUserToUnofficial)
                {
                    //  Remove the achievement from the local/user achievement set,
                    //  add it to the unofficial set.
                    NextAch.SetCategory(Achievement::Category::Unofficial);
                    NextAch.SetModified(FALSE);

                    UpdateAchievementList();
                    UpdateAchievementCounters();
                }
                else
                {
                    //  Updated an already existing achievement, still the same position/Id.
                    NextAch.SetModified(FALSE);
                    OnEditData(nLbxItemsChecked.at(nIndex), Column::Modified, "No");

                    //  Save em all - we may have changed any of them :S
                    // CoreAchievements->Save();
                    // UnofficialAchievements->Save();
                    // LocalAchievements->Save();    // Will this one have changed? Maybe
                }
            }
        }

        {
            char buffer[512];
            sprintf_s(buffer, 512, "Successfully uploaded data for %zu achievements!", nNumChecked);
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
                     m_nActiveCategory == Achievement::Category::Local);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH),
                     m_nActiveCategory == Achievement::Category::Local);

        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH),
            m_nActiveCategory == Achievement::Category::Local ? TRUE : Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);

        if (m_nActiveCategory != Achievement::Category::Core)
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

void Dlg_Achievements::OnLoad_NewRom(unsigned int nGameID)
{
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);

    {
        UpdateAchievementList();

        TCHAR buffer[256];
        _stprintf_s(buffer, 256, _T(" %u"), nGameID);
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_GAMEHASH, NativeStr(buffer).c_str());

        if (nGameID != 0)
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH),
                         m_nActiveCategory == Achievement::Category::Local);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
        }

        UpdateAchievementCounters();
    }

    UpdateSelectedAchievementButtons(nullptr);
}

void Dlg_Achievements::OnEditAchievement(const Achievement& ach)
{
    size_t nIndex = 0;
    for (auto nAchievementId : m_vAchievementIDs)
    {
        if (ach.ID() == nAchievementId)
            break;

        ++nIndex;
    }

    if (nIndex < m_vAchievementIDs.size())
    {
        OnEditData(nIndex, Column::Points, std::to_string(ach.Points()));

        UpdateAchievementCounters();

        if (m_nActiveCategory == Achievement::Category::Core)
            OnEditData(nIndex, Column::Modified, "Yes");
    }

    UpdateSelectedAchievementButtons(&ach);
}

void Dlg_Achievements::ReloadLBXData(ra::AchievementID nID)
{
    // const char* g_sColTitles[]            = { "Id", "Title", "Author", "Achieved?", "Modified?" };
    // const char* g_sColTitlesUnofficial[]  = { "Id", "Title", "Author", "Active", "Votes" };

    size_t nOffset = 0;
    for (const auto nLbxID : m_vAchievementIDs)
    {
        if (nLbxID == nID)
            break;

        ++nOffset;
    }

    if (nOffset == m_vAchievementIDs.size())
        return;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const Achievement* pAchievement = pGameContext.FindAchievement(nID);
    if (pAchievement == nullptr)
        return;

    const Achievement& Ach = *pAchievement;
    if (m_nActiveCategory == Achievement::Category::Core)
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

void Dlg_Achievements::UpdateActiveAchievements()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    for (size_t nIndex = 0; nIndex < m_vAchievementIDs.size(); ++nIndex)
    {
        const Achievement* pAchievement = pGameContext.FindAchievement(m_vAchievementIDs.at(nIndex));
        if (pAchievement != nullptr)
        {
            if (m_nActiveCategory == Achievement::Category::Core)
                OnEditData(nIndex, Column::Achieved, pAchievement->Active() ? "No" : "Yes");
            else
                OnEditData(nIndex, Column::Achieved, pAchievement->Active() ? "Yes" : "No");
        }
    }
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
        item.iItem = gsl::narrow_cast<int>(nItem);
        item.iSubItem = gsl::narrow_cast<int>(ra::etoi(nColumn));
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

Achievement* Dlg_Achievements::GetAchievementAt(gsl::index nIndex) const
{
    if (ra::to_unsigned(nIndex) < m_vAchievementIDs.size())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        return pGameContext.FindAchievement(m_vAchievementIDs.at(nIndex));
    }

    return nullptr;
}

