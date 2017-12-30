#include "RA_Dlg_Achievement.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_GameData.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_GameData.h"


namespace
{
	const char* COLUMN_TITLES_CORE[] = { "ID", "Title", "Points", "Author", "Achieved?", "Modified?" };
	const char* COLUMN_TITLES_UNOFFICIAL[] = { "ID", "Title", "Points", "Author", "Active",	"Votes" };
	const char* COLUMN_TITLES_LOCAL[] = { "ID", "Title", "Points", "Author", "Active",	"Votes" };
	const int COLUMN_SIZE[] = { 45, 200, 45, 80, 65, 65 };

	const int NUM_COLS = SIZEOF_ARRAY(COLUMN_SIZE);

	int iSelect = -1;
}

Dlg_Achievements g_AchievementsDialog;

Dlg_Achievements::Dlg_Achievements()
	: m_hAchievementsDlg(nullptr)
{
}

void Dlg_Achievements::SetupColumns(HWND hList)
{
	//	Remove all columns and data.
	while (ListView_DeleteColumn(hList, 0) == TRUE) {}
	ListView_DeleteAllItems(hList);

	for (int i = 0; i < NUM_COLS; ++i)
	{
		const char* sColTitle = nullptr;
		if (g_nActiveAchievementSet == Core)
			sColTitle = COLUMN_TITLES_CORE[i];
		else if (g_nActiveAchievementSet == Unofficial)
			sColTitle = COLUMN_TITLES_UNOFFICIAL[i];
		else if (g_nActiveAchievementSet == Local)
			sColTitle = COLUMN_TITLES_LOCAL[i];

		LV_COLUMN newColumn;
		ZeroMemory(&newColumn, sizeof(newColumn));
		newColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		newColumn.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
		if (i == (NUM_COLS - 1))
			newColumn.fmt |= LVCFMT_FILL;
		newColumn.cx = COLUMN_SIZE[i];
		std::string sColTitleStr = sColTitle;	//	Take a copy
		newColumn.pszText = const_cast<LPTSTR>(NativeStr(sColTitleStr).c_str());
		newColumn.cchTextMax = 255;
		newColumn.iSubItem = i;

		ListView_InsertColumn(hList, i, &newColumn);
	}

	m_lbxData.clear();
	ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
}


LRESULT ProcessCustomDraw(LPARAM lParam)
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


void Dlg_Achievements::RemoveAchievement(HWND hList, int nIter)
{
	ASSERT(nIter < ListView_GetItemCount(hList));
	ListView_DeleteItem(hList, nIter);
	m_lbxData.erase(m_lbxData.begin() + nIter);

	char buffer[16];
	sprintf_s(buffer, 16, " %d", g_pActiveAchievements->NumAchievements());
	SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());

	UpdateSelectedAchievementButtons(NULL);

	g_AchievementEditorDialog.LoadAchievement(NULL, FALSE);
}

size_t Dlg_Achievements::AddAchievement(HWND hList, const Achievement& Ach)
{
	AchievementDlgRow newRow;
	newRow.resize(NUM_COLS);

	//	Add to our local array:
	newRow[ID] = std::to_string(Ach.ID());
	newRow[Title] = Ach.Title();
	newRow[Points] = std::to_string(Ach.Points());
	newRow[Author] = Ach.Author();

	if (g_nActiveAchievementSet == Core)
	{
		newRow[Achieved] = !Ach.Active() ? "Yes" : "No";
		newRow[Modified] = Ach.Modified() ? "Yes" : "No";
	}
	else
	{
		newRow[Active] = Ach.Active() ? "Yes" : "No";
		newRow[Votes] = "N/A";
	}

	m_lbxData.push_back(newRow);

	LV_ITEM item;
	ZeroMemory(&item, sizeof(item));

	item.mask = LVIF_TEXT;
	item.cchTextMax = 256;
	item.iItem = static_cast<int>(m_lbxData.size());

	for (item.iSubItem = 0; item.iSubItem < NUM_COLS; ++item.iSubItem)
	{
		//	Cache this (stack) to ensure it lives until after ListView_*Item
		//	SD: Is this necessary?
		std::string sTextData = m_lbxData.back()[item.iSubItem].data();
		item.pszText = const_cast<LPTSTR>(NativeStr(sTextData).c_str());

		if (item.iSubItem == 0)
			item.iItem = ListView_InsertItem(hList, &item);
		else
			ListView_SetItem(hList, &item);
	}

	ASSERT(item.iItem == (m_lbxData.size() - 1));
	return static_cast<size_t>(item.iItem);
}

BOOL LocalValidateAchievementsBeforeCommit(int nLbxItems[1])
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

		char sIllegalChars[] = { '&', ':' };

		const size_t nNumIllegalChars = sizeof(sIllegalChars) / sizeof(sIllegalChars[0]);
		for (size_t i = 0; i < nNumIllegalChars; ++i)
		{
			char cNextChar = sIllegalChars[i];

			if (strchr(Ach.Title().c_str(), cNextChar) != nullptr)
			{
				sprintf_s(buffer, 2048, "Achievement title contains an illegal character: '%c'\nPlease remove and try again", cNextChar);
				MessageBox(nullptr,  NativeStr(buffer).c_str(), TEXT("Error!"), MB_OK);
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

//static
INT_PTR CALLBACK Dlg_Achievements::s_AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//	TBD: intercept any msgs?
	return g_AchievementsDialog.AchievementsProc(hDlg, nMsg, wParam, lParam);
}

BOOL AttemptUploadAchievementBlocking(const Achievement& Ach, unsigned int nFlags, Document& doc)
{
	const std::string sMem = Ach.CreateMemString();

	//	Deal with secret:
	char sPostCode[2048];
	sprintf_s(sPostCode, "%sSECRET%dSEC%s%dRE2%d",
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

	return(RAWeb::DoBlockingRequest(RequestSubmitAchievementData, args, doc));
}

void Dlg_Achievements::OnClickAchievementSet(AchievementSetType nAchievementSet)
{
	RASetAchievementCollection(nAchievementSet);

	if (nAchievementSet == Core)
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
	else if (nAchievementSet == Unofficial)
	{
		ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
		SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TEXT("Promote to Core"));
		SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), TEXT("Commit Selected"));
		SetWindowText(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TEXT("Refresh from Server"));

		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE); // Cannot add direct to Unofficial

		ShowWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);

		CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, FALSE);
		CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, TRUE);
		CheckDlgButton(m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, FALSE);
	}
	else if (nAchievementSet == Local)
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

	OnLoad_NewRom(g_pCurrentGameData->GetGameID()); // assert: calls UpdateSelectedAchievementButtons
	g_AchievementEditorDialog.OnLoad_NewRom();
}

INT_PTR Dlg_Achievements::AchievementsProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
	case WM_INITDIALOG:
	{
		RECT r;
		GetWindowRect(g_RAMainWnd, &r);
		SetWindowPos(hDlg, NULL, r.left, r.bottom, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
		m_hAchievementsDlg = hDlg;

		SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, (WPARAM)0, (LONG)0);
		SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, (WPARAM)0, (LONG)0);
		SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, (WPARAM)0, (LONG)0);

		switch (g_nActiveAchievementSet)
		{
		case Core:
			SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, (WPARAM)1, (LONG)0);
			break;
		case Unofficial:
			SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, (WPARAM)1, (LONG)0);
			break;
		case Local:
			SendDlgItemMessage(hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, (WPARAM)1, (LONG)0);
			break;
		default:
			ASSERT(!"Unknown achievement set!");
			break;
		}

		//	Continue as if a new rom had been loaded
		OnLoad_NewRom(g_pCurrentGameData->GetGameID());
		CheckDlgButton(hDlg, IDC_RA_CHKACHPROCESSINGACTIVE, g_pActiveAchievements->ProcessingActive());

		//	Click the core 
		OnClickAchievementSet(Core);
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RA_ACTIVE_CORE:
			OnClickAchievementSet(Core);
			return TRUE;

		case IDC_RA_ACTIVE_UNOFFICIAL:
			OnClickAchievementSet(Unofficial);
			return TRUE;

		case IDC_RA_ACTIVE_LOCAL:
			OnClickAchievementSet(Local);
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
				if (g_nActiveAchievementSet == Local)
				{
					// Promote from Local to Unofficial is just a commit to Unofficial
					CommitAchievements(hDlg);
				}
				else if (g_nActiveAchievementSet == Unofficial)
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
						TEXT("Are you sure?"), MB_YESNO|MB_ICONWARNING) == IDYES)
					{
						const Achievement& selectedAch = g_pActiveAchievements->GetAchievement(nSel);

						unsigned int nFlags = 1 << 0;	//	Active achievements! : 1
						if (g_nActiveAchievementSet == Unofficial)
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
				// 						//	Demote from Core
				// 
				// 					}
			}

			break;
		case IDC_RA_DOWNLOAD_ACH:
		{
			if (!RA_GameIsActive())
				break;

			if (g_nActiveAchievementSet == Local)
			{
				if (MessageBox(hDlg,
					TEXT("Are you sure that you want to reload achievements from disk?\n")
					TEXT("This will overwrite any changes that you have made.\n"),
					TEXT("Refresh from Disk"),
					MB_YESNO | MB_ICONWARNING) == IDYES)
				{
					GameID nGameID = g_pCurrentGameData->GetGameID();
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
			else if (MessageBox(hDlg,
				TEXT("Are you sure that you want to download fresh achievements from ") RA_HOST_URL TEXT("?\n")
				TEXT("This will overwrite any changes that you have made with fresh achievements from the server.\n"),
				TEXT("Refresh from Server"),
				MB_YESNO | MB_ICONWARNING) == IDYES)
			{
				GameID nGameID = g_pCurrentGameData->GetGameID();
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
				OnEditData(nOffset, Dlg_Achievements::Author, Cheevo.Author());

			HWND hList = GetDlgItem(hDlg, IDC_RA_LISTACHIEVEMENTS);
			int nNewID = AddAchievement(hList, Cheevo);
			ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, -1);
			ListView_EnsureVisible(hList, nNewID, FALSE);

			char buffer[16];
			sprintf_s(buffer, 16, " %d", g_pActiveAchievements->NumAchievements());
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

			//	Clone TO the user achievements
			const Achievement& Ach = g_pActiveAchievements->GetAchievement(nSel);

			//	switch to LocalAchievements
			Achievement& NewClone = g_pLocalAchievements->AddAchievement();
			NewClone.Set(Ach);
			NewClone.SetID(0);
			NewClone.SetAuthor(RAUsers::LocalUser().Username());
			char buffer[256];
			sprintf_s(buffer, 256, "%s (copy)", NewClone.Title().c_str());
			NewClone.SetTitle(buffer);

			OnClickAchievementSet(Local);

			ListView_SetItemState(hList, g_pLocalAchievements->NumAchievements() - 1, LVIS_FOCUSED | LVIS_SELECTED, -1);
			ListView_EnsureVisible(hList, g_pLocalAchievements->NumAchievements() - 1, FALSE);

			char buffer2[16];
			sprintf_s(buffer2, 16, " %d", g_pActiveAchievements->NumAchievements());
			SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer2).c_str());

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
					MessageBox(hDlg,
						TEXT("This achievement exists on ") RA_HOST_URL TEXT(".\n")
						TEXT("\n")
						TEXT("*Removing it will affect other gamers*\n")
						TEXT("\n")
						TEXT("Are you absolutely sure you want to delete this??"), TEXT("Are you sure?"), MB_YESNO | MB_ICONWARNING);
				}
			}
		}
		break;
		case IDC_RA_COMMIT_ACH:
		{
			if (!RA_GameIsActive())
				break;

			if (g_nActiveAchievementSet == Local)
			{
				// Local save is to disk
				if (g_pActiveAchievements->SaveToFile())
				{
					MessageBox(hDlg, TEXT("Saved OK!"), TEXT("OK"), MB_OK);
				}
				else
				{
					MessageBox(hDlg, TEXT("Error during save!"), TEXT("Error"), MB_OK | MB_ICONWARNING);
				}

				return TRUE;
			}
			else
			{
				CommitAchievements(hDlg);
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
						if (pAchBackup != NULL)
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
								if (g_nActiveAchievementSet == Core)
									OnEditData(nIndex, Dlg_Achievements::Achieved, "Yes");
								else
									OnEditData(nIndex, Dlg_Achievements::Active, "No");

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
					if (g_nActiveAchievementSet != Core)
						sMessage = TEXT("Activate this achievement?");

					if (MessageBox(hDlg, sMessage, TEXT("Activate Achievement"), MB_YESNO) == IDYES)
					{
						Cheevo.Reset();
						Cheevo.SetActive(true);

						size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
						ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
						if (nIndex < g_pActiveAchievements->NumAchievements())
						{
							if (g_nActiveAchievementSet == Core)
								OnEditData(nIndex, Dlg_Achievements::Achieved, "No");
							else
								OnEditData(nIndex, Dlg_Achievements::Active, "Yes");
						}

						InvalidateRect(hDlg, NULL, TRUE);
						//	Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
					}
				}
				else if (g_nActiveAchievementSet != Core)
				{
					if (MessageBox(hDlg, TEXT("Deactivate this achievement?"), TEXT("Deactivate Achievement"), MB_YESNO) == IDYES)
					{
						Cheevo.SetActive(false);

						size_t nIndex = g_pActiveAchievements->GetAchievementIndex(Cheevo);
						ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
						if (nIndex < g_pActiveAchievements->NumAchievements())
						{
							OnEditData(nIndex, Dlg_Achievements::Active, "No");
						}

						InvalidateRect(hDlg, NULL, TRUE);
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

						if (g_nActiveAchievementSet == Core)
							OnEditData(nIndex, Dlg_Achievements::Achieved, "No");
						else
							OnEditData(nIndex, Dlg_Achievements::Active, "Yes");

						if (nIndex == nSel)
							UpdateSelectedAchievementButtons(&Cheevo);
					}
				}

				InvalidateRect(hDlg, NULL, TRUE);
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
			iSelect = -1;
			//MessageBox( nullptr, "Item changed!", "TEST", MB_OK );
			LPNMLISTVIEW pLVInfo = (LPNMLISTVIEW)lParam;
			if (pLVInfo->iItem != -1)
			{
				iSelect = pLVInfo->iItem;
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
				UpdateSelectedAchievementButtons(NULL);
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
	}

	return FALSE;	//	Unhandled
}

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
	sprintf_s(message, 1024, "Uploading the selected %d achievement(s).\n"
							 "Are you sure? This will update the server with your new achievements\n"
							 "and players will be able to download them into their games immediately.",
							 nNumChecked);

	BOOL bErrorsEncountered = FALSE;

	if (MessageBox(hDlg, NativeStr(message).c_str(), NativeStr(title).c_str(), MB_YESNO | MB_ICONWARNING) == IDYES)
	{
		for (size_t i = 0; i < nNumChecked; ++i)
		{
			Achievement& NextAch = g_pActiveAchievements->GetAchievement(nLbxItemsChecked[i]);

			BOOL bMovedFromUserToUnofficial = (g_nActiveAchievementSet == Local);

			unsigned int nFlags = 1 << 0;	//	Active achievements! : 1
			if (g_nActiveAchievementSet == Core)
				nFlags |= 1 << 1;			//	Core: 3
			else if (g_nActiveAchievementSet == Unofficial)
				nFlags |= 1 << 2;			//	Retain at Unofficial: 5
			else if (g_nActiveAchievementSet == Local)
				nFlags |= 1 << 2;			//	Promote to Unofficial: 5

			Document response;
			if (AttemptUploadAchievementBlocking(NextAch, nFlags, response))
			{
				if (response["Success"].GetBool())
				{
					const AchievementID nAchID = response["AchievementID"].GetUint();
					NextAch.SetID(nAchID);

					//	Update listbox on achievements dlg

					//sprintf_s( LbxDataAt( nLbxItemsChecked[i], 0 ), 32, "%d", nAchID );
					LbxDataAt(nLbxItemsChecked[i], ID) = std::to_string(nAchID);

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
							if (g_nActiveAchievementSet == Core)
								OnEditData(nIndex, Dlg_Achievements::Modified, "No");
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
			sprintf_s(buffer, 512, "Successfully uploaded data for %d achievements!", nNumChecked);
			MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Success!"), MB_OK);

			InvalidateRect(hDlg, NULL, TRUE);
		}
	}
	return TRUE;
}

void Dlg_Achievements::UpdateSelectedAchievementButtons(const Achievement* Cheevo)
{
	if (Cheevo == NULL)
	{
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_RESET_ACH), FALSE);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), FALSE);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), g_nActiveAchievementSet == Local);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), FALSE);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), FALSE);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_REVERTSELECTED), Cheevo->Modified());
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_COMMIT_ACH), g_nActiveAchievementSet == Local ? TRUE : Cheevo->Modified());
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_CLONE_ACH), TRUE);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DEL_ACH), g_nActiveAchievementSet == Local);
		EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);

		if (g_nActiveAchievementSet != Core)
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

void Dlg_Achievements::LoadAchievements(HWND hList)
{
	SetupColumns(hList);

	for (size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i)
		AddAchievement(hList, g_pActiveAchievements->GetAchievement(i));
}

void Dlg_Achievements::OnLoad_NewRom(GameID nGameID)
{
	EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), FALSE);
	EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), FALSE);
	EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), FALSE);
	EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), FALSE);

	HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
	if (hList != NULL)
	{
		//SetupColumns( hList );
		LoadAchievements(hList);

		char buffer[256];
		sprintf_s(buffer, 256, " %d", nGameID);
		SetDlgItemText(m_hAchievementsDlg, IDC_RA_GAMEHASH, NativeStr(buffer).c_str());

		if (nGameID != 0)
		{
			EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH), TRUE);
			EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ADD_ACH), g_nActiveAchievementSet == Local);
			EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_ACTIVATE_ALL_ACH), TRUE);
			EnableWindow(GetDlgItem(m_hAchievementsDlg, IDC_RA_PROMOTE_ACH), TRUE);
		}

		sprintf_s(buffer, " %d", g_pActiveAchievements->NumAchievements());
		SetDlgItemText(m_hAchievementsDlg, IDC_RA_NUMACH, NativeStr(buffer).c_str());
	}

	UpdateSelectedAchievementButtons(NULL);
}

void Dlg_Achievements::OnGet_Achievement(const Achievement& ach)
{
	size_t nIndex = g_pActiveAchievements->GetAchievementIndex(ach);

	if (g_nActiveAchievementSet == Core)
		OnEditData(nIndex, Achieved, "Yes");
	else
		OnEditData(nIndex, Active, "No");

	UpdateSelectedAchievementButtons(&ach);
}

void Dlg_Achievements::OnEditAchievement(const Achievement& ach)
{
	size_t nIndex = g_pActiveAchievements->GetAchievementIndex(ach);
	ASSERT(nIndex < g_pActiveAchievements->NumAchievements());
	if (nIndex < g_pActiveAchievements->NumAchievements())
	{
		OnEditData(nIndex, Dlg_Achievements::Points, std::to_string(ach.Points()));

		if (g_nActiveAchievementSet == Core)
			OnEditData(nIndex, Dlg_Achievements::Modified, "Yes");

		// Achievement stays active after edit, so this print is unnecessary.
		/*else
			OnEditData( nIndex, Dlg_Achievements::Active, "No" );*/
	}

	UpdateSelectedAchievementButtons(&ach);
}

void Dlg_Achievements::ReloadLBXData(int nOffset)
{
	//const char* g_sColTitles[]			= { "ID", "Title", "Author", "Achieved?", "Modified?" };
	//const char* g_sColTitlesUnofficial[]  = { "ID", "Title", "Author", "Active", "Votes" };

	Achievement& Ach = g_pActiveAchievements->GetAchievement(nOffset);
	if (g_nActiveAchievementSet == Core)
	{
		OnEditData(nOffset, Dlg_Achievements::Title, Ach.Title());
		OnEditData(nOffset, Dlg_Achievements::Author, Ach.Author());
		OnEditData(nOffset, Dlg_Achievements::Achieved, !Ach.Active() ? "Yes" : "No");
		OnEditData(nOffset, Dlg_Achievements::Modified, Ach.Modified() ? "Yes" : "No");
	}
	else
	{
		OnEditData(nOffset, Dlg_Achievements::Title, Ach.Title());
		OnEditData(nOffset, Dlg_Achievements::Author, Ach.Author());
		OnEditData(nOffset, Dlg_Achievements::Active, Ach.Active() ? "Yes" : "No");

		//char buffer[256];
		//sprintf_s( buffer, 256, "%d/%d", Ach.Upvotes(), Ach.Downvotes() );
		//OnEditData( nOffset, Dlg_Achievements::Votes, buffer );
		OnEditData(nOffset, Dlg_Achievements::Votes, "N/A");
	}

	UpdateSelectedAchievementButtons(&Ach);
}

void Dlg_Achievements::OnEditData(size_t nItem, Column nColumn, const std::string& sNewData)
{
	if (nItem >= m_lbxData.size())
		return;

	m_lbxData[nItem][nColumn] = sNewData;

	HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
	if (hList != NULL)
	{
		LV_ITEM item;
		ZeroMemory(&item, sizeof(item));

		item.mask = LVIF_TEXT;
		item.iItem = nItem;
		item.iSubItem = nColumn;
		item.cchTextMax = 256;
		std::string sStr = m_lbxData[nItem][nColumn].data();	//	scoped cache
		item.pszText = const_cast<LPTSTR>(NativeStr(sStr).c_str());
		if (ListView_SetItem(hList, &item) == FALSE)
		{
			ASSERT(!"Failed to ListView_SetItem!");
		}
	}

	InvalidateRect(m_hAchievementsDlg, NULL, TRUE);
}

int Dlg_Achievements::GetSelectedAchievementIndex()
{
	HWND hList = GetDlgItem(m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS);
	return ListView_GetSelectionMark(hList);
}
