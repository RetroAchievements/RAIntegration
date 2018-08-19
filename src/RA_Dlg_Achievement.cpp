#include "RA_Dlg_Achievement.h"

#include "RA_Resource.h"
#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_GameData.h"
#include "RA_md5factory.h"
#include "RA_User.h"
#include "RA_GameData.h"

#ifndef _MEMORY_
#include <memory>
#endif // !_MEMORY_


namespace ra {

// Never seen it referenced in any other file


_CONSTANT_VAR MAX_TEXT_ITEM_SIZE{ 80 };

namespace enum_sizes {

_CONSTANT_VAR NUM_DLG_ACHIEVEMENT_COLS{ 6 };

} // namespace enum_sizes

using ColumnTitles = std::array<LPCTSTR, enum_sizes::NUM_DLG_ACHIEVEMENT_COLS>;
inline constexpr ColumnTitles COLUMN_TITLES_CORE{ _T("ID"), _T("Title"), _T("Points"), _T("Author"), _T("Achieved?"), _T("Modified?") };
inline constexpr ColumnTitles COLUMN_TITLES_UNOFFICIAL{ _T("ID"), _T("Title"), _T("Points"), _T("Author"), _T("Active"), _T("Votes") };
inline constexpr ColumnTitles COLUMN_TITLES_LOCAL{ _T("ID"), _T("Title"), _T("Points"), _T("Author"), _T("Active"), _T("Votes") };
inline constexpr std::array<int, enum_sizes::NUM_DLG_ACHIEVEMENT_COLS> COLUMN_SIZE{ 45, 200, 45, 80, 65, 65 };

// Doesn't really matter what's here just as long as the constant matches the value
inline constexpr std::array<DlgAchievementColumn, enum_sizes::NUM_DLG_ACHIEVEMENT_COLS> aColumn{
    DlgAchievementColumn::ID, DlgAchievementColumn::Title, DlgAchievementColumn::Points, DlgAchievementColumn::Author, DlgAchievementColumn::Achieved, DlgAchievementColumn::Active
};

_CONSTANT_VAR NUM_COLS{ enum_sizes::NUM_DLG_ACHIEVEMENT_COLS };

auto iSelect{ -1 };

} // namespace ra

Dlg_Achievements g_AchievementsDialog;

_Use_decl_annotations_
void Dlg_Achievements::SetupColumns(HWND hList)
{
    //	Remove all columns and data.
    while (ListView_DeleteColumn(hList, 0) == TRUE) {}
    ListView_DeleteAllItems(hList);
   
    for (auto it = ra::aColumn.cbegin(); it != ra::aColumn.cend(); ++it)
    {
        auto lplvColumn{ std::make_unique<LV_COLUMN>() };
        lplvColumn->mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        lplvColumn->fmt  = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;

        const auto count{ std::distance(ra::aColumn.cbegin(), it) };
        if (count == (ra::enum_sizes::NUM_DLG_ACHIEVEMENT_COLS - 1))
            lplvColumn->fmt |= LVCFMT_FILL;
        lplvColumn->cx = ra::COLUMN_SIZE.at(count);

        ra::tstring sColTitleStr;
        // NB: Do not attempt to assign pszText inside the switch (or any conditional for that matter) -SBS
        switch (const auto uCount{ ra::to_unsigned(count) }; g_nActiveAchievementSet)
        {
            case ra::AchievementSetType::Core:
                sColTitleStr = NativeStr(ra::COLUMN_TITLES_CORE.at(uCount)); //	Take a copy
                break;
            case ra::AchievementSetType::Unofficial:
                sColTitleStr = NativeStr(ra::COLUMN_TITLES_UNOFFICIAL.at(uCount)); //	Take a copy
                break;
            case ra::AchievementSetType::Local:
                sColTitleStr = NativeStr(ra::COLUMN_TITLES_LOCAL.at(uCount)); //	Take a copy
        }
        lplvColumn->pszText    = sColTitleStr.data();
        lplvColumn->cchTextMax = 255;
        lplvColumn->iSubItem   = count;

        ListView_InsertColumn(hList, count, lplvColumn.get());
    }

    m_lbxData.clear();
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
}


_NODISCARD LRESULT ProcessCustomDraw(_In_ LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
    switch (lplvcd->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            //	Before the paint cycle begins
            //	request notifications for individual listview items
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT: //Before an item is drawn
        {
            int nNextItem = static_cast<int>(lplvcd->nmcd.dwItemSpec);

            if (static_cast<size_t>(nNextItem) < g_pActiveAchievements->NumAchievements())
            {
                //if (((int)lplvcd->nmcd.dwItemSpec%2)==0)
                BOOL bSelected = &g_pActiveAchievements->GetAchievement(nNextItem) == g_AchievementEditorDialog.ActiveAchievement();
                BOOL bModified = g_pActiveAchievements->GetAchievement(nNextItem).Modified();

                lplvcd->clrText = bModified ? RGB(255, 0, 0) : RGB(0, 0, 0);
                lplvcd->clrTextBk = bSelected ? RGB(222, 222, 222) : RGB(255, 255, 255);
            }
            return CDRF_NEWFONT;
        }
        break;

        //Before a subitem is drawn
    // 	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
    // 		{
    // 			if (iSelect == (int)lplvcd->nmcd.dwItemSpec)
    // 			{
    // 				if (0 == lplvcd->iSubItem)
    // 				{
    // 					//customize subitem appearance for column 0
    // 					lplvcd->clrText   = RGB(255,128,0);
    // 					lplvcd->clrTextBk = RGB(255,255,255);
    // 
    // 					//To set a custom font:
    // 					//SelectObject(lplvcd->nmcd.hdc, 
    // 					//    <your custom HFONT>);
    // 
    // 					return CDRF_NEWFONT;
    // 				}
    // 				else if (1 == lplvcd->iSubItem)
    // 				{
    // 					//customize subitem appearance for columns 1..n
    // 					//Note: setting for column i 
    // 					//carries over to columnn i+1 unless
    // 					//      it is explicitly reset
    // 					lplvcd->clrTextBk = RGB(255,0,0);
    // 					lplvcd->clrTextBk = RGB(255,255,255);
    // 
    // 					return CDRF_NEWFONT;
    // 				}
    // 			}
    // 		}
    }
    return CDRF_DODEFAULT;
}


_Use_decl_annotations_
void Dlg_Achievements::RemoveAchievement(HWND hList, int nIter)
{
    ASSERT(nIter < ListView_GetItemCount(hList));
    ListView_DeleteItem(hList, nIter);
    m_lbxData.erase(m_lbxData.begin() + nIter);

    char buffer[16];
    sprintf_s(buffer, 16, " %u", g_pActiveAchievements->NumAchievements());
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());
    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL, NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

    UpdateSelectedAchievementButtons(nullptr);

    g_AchievementEditorDialog.LoadAchievement(nullptr, FALSE);
}

_Use_decl_annotations_
size_t Dlg_Achievements::AddAchievement(HWND hList, const Achievement& Ach)
{
    AchievementDlgRow newRow;

    //	Add to our local array:
    newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::ID)), std::to_string(Ach.ID()));
    newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Title)), Ach.Title());
    newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Points)), std::to_string(Ach.Points()));
    newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Author)), Ach.Author());

    if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
    {
        newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Achieved)), !Ach.Active() ? "Yes" : "No");
        newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Modified)), Ach.Modified() ? "Yes" : "No");
    }
    else
    {
        newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Active)), Ach.Active() ? "Yes" : "No");
        newRow.emplace(std::next(newRow.cbegin(), ra::etoi(ra::DlgAchievementColumn::Votes)), "N/A");
    }

    m_lbxData.push_back(std::move(newRow));

    auto lpItem{ std::make_unique<LV_ITEM>() };
    lpItem->mask       = LVIF_TEXT;
    lpItem->iItem      = static_cast<int>(ra::to_signed(m_lbxData.size())); // size_t could be unsigned long-long
    lpItem->cchTextMax = 256;
    
    for (lpItem->iSubItem = 0; lpItem->iSubItem < ra::NUM_COLS; lpItem->iSubItem++)
    {
        //	Cache this (stack) to ensure it lives until after ListView_*Item
        //	SD: Is this necessary?
        auto sTextData{ NativeStr(m_lbxData.back().at(lpItem->iSubItem)) };
        lpItem->pszText = sTextData.data();

        if (lpItem->iSubItem == 0)
            lpItem->iItem = ListView_InsertItem(hList, lpItem.get());
        else
            ListView_SetItem(hList, lpItem.get());
    }

    ASSERT(lpItem->iItem == (ra::to_signed(m_lbxData.size()) - 1));
    return ra::to_unsigned(lpItem->iItem);
}

_NODISCARD BOOL LocalValidateAchievementsBeforeCommit(_In_ int nLbxItems[1])
{
    char buffer[2048];
    for (size_t i = 0; i < 1; ++i)
    {
        int nIter = nLbxItems[i];
        const Achievement& Ach = g_pActiveAchievements->GetAchievement(nIter);
        if (Ach.Title().length() < 2)
        {
            sprintf_s(buffer, 2048, "Achievement title too short:\n%s\nMust be greater than 2 characters.", Ach.Title().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }
        if (Ach.Title().length() > 63)
        {
            sprintf_s(buffer, 2048, "Achievement title too long:\n%s\nMust be fewer than 80 characters.", Ach.Title().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }

        if (Ach.Description().length() < 2)
        {
            sprintf_s(buffer, 2048, "Achievement description too short:\n%s\nMust be greater than 2 characters.", Ach.Description().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }
        if (Ach.Description().length() > 255)
        {
            sprintf_s(buffer, 2048, "Achievement description too long:\n%s\nMust be fewer than 255 characters.", Ach.Description().c_str());
            MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
            return FALSE;
        }

        char sIllegalChars[] ={ '&', ':' };

        const size_t nNumIllegalChars = sizeof(sIllegalChars) / sizeof(sIllegalChars[0]);
        for (size_t i = 0; i < nNumIllegalChars; ++i)
        {
            char cNextChar = sIllegalChars[i];

            if (strchr(Ach.Title().c_str(), cNextChar) != nullptr)
            {
                sprintf_s(buffer, 2048, "Achievement title contains an illegal character: '%c'\nPlease remove and try again", cNextChar);
                MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
                return FALSE;
            }
            if (strchr(Ach.Description().c_str(), cNextChar) != nullptr)
            {
                sprintf_s(buffer, 2048, "Achievement description contains an illegal character: '%c'\nPlease remove and try again", cNextChar);
                MessageBox(nullptr, NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
                return FALSE;
            }
        }
    }

    return TRUE;
}

_Use_decl_annotations_
INT_PTR CALLBACK s_AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //	TBD: intercept any msgs?
    return g_AchievementsDialog.AchievementsProc(hDlg, nMsg, wParam, lParam);
}

_NODISCARD BOOL AttemptUploadAchievementBlocking(_In_ const Achievement& Ach,
                                                 _In_ unsigned int nFlags,
                                                 _Out_ Document& doc)
{
    const std::string sMem = Ach.CreateMemString();

    //	Deal with secret:
    char sPostCode[2048];
    sprintf_s(sPostCode, "%sSECRET%uSEC%s%uRE2%u",
              RAUsers::LocalUser().Username().c_str(),
              Ach.ID(),
              sMem.c_str(),
              Ach.Points(),
              Ach.Points() * 3);

    std::string sPostCodeHash = RAGenerateMD5(std::string(sPostCode));

    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['a'] = std::to_string(Ach.ID());
    args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
    args['n'] = Ach.Title();
    args['d'] = Ach.Description();
    args['m'] = sMem;
    args['z'] = std::to_string(Ach.Points());
    args['f'] = std::to_string(nFlags);
    args['b'] = Ach.BadgeImageURI();
    args['h'] = sPostCodeHash;

    return(RAWeb::DoBlockingRequest(ra::RequestType::SubmitAchievementData, args, doc));
}

_Use_decl_annotations_
void Dlg_Achievements::OnClickAchievementSet(ra::AchievementSetType eAchievementSet)
{
    RASetAchievementCollection(eAchievementSet);

    // Got rid of NumAchievementSetTypes and made a constant in ra::enum_sizes
    switch (eAchievementSet)
    {
        case ra::AchievementSetType::Core:
        {
            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), SW_HIDE);
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Demote from Core"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Commit Selected"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Server"));

            ::EnableWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE); // Cannot add direct to Core

            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), SW_HIDE);

            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE), TRUE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL), FALSE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL), FALSE);
        }break;
        case ra::AchievementSetType::Unofficial:
        {
            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), SW_SHOWNORMAL);
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Promote to Core"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Commit Selected"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Server"));

            ::EnableWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE); // Cannot add direct to Unofficial
            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), SW_SHOWNORMAL);

            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE), FALSE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL), TRUE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL), FALSE);
        }break;
        case ra::AchievementSetType::Local:
        {
            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), SW_SHOWNORMAL);
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Promote to Unofficial"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Save All Local"));
            SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Disk"));

            ::EnableWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), TRUE); // Can add to Local
            ::ShowWindow(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), SW_SHOWNORMAL);

            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE), FALSE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL), FALSE);
            Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL), TRUE);
        }
    }

    SetWindowText(::GetDlgItem(m_hAchievementsDlg, IDC_RA_POINT_TOTAL), NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

    Button_SetCheck(::GetDlgItem(m_hAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE), g_pActiveAchievements->ProcessingActive());
    OnLoad_NewRom(g_pCurrentGameData->GetGameID()); // assert: calls UpdateSelectedAchievementButtons
    g_AchievementEditorDialog.OnLoad_NewRom();
}

_Use_decl_annotations_
INT_PTR Dlg_Achievements::AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            m_hAchievementsDlg = hDlg;
            // TODO/TBD: Make controls managed resources and data members later
            Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_CORE), FALSE);
            Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_UNOFFICIAL), FALSE);
            Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_LOCAL), FALSE);

            switch (g_nActiveAchievementSet)
            {
                case ra::AchievementSetType::Core:
                    Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_CORE), TRUE);
                    break;
                case ra::AchievementSetType::Unofficial:
                    Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_UNOFFICIAL), TRUE);
                    break;
                case ra::AchievementSetType::Local:
                    Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_ACTIVE_LOCAL), TRUE);
                    break;
                default:
                    ASSERT(!"Unknown achievement set!");
                    break;
            }

            //	Continue as if a new rom had been loaded
            OnLoad_NewRom(g_pCurrentGameData->GetGameID());
            Button_SetCheck(::GetDlgItem(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE), g_pActiveAchievements->ProcessingActive());

            //	Click the core 
            OnClickAchievementSet(ra::AchievementSetType::Core);

            RestoreWindowPosition(hDlg, "Achievements", false, true);
        }
        return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RA_ACTIVE_CORE:
                    OnClickAchievementSet(ra::AchievementSetType::Core);
                    return TRUE;

                case IDC_RA_ACTIVE_UNOFFICIAL:
                    OnClickAchievementSet(ra::AchievementSetType::Unofficial);
                    return TRUE;

                case IDC_RA_ACTIVE_LOCAL:
                    OnClickAchievementSet(ra::AchievementSetType::Local);
                    return TRUE;

                case IDCLOSE:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_PROMOTE_ACH:
                    //	Replace with background upload?
                    if (!RA_GameIsActive())
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                    }
                    else
                    {
                        if (g_nActiveAchievementSet == ra::AchievementSetType::Local)
                        {
                            // Promote from Local to Unofficial is just a commit to Unofficial
                            return CommitAchievements(hDlg);
                        }
                        else if (g_nActiveAchievementSet == ra::AchievementSetType::Unofficial)
                        {
                            HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                            int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSel == -1)
                                return FALSE;

                            //	Promote to Core

                            //	Note: specify that this is a one-way operation
                            if (MessageBox(hDlg,
                                TEXT("Promote this achievement to the Core Achievement set.\n\n")
                                TEXT("Please note this is a one-way operation, and will allow players\n")
                                TEXT("to officially obtain this achievement and the points for it.\n"),
                                //TEXT("Note: all players who have achieved it while it has been unofficial\n")
                                //TEXT("will have to earn this again now it is in the core group.\n"),
                                TEXT("Are you sure?"), MB_YESNO | MB_ICONWARNING) == IDYES)
                            {
                                const Achievement& selectedAch = g_pActiveAchievements->GetAchievement(nSel);

                                unsigned int nFlags = 1 << 0;	//	Active achievements! : 1
                                if (g_nActiveAchievementSet == ra::AchievementSetType::Unofficial)
                                    nFlags |= 1 << 1;			//	Official achievements: 3


                                Document response;
                                if (AttemptUploadAchievementBlocking(selectedAch, nFlags, response))
                                {
                                    if (response["Success"].GetBool())
                                    {
                                        const unsigned int nID = response["AchievementID"].GetUint();

                                        //	Remove the achievement from the local/user achievement set,
                                        //	 add it to the unofficial set.
                                        Achievement& newAch = g_pCoreAchievements->AddAchievement();
                                        newAch.Set(selectedAch);
                                        g_pUnofficialAchievements->RemoveAchievement(nSel);
                                        RemoveAchievement(hList, nSel);

                                        newAch.SetActive(TRUE);	//	Disable it: all promoted ach must be reachieved

                                        //CoreAchievements->Save();
                                        //UnofficialAchievements->Save();

                                        MessageBox(hDlg, TEXT("Successfully uploaded achievement!"), TEXT("Success!"), MB_OK);
                                    }
                                    else
                                    {
                                        MessageBox(hDlg,
                                                   NativeStr(std::string("Error in upload: response from server:") + std::string(response["Error"].GetString())).c_str(),
                                                   TEXT("Error in upload!"), MB_OK);
                                    }
                                }
                                else
                                {
                                    MessageBox(hDlg, TEXT("Error connecting to server... are you online?"), TEXT("Error in upload!"), MB_OK);
                                }
                            }

                        }
                        // 					else if( g_nActiveAchievementSet == Core )
                        // 					{
                        // 						//	Demote from ra::AchievementSetType::Core
                        // 
                        // 					}
                    }

                    break;
                case IDC_RA_DOWNLOAD_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    if (g_nActiveAchievementSet == ra::AchievementSetType::Local)
                    {
                        if (MessageBox(hDlg,
                            TEXT("Are you sure that you want to reload achievements from disk?\n")
                            TEXT("This will overwrite any changes that you have made.\n"),
                            TEXT("Refresh from Disk"),
                            MB_YESNO | MB_ICONWARNING) == IDYES)
                        {
                            ra::GameID nGameID = g_pCurrentGameData->GetGameID();
                            if (nGameID != 0)
                            {
                                g_pLocalAchievements->Clear();
                                g_pLocalAchievements->LoadFromFile(nGameID);

                                //	Refresh dialog contents:
                                OnLoad_NewRom(nGameID);

                                //	Cause it to reload!
                                OnClickAchievementSet(g_nActiveAchievementSet);
                            }
                        }
                    }
                    else
                    {
                        std::ostringstream oss;
                        oss << "Are you sure that you want to download fresh achievements from " << _RA_HostName() << "?\n" <<
                            "This will overwrite any changes that you have made with fresh achievements from the server";
                        if (MessageBox(hDlg, NativeStr(oss.str()).c_str(), TEXT("Refresh from Server"), MB_YESNO | MB_ICONWARNING) == IDYES)
                        {
                            ra::GameID nGameID = g_pCurrentGameData->GetGameID();
                            if (nGameID != 0)
                            {
                                g_pCoreAchievements->DeletePatchFile(nGameID);
                                g_pUnofficialAchievements->DeletePatchFile(nGameID);

                                g_pCoreAchievements->Clear();
                                g_pUnofficialAchievements->Clear();
                                g_pLocalAchievements->Clear();

                                //	Reload the achievements file (fetch from server fresh)

                                AchievementSet::FetchFromWebBlocking(nGameID);

                                g_pLocalAchievements->LoadFromFile(nGameID);
                                g_pUnofficialAchievements->LoadFromFile(nGameID);
                                g_pCoreAchievements->LoadFromFile(nGameID);

                                //	Refresh dialog contents:
                                OnLoad_NewRom(nGameID);

                                //	Cause it to reload!
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

                    //	Add a new achievement with default params
                    Achievement& Cheevo = g_pActiveAchievements->AddAchievement();
                    Cheevo.SetAuthor(RAUsers::LocalUser().Username());
                    Cheevo.SetBadgeImage("00000");

                    //	Reverse find where I am in the list:
                    unsigned int nOffset = 0;
                    for (; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
                    {
                        if (&Cheevo == &g_pActiveAchievements->GetAchievement(nOffset))
                            break;
                    }
                    ASSERT(nOffset < g_pActiveAchievements->NumAchievements());
                    if (nOffset < g_pActiveAchievements->NumAchievements())
                        OnEditData(nOffset, ra::DlgAchievementColumn::Author, Cheevo.Author());

                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    int nNewID = AddAchievement(hList, Cheevo);
                    ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, -1);
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
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel == -1)
                        return FALSE;

                    //	switch to LocalAchievements
                    Achievement& NewClone = g_pLocalAchievements->AddAchievement();

                    //	Clone TO the user achievements
                    Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);

                    NewClone.Set(Ach);
                    NewClone.SetID(0);
                    NewClone.SetAuthor(RAUsers::LocalUser().Username());
                    char buffer[256];
                    sprintf_s(buffer, 256, "%s (copy)", NewClone.Title().c_str());
                    NewClone.SetTitle(buffer);

                    OnClickAchievementSet(ra::AchievementSetType::Local);

                    ListView_SetItemState(hList, g_pLocalAchievements->NumAchievements() - 1, LVIS_FOCUSED | LVIS_SELECTED, -1);
                    ListView_EnsureVisible(hList, g_pLocalAchievements->NumAchievements() - 1, FALSE);

                    char buffer2[16];
                    sprintf_s(buffer2, 16, " %u", g_pActiveAchievements->NumAchievements());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer2).c_str());
                    SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL, NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

                    NewClone.SetModified(TRUE);
                    UpdateSelectedAchievementButtons(&NewClone);
                }

                break;
                case IDC_RA_DEL_ACH:
                {
                    if (!RA_GameIsActive())
                    {
                        MessageBox(hDlg, TEXT("ROM not loaded: please load a ROM first!"), TEXT("Error!"), MB_OK);
                        break;
                    }

                    //	Attempt to remove from list, but if it has an ID > 0, 
                    //	 attempt to remove from DB
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);

                        if (Ach.ID() == 0)
                        {
                            //	Local achievement
                            if (MessageBox(hDlg, TEXT("Are you sure that you want to remove this achievement?"), TEXT("Remove Achievement"), MB_YESNO | MB_ICONWARNING) == IDYES)
                            {
                                g_pActiveAchievements->RemoveAchievement(nSel);
                                RemoveAchievement(hList, nSel);
                            }
                        }
                        else
                        {
                            //	This achievement exists on the server: must call SQL to remove!
                            //	Note: this is probably going to affect other users: frown on this D:
                            std::ostringstream oss;
                            oss << "This achievement exists on " << _RA_HostName() << ".\n\n"
                                << "*Removing it will affect other games*\n\n"
                                << "Are you absolutely sure you want to delete this?";
                            MessageBox(hDlg, NativeStr(oss.str()).c_str(), TEXT("Confirm Delete"), MB_YESNO | MB_ICONWARNING);
                        }
                    }
                }
                break;
                case IDC_RA_COMMIT_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    if (g_nActiveAchievementSet == ra::AchievementSetType::Local)
                    {
                        // Local save is to disk
                        if (g_pActiveAchievements->SaveToFile())
                        {
                            MessageBox(hDlg, TEXT("Saved OK!"), TEXT("OK"), MB_OK);
                            for (unsigned int i = 0; i < g_pActiveAchievements->NumAchievements(); i++)
                                g_pActiveAchievements->GetAchievement(i).SetModified(FALSE);

                            InvalidateRect(hDlg, nullptr, FALSE);
                        }
                        else
                        {
                            MessageBox(hDlg, TEXT("Error during save!"), TEXT("Error"), MB_OK | MB_ICONWARNING);
                        }

                        return TRUE;
                    }
                    else
                    {
                        return CommitAchievements(hDlg);
                    }
                }
                break;
                case IDC_RA_REVERTSELECTED:
                {
                    if (!RA_GameIsActive())
                        break;

                    //	Attempt to remove from list, but if it has an ID > 0, 
                    //	 attempt to remove from DB
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nSel);

                        //	Ignore if it's not modified... no changes should be present...
                        if (!Cheevo.Modified())
                            break;

                        if (MessageBox(hDlg,
                            TEXT("Attempt to revert this achievement from file?"),
                            TEXT("Revert from file?"), MB_YESNO) == IDYES)
                        {
                            //	Find Achievement with Ach.ID()
                            unsigned int nID = Cheevo.ID();

                            BOOL bFound = FALSE;

                            AchievementSet TempSet(g_nActiveAchievementSet);
                            if (TempSet.LoadFromFile(g_pCurrentGameData->GetGameID()))
                            {
                                Achievement* pAchBackup = TempSet.Find(nID);
                                if (pAchBackup != nullptr)
                                {
                                    Cheevo.Set(*pAchBackup);

                                    Cheevo.SetActive(TRUE);
                                    Cheevo.SetModified(FALSE);
                                    //Cheevo.SetDirtyFlag();

                                    //	Reverse find where I am in the list:
                                    size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                    assert(nIndex < g_pActiveAchievements->NumAchievements());
                                    if (nIndex < g_pActiveAchievements->NumAchievements())
                                    {
                                        if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
                                            OnEditData(nIndex, ra::DlgAchievementColumn::Achieved, "Yes");
                                        else
                                            OnEditData(nIndex, ra::DlgAchievementColumn::Active, "No");

                                        ReloadLBXData(nIndex);
                                    }

                                    //	Finally, reselect to update AchEditor
                                    g_AchievementEditorDialog.LoadAchievement(&Cheevo, FALSE);

                                    bFound = TRUE;
                                }
                            }

                            if (!bFound)
                            {
                                MessageBox(hDlg, TEXT("Couldn't find this achievement!"), TEXT("Error!"), MB_OK);
                            }
                            else
                            {
                                //MessageBox( HWnd, "Reverted", "OK!", MB_OK );
                            }
                        }
                    }
                }
                break;

                case IDC_RA_CHKACHPROCESSINGACTIVE:
                    g_pActiveAchievements->SetPaused(IsDlgButtonChecked(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE) == FALSE);
                    return TRUE;

                case IDC_RA_RESET_ACH:
                {
                    if (!RA_GameIsActive())
                        break;

                    //	this could fuck up in so, so many ways. But fuck it, just reset achieved status.
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (nSel != -1)
                    {
                        Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nSel);
                        if (!Cheevo.Active())
                        {
                            const TCHAR* sMessage = TEXT("Temporarily reset 'achieved' state of this achievement?");
                            if (g_nActiveAchievementSet != ra::AchievementSetType::Core)
                                sMessage = TEXT("Activate this achievement?");

                            if (MessageBox(hDlg, sMessage, TEXT("Activate Achievement"), MB_YESNO) == IDYES)
                            {
                                Cheevo.Reset();
                                Cheevo.SetActive(true);

                                size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                                if (nIndex < g_pActiveAchievements->NumAchievements())
                                {
                                    if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
                                        OnEditData(nIndex, ra::DlgAchievementColumn::Achieved, "No");
                                    else
                                        OnEditData(nIndex, ra::DlgAchievementColumn::Active, "Yes");
                                }

                                //	Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
                            }
                        }
                        else if (g_nActiveAchievementSet != ra::AchievementSetType::Core)
                        {
                            if (MessageBox(hDlg, TEXT("Deactivate this achievement?"), TEXT("Deactivate Achievement"), MB_YESNO) == IDYES)
                            {
                                Cheevo.SetActive(false);

                                size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
                                ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                                if (nIndex < g_pActiveAchievements->NumAchievements())
                                {
                                    OnEditData(nIndex, ra::DlgAchievementColumn::Active, "No");
                                }

                                //	Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
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

                    if (MessageBox(hDlg, TEXT("Activate all achievements?"), TEXT("Activate Achievements"), MB_YESNO) == IDYES)
                    {
                        HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
                        int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                        for (size_t nIndex = 0; nIndex < g_pActiveAchievements->NumAchievements(); ++nIndex)
                        {
                            Achievement& Cheevo = g_pActiveAchievements->GetAchievement(nIndex);
                            if (!Cheevo.Active())
                            {
                                Cheevo.Reset();
                                Cheevo.SetActive(true);

                                if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
                                    OnEditData(nIndex, ra::DlgAchievementColumn::Achieved, "No");
                                else
                                    OnEditData(nIndex, ra::DlgAchievementColumn::Active, "Yes");

                                if (nIndex == nSel)
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
            switch ((reinterpret_cast<LPNMHDR>(lParam)->code))
            {
                case LVN_ITEMCHANGED:	//!?? LVN on a LPNMHDR?
                {
                    ra::iSelect = -1;
                    //MessageBox( nullptr, "Item changed!", "TEST", MB_OK );
                    LPNMLISTVIEW pLVInfo = (LPNMLISTVIEW)lParam;
                    if (pLVInfo->iItem != -1)
                    {
                        ra::iSelect = pLVInfo->iItem;
                        if ((pLVInfo->uNewState &= LVIS_SELECTED) != 0)
                        {
                            int nNewIndexSelected = pLVInfo->iItem;
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
                    if (reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem != -1)
                    {
                        SendMessage(g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_ACHIEVEMENTEDITOR, 0);
                        g_AchievementEditorDialog.LoadAchievement(&g_pActiveAchievements->GetAchievement(reinterpret_cast<LPNMITEMACTIVATE>(lParam)->iItem), FALSE);
                    }
                    return FALSE;	//? TBD ##SD

                case NM_CUSTOMDRAW:
                    SetWindowLong(hDlg, DWL_MSGRESULT, static_cast<LONG>(ProcessCustomDraw(lParam)));
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

    return FALSE;	//	Unhandled
}

_Use_decl_annotations_
INT_PTR Dlg_Achievements::CommitAchievements(HWND hDlg)
{
    const int nMaxUploadLimit = 1;

    size_t nNumChecked = 0;
    int nIDsChecked[nMaxUploadLimit];
    int nLbxItemsChecked[nMaxUploadLimit];

    HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (nSel != -1)
    {
        Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);
        nLbxItemsChecked[0] = nSel;
        nIDsChecked[0] = Ach.ID();

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
    //else
    //{
    //	title = std::string("Upload ") + std::to_string( nNumChecked ) + std::string(" Achievements");
    //}

    char message[1024];
    sprintf_s(message, 1024, "Uploading the selected %u achievement(s).\n"
              "Are you sure? This will update the server with your new achievements\n"
              "and players will be able to download them into their games immediately.",
              nNumChecked);

    BOOL bErrorsEncountered = FALSE;

    if (MessageBox(hDlg, NativeStr(message).c_str(), NativeStr(title).c_str(), MB_YESNO | MB_ICONWARNING) == IDYES)
    {
        for (size_t i = 0; i < nNumChecked; ++i)
        {
            Achievement& NextAch = g_pActiveAchievements->GetAchievement(nLbxItemsChecked[i]);

            BOOL bMovedFromUserToUnofficial = (g_nActiveAchievementSet == ra::AchievementSetType::Local);

            unsigned int nFlags = 1 << 0;	//	Active achievements! : 1
            if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
                nFlags |= 1 << 1;			//	Core: 3
            else if (g_nActiveAchievementSet == ra::AchievementSetType::Unofficial)
                nFlags |= 1 << 2;			//	Retain at Unofficial: 5
            else if (g_nActiveAchievementSet == ra::AchievementSetType::Local)
                nFlags |= 1 << 2;			//	Promote to Unofficial: 5

            Document response;
            if (AttemptUploadAchievementBlocking(NextAch, nFlags, response))
            {
                if (response["Success"].GetBool())
                {
                    const ra::AchievementID nAchID = response["AchievementID"].GetUint();
                    NextAch.SetID(nAchID);

                    //	Update listbox on achievements dlg

                    LbxDataAt(nLbxItemsChecked[i], ra::DlgAchievementColumn::ID) = std::to_string(nAchID);

                    if (bMovedFromUserToUnofficial)
                    {
                        //	Remove the achievement from the local/user achievement set,
                        //	 add it to the unofficial set.
                        Achievement& NewAch = g_pUnofficialAchievements->AddAchievement();
                        NewAch.Set(NextAch);
                        NewAch.SetModified(FALSE);
                        g_pLocalAchievements->RemoveAchievement(nLbxItemsChecked[0]);
                        RemoveAchievement(hList, nLbxItemsChecked[0]);

                        //LocalAchievements->Save();
                        //UnofficialAchievements->Save();
                    }
                    else
                    {
                        //	Updated an already existing achievement, still the same position/ID.
                        NextAch.SetModified(FALSE);

                        //	Reverse find where I am in the list:
                        size_t nIndex = g_pActiveAchievements->GetAchievementIndex(*g_AchievementEditorDialog.ActiveAchievement());
                        ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
                        if (nIndex < g_pActiveAchievements->NumAchievements())
                        {
                            if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
                                OnEditData(nIndex, ra::DlgAchievementColumn::Modified, "No");
                        }

                        //	Save em all - we may have changed any of them :S
                        //CoreAchievements->Save();
                        //UnofficialAchievements->Save();
                        //LocalAchievements->Save();	// Will this one have changed? Maybe
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
            MessageBox(hDlg, TEXT("Errors encountered!\nPlease recheck your data, or get latest."), TEXT("Errors!"), MB_OK);
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

_Use_decl_annotations_
void Dlg_Achievements::UpdateSelectedAchievementButtons(const Achievement* Cheevo)
{
    if (Cheevo == nullptr)
    {
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), g_nActiveAchievementSet == ra::AchievementSetType::Local);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), g_nActiveAchievementSet == ra::AchievementSetType::Local ? TRUE : Cheevo->Modified());
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), g_nActiveAchievementSet == ra::AchievementSetType::Local);
        EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);

        if (g_nActiveAchievementSet != ra::AchievementSetType::Core)
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), TRUE);
            SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), Cheevo->Active() ? TEXT("Deactivate Selected") : TEXT("Activate Selected"));
        }
        else
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), !Cheevo->Active());
            SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), TEXT("Activate Selected"));
        }
    }
}

_Use_decl_annotations_
void Dlg_Achievements::LoadAchievements(HWND hList)
{
    SetupColumns(hList);

    for (size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i)
        const auto _{ AddAchievement(hList, g_pActiveAchievements->GetAchievement(i)) }; // should throw (holding off) but we marked it as [[nodiscard]]
}

_Use_decl_annotations_
void Dlg_Achievements::OnLoad_NewRom(ra::GameID nGameID)
{
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);
    EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);

    HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
    if (hList != nullptr)
    {
        //SetupColumns( hList );
        LoadAchievements(hList);

        TCHAR buffer[256];
        _stprintf_s(buffer, 256, _T(" %u"), nGameID);
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_GAMEHASH, NativeStr(buffer).c_str());

        if (nGameID != 0)
        {
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), g_nActiveAchievementSet == ra::AchievementSetType::Local);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);
            EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
        }

        _stprintf_s(buffer, _T(" %u"), g_pActiveAchievements->NumAchievements());
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());
        SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL, NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());
    }

    UpdateSelectedAchievementButtons(nullptr);
}

_Use_decl_annotations_
void Dlg_Achievements::OnGet_Achievement(const Achievement& ach)
{
    size_t nIndex = g_pActiveAchievements->GetAchievementIndex(ach);

    if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
        OnEditData(nIndex, ra::DlgAchievementColumn::Achieved, "Yes");
    else
        OnEditData(nIndex, ra::DlgAchievementColumn::Active, "No");

    UpdateSelectedAchievementButtons(&ach);
}

_Use_decl_annotations_
void Dlg_Achievements::OnEditAchievement(const Achievement& ach)
{
    size_t nIndex = g_pActiveAchievements->GetAchievementIndex(ach);
    ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
    if (nIndex < g_pActiveAchievements->NumAchievements())
    {
        OnEditData(nIndex, ra::DlgAchievementColumn::Points, std::to_string(ach.Points()));

        SetDlgItemText(m_hAchievementsDlg, IDC_RA_POINT_TOTAL, NativeStr(std::to_string(g_pActiveAchievements->PointTotal())).c_str());

        if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
            OnEditData(nIndex, ra::DlgAchievementColumn::Modified, "Yes");

        // Achievement stays active after edit, so this print is unnecessary.
        /*else
            OnEditData( nIndex, Dlg_Achievements::Active, "No" );*/
    }

    UpdateSelectedAchievementButtons(&ach);
}

_Use_decl_annotations_
void Dlg_Achievements::ReloadLBXData(int nOffset)
{
    //const char* g_sColTitles[]			= { "ID", "Title", "Author", "Achieved?", "Modified?" };
    //const char* g_sColTitlesUnofficial[]  = { "ID", "Title", "Author", "Active", "Votes" };

    Achievement& Ach = g_pActiveAchievements->GetAchievement(nOffset);
    if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
    {
        OnEditData(nOffset, ra::DlgAchievementColumn::Title, Ach.Title());
        OnEditData(nOffset, ra::DlgAchievementColumn::Author, Ach.Author());
        OnEditData(nOffset, ra::DlgAchievementColumn::Achieved, !Ach.Active() ? "Yes" : "No");
        OnEditData(nOffset, ra::DlgAchievementColumn::Modified, Ach.Modified() ? "Yes" : "No");
    }
    else
    {
        OnEditData(nOffset, ra::DlgAchievementColumn::Title, Ach.Title());
        OnEditData(nOffset, ra::DlgAchievementColumn::Author, Ach.Author());
        OnEditData(nOffset, ra::DlgAchievementColumn::Active, Ach.Active() ? "Yes" : "No");

        //char buffer[256];
        //sprintf_s( buffer, 256, "%d/%d", Ach.Upvotes(), Ach.Downvotes() );
        //OnEditData( nOffset, Dlg_Achievements::Votes, buffer );
        OnEditData(nOffset, ra::DlgAchievementColumn::Votes, "N/A");
    }

    UpdateSelectedAchievementButtons(&Ach);
}

_Use_decl_annotations_
void Dlg_Achievements::OnEditData(size_t nItem, ra::DlgAchievementColumn nColumn, const std::string& sNewData)
{
    if (nItem >= m_lbxData.size())
        return;

    // This seemed kind of suspicious, it could cause an out_range exception at runtime with bounds checking...
    // (rvalue as lvalue)
    {
        auto& row{ m_lbxData.at(nItem) };
        row.emplace(std::next(row.cbegin(), ra::etoi(nColumn)), sNewData);
    }

    // TODO: At least make hList a data member -SBS
    if (::GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS) != nullptr)
    {
        auto lpItem{ std::make_unique<LV_ITEM>() };
        lpItem->mask       = LVIF_TEXT;
        lpItem->iItem      = nItem;
        lpItem->iSubItem   = ra::etoi(nColumn);
        
        auto sStr{ NativeStr(m_lbxData.at(nItem).at(ra::etoi(nColumn))) };	//	scoped cache
        lpItem->pszText    = sStr.data();
        lpItem->cchTextMax = 256;
        if (ListView_SetItem(::GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS), lpItem.get()) == FALSE)
        {
            ASSERT(!"Failed to ListView_SetItem!");
        }
        else
        {
            auto lprcBounds{ std::make_unique<RECT>() };
            ListView_GetItemRect(::GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS), nItem, lprcBounds.get(), LVIR_BOUNDS);
            InvalidateRect(::GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS), lprcBounds.get(), FALSE);
        }
    }
}

int Dlg_Achievements::GetSelectedAchievementIndex()
{
    return ListView_GetSelectionMark(::GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS));
}
