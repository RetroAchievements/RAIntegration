#pragma once

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_User.h"
#include "RA_PopupWindows.h"
#include "RA_httpthread.h"
#include "RA_RichPresence.h"
#include "RA_md5factory.h"
#include "RA_GameData.h"
#include <fstream>

AchievementSet* g_pCoreAchievements = nullptr;
AchievementSet* g_pUnofficialAchievements = nullptr;
AchievementSet* g_pLocalAchievements = nullptr;

AchievementSet** ACH_SETS[] ={ &g_pCoreAchievements, &g_pUnofficialAchievements, &g_pLocalAchievements };
static_assert(SIZEOF_ARRAY(ACH_SETS) == NumAchievementSetTypes, "Must match!");

AchievementSetType g_nActiveAchievementSet = Core;
AchievementSet* g_pActiveAchievements = g_pCoreAchievements;


void RASetAchievementCollection(AchievementSetType Type)
{
    g_nActiveAchievementSet = Type;
    g_pActiveAchievements = *ACH_SETS[Type];
}

std::string AchievementSet::GetAchievementSetFilename(GameID nGameID)
{
    switch (m_nSetType)
    {
        case Core:
            return RA_DIR_DATA + std::to_string(nGameID) + ".txt";
        case Unofficial:
            return RA_DIR_DATA + std::to_string(nGameID) + ".txt";	// Same as Core
        case Local:
            return RA_DIR_DATA + std::to_string(nGameID) + "-User.txt";
        default:
            return "";
    }
}

BOOL AchievementSet::DeletePatchFile(GameID nGameID)
{
    if (nGameID == 0)
    {
        //	Nothing to do
        return TRUE;
    }

    if (m_nSetType == Local)
    {
        //	We do not automatically delete Local patch files
        return TRUE;
    }

    //	Remove the text file
    std::string sFilename = GetAchievementSetFilename(nGameID);
    return RemoveFileIfExists(g_sHomeDir + "\\" + sFilename);
}

//static 
void AchievementSet::OnRequestUnlocks(const Document& doc)
{
    if (!doc.HasMember("Success") || doc["Success"].GetBool() == false)
    {
        ASSERT(!"Invalid unlocks packet!");
        return;
    }

    const GameID nGameID = static_cast<GameID>(doc["GameID"].GetUint());
    const bool bHardcoreMode = doc["HardcoreMode"].GetBool();
    const Value& UserUnlocks = doc["UserUnlocks"];

    for (SizeType i = 0; i < UserUnlocks.Size(); ++i)
    {
        AchievementID nNextAchID = static_cast<AchievementID>(UserUnlocks[i].GetUint());
        //	IDs could be present in either core or unofficial:
        if (g_pCoreAchievements->Find(nNextAchID) != nullptr)
            g_pCoreAchievements->Unlock(nNextAchID);
        else if (g_pUnofficialAchievements->Find(nNextAchID) != nullptr)
            g_pUnofficialAchievements->Unlock(nNextAchID);
    }
}

Achievement& AchievementSet::AddAchievement()
{
    m_Achievements.push_back(Achievement(m_nSetType));
    return m_Achievements.back();
}

BOOL AchievementSet::RemoveAchievement(size_t nIter)
{
    if (nIter < m_Achievements.size())
    {
        m_Achievements.erase(m_Achievements.begin() + nIter);
        return TRUE;
    }
    else
    {
        ASSERT(!"There are no achievements to remove...");
        return FALSE;
    }
}

Achievement* AchievementSet::Find(AchievementID nAchievementID)
{
    std::vector<Achievement>::iterator iter = m_Achievements.begin();
    while (iter != m_Achievements.end())
    {
        if ((*iter).ID() == nAchievementID)
            return &(*iter);
        iter++;
    }

    return nullptr;
}

size_t AchievementSet::GetAchievementIndex(const Achievement& Ach)
{
    for (size_t nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
    {
        if (&Ach == &GetAchievement(nOffset))
            return nOffset;
    }

    //	Not found
    return -1;
}

unsigned int AchievementSet::NumActive() const
{
    unsigned int nNumActive = 0;
    std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
    while (iter != m_Achievements.end())
    {
        if ((*iter).Active())
            nNumActive++;
        iter++;
    }
    return nNumActive;
}

void AchievementSet::Clear()
{
    std::vector<Achievement>::iterator iter = m_Achievements.begin();
    while (iter != m_Achievements.end())
    {
        iter->Clear();
        iter++;
    }

    m_Achievements.clear();
    m_bProcessingActive = TRUE;
}

void AchievementSet::Test()
{
    if (!m_bProcessingActive)
        return;

    for (std::vector<Achievement>::iterator iter = m_Achievements.begin(); iter != m_Achievements.end(); ++iter)
    {
        Achievement& ach = (*iter);
        if (!ach.Active())
            continue;

        if (ach.Test() == TRUE)
        {
            //	Award. If can award or have already awarded, set inactive:
            ach.SetActive(FALSE);

            //	Reverse find where I am in the list:
            unsigned int nOffset = 0;
            for (nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
            {
                if (&ach == &g_pActiveAchievements->GetAchievement(nOffset))
                    break;
            }

            ASSERT(nOffset < NumAchievements());
            if (nOffset < NumAchievements())
            {
                g_AchievementsDialog.ReloadLBXData(nOffset);

                if (g_AchievementEditorDialog.ActiveAchievement() == &ach)
                    g_AchievementEditorDialog.LoadAchievement(&ach, TRUE);
            }

            if (RAUsers::LocalUser().IsLoggedIn())
            {
                const std::string sPoints = std::to_string(ach.Points());

                if (ach.ID() == 0)
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("Test: Achievement Unlocked",
                            ach.Title() + " (" + sPoints + ") (Unofficial)",
                            PopupAchievementUnlocked,
                            ach.BadgeImage()));
                }
                else if (ach.Modified())
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("Modified: Achievement Unlocked",
                            ach.Title() + " (" + sPoints + ") (Unofficial)",
                            PopupAchievementUnlocked,
                            ach.BadgeImage()));
                }
                else if (g_bRAMTamperedWith)
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("(RAM tampered with!): Achievement Unlocked",
                            ach.Title() + " (" + sPoints + ") (Unofficial)",
                            PopupAchievementError,
                            ach.BadgeImage()));
                }
                else
                {
                    PostArgs args;
                    args['u'] = RAUsers::LocalUser().Username();
                    args['t'] = RAUsers::LocalUser().Token();
                    args['a'] = std::to_string(ach.ID());
                    args['h'] = std::to_string(static_cast<int>(g_bHardcoreModeActive));

                    RAWeb::CreateThreadedHTTPRequest(RequestSubmitAwardAchievement, args);
                }
            }

            if (ach.GetPauseOnTrigger())
            {
                RA_CausePause();

                auto buffer = ra::tsprintf("Pause on Trigger: %", ach.Title());
                MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Paused"), MB_OK);
            }
        }
    }
}

BOOL AchievementSet::SaveToFile()
{
    //	Takes all achievements in this group and dumps them in the filename provided.

    std::ofstream ofile{ GetAchievementSetFilename(g_pCurrentGameData->GetGameID()), std::ios::binary };

    if(!ofile.is_open())
        return FALSE;

    using namespace std::string_literals;
    auto sNextLine = "0.030\n"s; // Min ver
    ofile.write(sNextLine.c_str(), sNextLine.length());

    sNextLine = ra::tsprintf("%\n", g_pCurrentGameData->GameTitle());
    ofile.write(sNextLine.c_str(), sNextLine.length());

    for (auto& i : m_Achievements)
    {
        i.CreateMemString();

        sNextLine = ra::tsprintf("%:%:%:%:%:%:%:%:%:%:%:%:%:%\n",
            i.ID(),
            i.CreateMemString(),
            i.Title(),
            i.Description(),
            " ", //Ach.ProgressIndicator()=="" ? " " : Ach.ProgressIndicator(),
            " ", //Ach.ProgressIndicatorMax()=="" ? " " : Ach.ProgressIndicatorMax(),
            " ", //Ach.ProgressIndicatorFormat()=="" ? " " : Ach.ProgressIndicatorFormat(),
            i.Author(),
            i.Points(),
            i.CreatedDate(),
            i.ModifiedDate(),
            i.Upvotes(),   // Fix values
            i.Downvotes(), // Fix values
            i.BadgeImageURI());

        ofile.write(sNextLine.c_str(), sNextLine.length());
    }

    return TRUE;
}

BOOL AchievementSet::Serialize(FileStream& Stream)
{
    //	Why not submit each ach straight to cloud?
    return FALSE;

    //FILE* pf = nullptr;
    //const std::string sFilename = GetAchievementSetFilename( m_nGameID );
    //if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
    //{
    //	FileStream fs( pf );
    //	Writer<FileStream> writer( fs );
    //	
    //	Document doc;
    //	doc.AddMember( "MinVer", "0.050", doc.GetAllocator() );
    //	doc.AddMember( "GameTitle", m_sPreferredGameTitle, doc.GetAllocator() );
    //	
    //	Value achElements;
    //	achElements.SetArray();

    //	std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
    //	while( iter != m_Achievements.end() )
    //	{
    //		Value nextElement;

    //		const Achievement& ach = (*iter);
    //		nextElement.AddMember( "ID", ach.ID(), doc.GetAllocator() );
    //		nextElement.AddMember( "Mem", ach.CreateMemString(), doc.GetAllocator() );
    //		nextElement.AddMember( "Title", ach.Title(), doc.GetAllocator() );
    //		nextElement.AddMember( "Description", ach.Description(), doc.GetAllocator() );
    //		nextElement.AddMember( "Author", ach.Author(), doc.GetAllocator() );
    //		nextElement.AddMember( "Points", ach.Points(), doc.GetAllocator() );
    //		nextElement.AddMember( "Created", ach.CreatedDate(), doc.GetAllocator() );
    //		nextElement.AddMember( "Modified", ach.ModifiedDate(), doc.GetAllocator() );
    //		nextElement.AddMember( "Badge", ach.BadgeImageFilename(), doc.GetAllocator() );

    //		achElements.PushBack( nextElement, doc.GetAllocator() );
    //		iter++;
    //	}

    //	doc.AddMember( "Achievements", achElements, doc.GetAllocator() );

    //	//	Build a document to persist, then pass to doc.Accept();
    //	doc.Accept( writer );

    //	fclose( pf );
    //	return TRUE;
    //}
    //else
    //{
    //	//	Could not write to file?
    //	return FALSE;
    //}
}

//	static: fetches both core and unofficial
BOOL AchievementSet::FetchFromWebBlocking(GameID nGameID)
{
    //	Can't open file: attempt to find it on SQL server!
    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['g'] = std::to_string(nGameID);
    args['h'] = g_bHardcoreModeActive ? "1" : "0";

    Document doc;
    if (RAWeb::DoBlockingRequest(RequestPatch, args, doc) &&
        doc.HasMember("Success") &&
        doc["Success"].GetBool() &&
        doc.HasMember("PatchData"))
    {
        const Value& PatchData = doc["PatchData"];
        SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
        FILE* pf = nullptr;
        fopen_s(&pf, std::string(RA_DIR_DATA + std::to_string(nGameID) + ".txt").c_str(), "wb");
        if (pf != nullptr)
        {
            FileStream fs(pf);
            Writer<FileStream> writer(fs);
            PatchData.Accept(writer);
            fclose(pf);
            return TRUE;
        }
        else
        {
            ASSERT(!"Could not open patch file for writing?");
            RA_LOG("Could not open patch file for writing?");
            return FALSE;
        }
    }
    else
    {
        //	Could not connect...
        PopupWindows::AchievementPopups().AddMessage(
            MessagePopup(std::string("Could not connect to " RA_HOST_URL "..."), "Working offline...", PopupInfo)
        );

        return FALSE;
    }
}

BOOL AchievementSet::LoadFromFile(GameID nGameID)
{
    if (nGameID == 0)
    {
        return TRUE;
    }

    const std::string sFilename = GetAchievementSetFilename(nGameID);

    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pFile = nullptr;
    errno_t nErr = fopen_s(&pFile, sFilename.c_str(), "r");
    if (pFile != nullptr)
    {
        //	Store this: we are now assuming this is the correct checksum if we have a file for it
        g_pCurrentGameData->SetGameID(nGameID);

        if (m_nSetType == Local)
        {
            const char EndLine = '\n';
            char buffer[4096];
            unsigned long nCharsRead = 0;

            //	Get min ver:
            _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
            //	UNUSED at this point? TBD

            //	Get game title:
            _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
            if (nCharsRead > 0)
            {
                buffer[nCharsRead - 1] = '\0';	//	Turn that endline into an end-string

                //	Loading Local from file...?
                g_pCurrentGameData->SetGameTitle(buffer);
            }

            while (!feof(pFile))
            {
                ZeroMemory(buffer, 4096);
                _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
                if (nCharsRead > 0)
                {
                    if (buffer[0] == 'L')
                    {
                        //SKIP
                        //RA_Leaderboard lb;
                        //lb.ParseLine(buffer);

                        //g_LeaderboardManager.AddLeaderboard(lb);
                    }
                    else if (isdigit(buffer[0]))
                    {
                        Achievement& newAch = g_pLocalAchievements->AddAchievement();
                        buffer[nCharsRead - 1] = '\0';	//	Turn that endline into an end-string

                        newAch.ParseLine(buffer);
                    }
                }
            }

            fclose(pFile);
            return TRUE;
        }
        else
        {
            Document doc;
            doc.ParseStream(FileStream(pFile));
            if (!doc.HasParseError())
            {
                //ASSERT( doc["Success"].GetBool() );
                g_pCurrentGameData->ParseData(doc);
                GameID nGameID = g_pCurrentGameData->GetGameID();

                RA_RichPresenceInterpretter::PersistAndParseScript(nGameID, g_pCurrentGameData->RichPresencePatch());

                const Value& AchievementsData = doc["Achievements"];
                for (SizeType i = 0; i < AchievementsData.Size(); ++i)
                {
                    //	Parse into correct boxes
                    unsigned int nFlags = AchievementsData[i]["Flags"].GetUint();
                    if (nFlags == 3 && m_nSetType == Core)
                    {
                        Achievement& newAch = AddAchievement();
                        newAch.Parse(AchievementsData[i]);
                    }
                    else if (nFlags == 5 && m_nSetType == Unofficial)
                    {
                        Achievement& newAch = AddAchievement();
                        newAch.Parse(AchievementsData[i]);
                    }
                }

                if (m_nSetType != Core)
                {
                    fclose(pFile);
                    return TRUE;
                }

                const Value& LeaderboardsData = doc["Leaderboards"];
                for (SizeType i = 0; i < LeaderboardsData.Size(); ++i)
                {
                    //"Leaderboards":[{"ID":"2","Mem":"STA:0xfe10=h0000_0xhf601=h0c_d0xhf601!=h0c_0xfff0=0_0xfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 1","Description":"Complete this act in the fastest time!"},

                    RA_Leaderboard lb(LeaderboardsData[i]["ID"].GetUint());
                    lb.LoadFromJSON(LeaderboardsData[i]);

                    g_LeaderboardManager.AddLeaderboard(lb);
                }
            }
            else
            {
                fclose(pFile);
                ASSERT(!"Could not parse file?!");
                return FALSE;
            }

            fclose(pFile);

            unsigned int nTotalPoints = 0;
            for (size_t i = 0; i < g_pCoreAchievements->NumAchievements(); ++i)
                nTotalPoints += g_pCoreAchievements->GetAchievement(i).Points();

            if (RAUsers::LocalUser().IsLoggedIn())
            {
                //	Loaded OK: post a request for unlocks
                PostArgs args;
                args['u'] = RAUsers::LocalUser().Username();
                args['t'] = RAUsers::LocalUser().Token();
                args['g'] = std::to_string(nGameID);
                args['h'] = g_bHardcoreModeActive ? "1" : "0";

                RAWeb::CreateThreadedHTTPRequest(RequestUnlocks, args);

                std::string sNumCoreAch = std::to_string(g_pCoreAchievements->NumAchievements());

                g_PopupWindows.AchievementPopups().AddMessage(
                    MessagePopup("Loaded " + sNumCoreAch + " achievements, Total Score " + std::to_string(nTotalPoints), "", PopupInfo));
            }

            return TRUE;
        }
    }
    else
    {
        //	Cannot open file
        RA_LOG("Cannot open file %\n", sFilename.c_str());

        
        RA_LOG("Error: %\n", ra::GetLastErrorMsg());
        return FALSE;
    }
}

void AchievementSet::SaveProgress(const char* sSaveStateFilename)
{
    if (!RAUsers::LocalUser().IsLoggedIn())
        return;

    if (sSaveStateFilename == nullptr)
        return;

    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    auto buffer = ra::tsprintf("%.rap", sSaveStateFilename);
    FILE* pf = nullptr;
    fopen_s(&pf, buffer.c_str(), "w");
    if (pf == nullptr)
    {
        ASSERT(!"Could not save progress!");
        return;
    }


    // TODO: fix this up later - sbs
    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        Achievement* pAch = &m_Achievements[i];
        if (!pAch->Active())
            continue;

        //	Write ID of achievement and num conditions:
        std::string cheevoProgressString;
        cheevoProgressString.reserve(4096);

        for (unsigned int nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp)
        {
            // uses the move assignment to drain the right side
            buffer = ra::tsprintf("%d:%d:", pAch->ID(), pAch->NumConditions(nGrp));
            cheevoProgressString += buffer;

            for (unsigned int j = 0; j < pAch->NumConditions(nGrp); ++j)
            {
                Condition& cond = pAch->GetCondition(nGrp, j);
                
                buffer = ra::tsprintf("%:%:%:%:%:",
                    cond.CurrentHits(),
                    cond.CompSource().RawValue(),
                    cond.CompSource().RawPreviousValue(),
                    cond.CompTarget().RawValue(),
                    cond.CompTarget().RawPreviousValue());
                cheevoProgressString += buffer;
            }
        }

        //	Generate a slightly different key to md5ify:
        auto sCheevoProgressMangled = ra::tsprintf("%%%%",
            RAUsers::LocalUser().Username().c_str(), cheevoProgressString, RAUsers::LocalUser().Username().c_str(), pAch->ID());

        std::string sMD5Progress = RAGenerateMD5(std::string(sCheevoProgressMangled));
        std::string sMD5Achievement = RAGenerateMD5(pAch->CreateMemString());

        fwrite(cheevoProgressString.c_str(), sizeof(char), cheevoProgressString.length(), pf);
        fwrite(sMD5Progress.c_str(), sizeof(char), sMD5Progress.length(), pf);
        fwrite(":", sizeof(char), 1, pf);
        fwrite(sMD5Achievement.c_str(), sizeof(char), sMD5Achievement.length(), pf);
        fwrite(":", sizeof(char), 1, pf);	//	Check!
    }

    fclose(pf);
}

void AchievementSet::LoadProgress(const char* sLoadStateFilename)
{
    long nFileSize = 0;

    // not too sure about the unsigned arrays

    unsigned int CondNumHits[50];	//	50 conditions per achievement
    unsigned int CondSourceVal[50];
    unsigned int CondSourceLastVal[50];
    unsigned int CondTargetVal[50];
    unsigned int CondTargetLastVal[50];
    unsigned int nID = 0;
    unsigned int nNumCond = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    int nMemStringLen = 0;

    if (!RAUsers::LocalUser().IsLoggedIn())
        return;

    if (sLoadStateFilename == nullptr)
        return;

    auto buffer = ra::tsprintf("%%.rap", RA_DIR_DATA, sLoadStateFilename);

    char* pRawFile = _MallocAndBulkReadFileToBuffer(buffer.c_str(), nFileSize);

    if (pRawFile != nullptr)
    {
        unsigned int nOffs = 0;
        while (nOffs < (unsigned int)(nFileSize - 2) && pRawFile[nOffs] != '\0')
        {
            char* pIter = &pRawFile[nOffs];

            //	Parse achievement id and num conditions
            nID = strtol(pIter, &pIter, 10); pIter++;
            nNumCond = strtol(pIter, &pIter, 10);	pIter++;

            //	Concurrently build the md5 checkstring
            auto cheevoProgressString = ra::tsprintf("%:%:", nID, nNumCond);

            ZeroMemory(CondNumHits, 50 * sizeof(unsigned int));
            ZeroMemory(CondSourceVal, 50 * sizeof(unsigned int));
            ZeroMemory(CondSourceLastVal, 50 * sizeof(unsigned int));
            ZeroMemory(CondTargetVal, 50 * sizeof(unsigned int));
            ZeroMemory(CondTargetLastVal, 50 * sizeof(unsigned int));

            for (i = 0; i < nNumCond && i < 50; ++i)
            {
                //	Parse next condition state
                CondNumHits[i] = strtol(pIter, &pIter, 10); pIter++;
                CondSourceVal[i] = strtol(pIter, &pIter, 10); pIter++;
                CondSourceLastVal[i] = strtol(pIter, &pIter, 10); pIter++;
                CondTargetVal[i] = strtol(pIter, &pIter, 10); pIter++;
                CondTargetLastVal[i] = strtol(pIter, &pIter, 10); pIter++;

                //	Concurrently build the md5 checkstring
                buffer = ra::tsprintf("%:%:%:%:%:",
                    CondNumHits[i],
                    CondSourceVal[i],
                    CondSourceLastVal[i],
                    CondTargetVal[i],
                    CondTargetLastVal[i]);

                cheevoProgressString.append(buffer);
            }

            //	Read the given md5:
            auto pGivenProgressMD5 = strtok_s(pIter, ":", &pIter);
            auto pGivenCheevoMD5 = strtok_s(pIter, ":", &pIter);

            //	Regenerate the md5 and see if it sticks:
            auto cheevoMD5TestMangled = ra::tsprintf("%%%%",
                RAUsers::LocalUser().Username().c_str(), cheevoProgressString,
                RAUsers::LocalUser().Username().c_str(), nID);

            std::string sRecalculatedProgressMD5 = RAGenerateMD5(cheevoMD5TestMangled);

            if (sRecalculatedProgressMD5 == pGivenProgressMD5)
            {
                //	Embed in achievement:
                Achievement* pAch = Find(nID);
                if (pAch != nullptr)
                {
                    std::string sMemStr = pAch->CreateMemString();

                    //	Recalculate the current achievement to see if it's compatible:
                    std::string sMemMD5 = RAGenerateMD5(sMemStr);
                    if (sMemMD5.compare(0, 32, pGivenCheevoMD5) == 0)
                    {
                        for (size_t nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp)
                        {
                            for (j = 0; j < pAch->NumConditions(nGrp); ++j)
                            {
                                Condition& cond = pAch->GetCondition(nGrp, j);

                                cond.OverrideCurrentHits(CondNumHits[j]);
                                cond.CompSource().SetValues(CondSourceVal[j], CondSourceLastVal[j]);
                                cond.CompTarget().SetValues(CondTargetVal[j], CondTargetLastVal[j]);

                                pAch->SetDirtyFlag(Dirty_Conditions);
                            }
                        }
                    }
                    else
                    {
                        ASSERT(!"Achievement progress savestate incompatible (achievement has changed?)");
                        RA_LOG("Achievement progress savestate incompatible (achievement has changed?)");
                    }
                }
                else
                {
                    ASSERT(!"Achievement doesn't exist!");
                    RA_LOG("Achievement doesn't exist!");
                }
            }
            else
            {
                //assert(!"MD5 invalid... what to do... maybe they're trying to hack achievements?");
            }

            nOffs = (pIter - pRawFile);
        }

        free(pRawFile);
        pRawFile = nullptr;
    }
}

Achievement& AchievementSet::Clone(unsigned int nIter)
{
    Achievement& newAch = AddAchievement();		//	Create a brand new achievement
    newAch.Set(m_Achievements[nIter]);		//	Copy in all values from the clone src
    newAch.SetID(0);
    newAch.SetModified(TRUE);
    newAch.SetActive(FALSE);

    return newAch;
}

BOOL AchievementSet::Unlock(AchievementID nAchID)
{
    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        if (m_Achievements[i].ID() == nAchID)
        {
            m_Achievements[i].SetActive(FALSE);
            return TRUE;	//	Update Dlg? //TBD
        }
    }

    RA_LOG("Attempted to unlock achievement % but failed!\n", nAchID);
    return FALSE;//??
}

BOOL AchievementSet::HasUnsavedChanges()
{
    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        if (m_Achievements[i].Modified())
            return TRUE;
    }

    return FALSE;
}

