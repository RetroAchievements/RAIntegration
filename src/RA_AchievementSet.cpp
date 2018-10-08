#include "RA_AchievementSet.h"

#include "RA_Core.h"
#include "RA_Dlg_Achievement.h" // RA_httpthread.h
#include "RA_Dlg_AchEditor.h" // RA_httpthread.h
#include "RA_User.h"
#include "RA_PopupWindows.h"
#include "RA_httpthread.h"
#include "RA_RichPresence.h"
#include "RA_md5factory.h"
#include "RA_GameData.h"

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"


AchievementSet* g_pCoreAchievements = nullptr;
AchievementSet* g_pUnofficialAchievements = nullptr;
AchievementSet* g_pLocalAchievements = nullptr;

inline constexpr std::array<AchievementSet**, 3> ACH_SETS
{
    &g_pCoreAchievements,
    &g_pUnofficialAchievements,
    &g_pLocalAchievements
};

AchievementSet::Type g_nActiveAchievementSet = AchievementSet::Type::Core;
AchievementSet* g_pActiveAchievements = g_pCoreAchievements;

_Use_decl_annotations_
void RASetAchievementCollection(AchievementSet::Type Type) noexcept
{
    g_nActiveAchievementSet = Type;
    g_pActiveAchievements = *ACH_SETS.at(ra::etoi(Type));
}

_Use_decl_annotations_
std::wstring AchievementSet::GetAchievementSetFilename(ra::GameID nGameID) noexcept
{
    std::wstring str;
    {
        std::wostringstream oss;
        oss << g_sHomeDir << RA_DIR_DATA << nGameID;
        if (m_nSetType == Type::Local)
            oss << L"-User";
        oss << L".txt";
        str = oss.str();
    }
    return str;
}

BOOL AchievementSet::DeletePatchFile(ra::GameID nGameID)
{
    if (nGameID == 0)
    {
        //	Nothing to do
        return TRUE;
    }

    if (m_nSetType == Type::Local)
    {
        //	We do not automatically delete Local patch files
        return TRUE;
    }

    //	Remove the text file
    std::wstring sFilename = GetAchievementSetFilename(nGameID);
    return RemoveFileIfExists(g_sHomeDir + L"\\" + sFilename);
}

//static 
void AchievementSet::OnRequestUnlocks(const rapidjson::Document& doc)
{
    if (!doc.HasMember("Success") || doc["Success"].GetBool() == false)
    {
        ASSERT(!"Invalid unlocks packet!");
        return;
    }

    const auto nGameID{ static_cast<ra::GameID>(doc["GameID"].GetUint()) };
    const auto bHardcoreMode{ doc["HardcoreMode"].GetBool() };
    const auto& UserUnlocks{ doc["UserUnlocks"] };

    for (const auto& unlocked : UserUnlocks.GetArray())
    {
        //	IDs could be present in either core or unofficial:
        const auto nNextAchID{ static_cast<ra::AchievementID>(unlocked.GetUint()) };
        if (g_pCoreAchievements->Find(nNextAchID) != nullptr)
            g_pCoreAchievements->Unlock(nNextAchID);
        else if (g_pUnofficialAchievements->Find(nNextAchID) != nullptr)
            g_pUnofficialAchievements->Unlock(nNextAchID);
    }

    // pre-fetch locked images for any achievements the player hasn't earned
    for (size_t i = 0U; i < g_pCoreAchievements->NumAchievements(); ++i)
    {
        const auto& ach{ g_pCoreAchievements->GetAchievement(i) };
        if (ach.Active())
            ra::services::g_ImageRepository.FetchImage(ra::services::ImageType::Badge, ach.BadgeImageURI() + "_lock");
    }
}

Achievement& AchievementSet::AddAchievement()
{
    m_Achievements.push_back(Achievement{});
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

Achievement* AchievementSet::Find(ra::AchievementID nAchievementID)
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
    return ra::to_unsigned(-1);
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

                if (g_nActiveAchievementSet != Type::Core)
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("Test: Achievement Unlocked",
                        ach.Title() + " (" + sPoints + ") (Unofficial)",
                        PopupAchievementUnlocked,
                        ra::services::ImageType::Badge, ach.BadgeImageURI()));
                }
                else if (ach.Modified())
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("Modified: Achievement Unlocked",
                        ach.Title() + " (" + sPoints + ") (Unofficial)",
                        PopupAchievementUnlocked,
                        ra::services::ImageType::Badge, ach.BadgeImageURI()));
                }
                else if (g_bRAMTamperedWith)
                {
                    g_PopupWindows.AchievementPopups().AddMessage(
                        MessagePopup("(RAM tampered with!): Achievement Unlocked",
                        ach.Title() + " (" + sPoints + ") (Unofficial)",
                        PopupAchievementError,
                        ra::services::ImageType::Badge, ach.BadgeImageURI()));
                }
                else
                {
                    PostArgs args;
                    args['u'] = RAUsers::LocalUser().Username();
                    args['t'] = RAUsers::LocalUser().Token();
                    args['a'] = std::to_string(ach.ID());
                    args['h'] = _RA_HardcoreModeIsActive() ? "1" : "0";

                    RAWeb::CreateThreadedHTTPRequest(RequestSubmitAwardAchievement, args);
                }
            }

            if (ach.GetPauseOnTrigger())
            {
                RA_CausePause();

                char buffer[256];
                sprintf_s(buffer, 256, "Pause on Trigger: %s", ach.Title().c_str());
                MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Paused"), MB_OK);
            }
        }
    }
}

void AchievementSet::Reset()
{
    for (Achievement& ach : m_Achievements)
        ach.Reset();

    m_bProcessingActive = TRUE;
}

BOOL AchievementSet::SaveToFile()
{
    //	Takes all achievements in this group and dumps them in the filename provided.
    FILE* pFile = nullptr;
    char sNextLine[2048];
    char sMem[2048];
    unsigned int i = 0;

    const std::wstring sFilename = GetAchievementSetFilename(g_pCurrentGameData->GetGameID());

    _wfopen_s(&pFile, sFilename.c_str(), L"w");
    if (pFile != nullptr)
    {
        sprintf_s(sNextLine, 2048, "0.030\n");						//	Min ver
        fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);

        sprintf_s(sNextLine, 2048, "%s\n", g_pCurrentGameData->GameTitle().c_str());
        fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);

        //std::vector<Achievement>::const_iterator iter = g_pActiveAchievements->m_Achievements.begin();
        //while (iter != g_pActiveAchievements->m_Achievements.end())
        for (i = 0; i < g_pLocalAchievements->NumAchievements(); ++i)
        {
            Achievement* pAch = &g_pLocalAchievements->GetAchievement(i);

            ZeroMemory(sMem, 2048);
            const auto _ = pAch->CreateMemString();

            ZeroMemory(sNextLine, 2048);
            sprintf_s(sNextLine, 2048, "%u:%s:%s:%s:%s:%s:%s:%s:%d:%lu:%lu:%d:%d:%s\n",
                pAch->ID(),
                pAch->CreateMemString().c_str(),
                pAch->Title().c_str(),
                pAch->Description().c_str(),
                " ", //Ach.ProgressIndicator()=="" ? " " : Ach.ProgressIndicator(),
                " ", //Ach.ProgressIndicatorMax()=="" ? " " : Ach.ProgressIndicatorMax(),
                " ", //Ach.ProgressIndicatorFormat()=="" ? " " : Ach.ProgressIndicatorFormat(),
                pAch->Author().c_str(),
                (unsigned short)pAch->Points(),
                (unsigned long)pAch->CreatedDate(),
                (unsigned long)pAch->ModifiedDate(),
                (unsigned short)pAch->Upvotes(), // Fix values
                (unsigned short)pAch->Downvotes(), // Fix values
                pAch->BadgeImageURI().c_str());

            fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);
        }

        fclose(pFile);
        return TRUE;
    }

    return FALSE;

    //FILE* pf = nullptr;
    //const std::string sFilename = GetAchievementSetFilename( m_nGameID );
    //if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
    //{
    //	FileStream fs( pf );
    //	
    //	CoreAchievements->Serialize( fs );
    //	UnofficialAchievements->Serialize( fs );
    //	LocalAchievements->Serialize( fs );

    //	fclose( pf );
    //}
}

//	static: fetches both core and unofficial
BOOL AchievementSet::FetchFromWebBlocking(ra::GameID nGameID)
{
    //	Can't open file: attempt to find it on SQL server!
    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['g'] = std::to_string(nGameID);
    args['h'] = _RA_HardcoreModeIsActive() ? "1" : "0";

    rapidjson::Document doc;
    if (RAWeb::DoBlockingRequest(RequestPatch, args, doc) &&
        doc.HasMember("Success") &&
        doc["Success"].GetBool() &&
        doc.HasMember("PatchData"))
    {
        std::wstring sAchSetFileName;
        {
            std::wostringstream oss;
            oss << g_sHomeDir << RA_DIR_DATA << nGameID << L".txt";
            sAchSetFileName = oss.str();
        }

        std::ofstream ofile{ sAchSetFileName };
        if (!ofile.is_open())
        {
            ASSERT(!"Could not open patch file for writing?");
            RA_LOG("Could not open patch file for writing?");
            return FALSE;
        }

        rapidjson::OStreamWrapper osw{ ofile };
        rapidjson::Writer<rapidjson::OStreamWrapper> writer{ osw };

        const auto& PatchData{ doc["PatchData"] };
        PatchData.Accept(writer);

        return TRUE;
    }
    else
    {
        //	Could not connect...
        std::ostringstream oss;
        oss << "Could not connect to " << _RA_HostName();
        PopupWindows::AchievementPopups().AddMessage(MessagePopup{ oss.str(), "Working offline..." });

        return FALSE;
    }
}

_Use_decl_annotations_
BOOL AchievementSet::LoadFromFile(ra::GameID nGameID)
{
    if (nGameID == 0)
        return TRUE;

    const auto sFilename{ GetAchievementSetFilename(nGameID) };
    std::ifstream ifile{ sFilename };
    if (!ifile.is_open())
    {
        //	Cannot open file
        RA_LOG("Cannot open file %s\n", ra::Narrow(sFilename).c_str());
        char sErrMsg[2048U]{};
        strerror_s(sErrMsg, errno);
        RA_LOG("Error: %s\n", sErrMsg);
        return FALSE;
    }

    //	Store this: we are now assuming this is the correct checksum if we have a file for it
    g_pCurrentGameData->SetGameID(nGameID);

    if (m_nSetType == Type::Local)
    {
        auto buffer{ std::make_unique<char[]>(4096U) };

        //	Get min ver: //	UNUSED at this point? TBD
        ifile.getline(buffer.get(), 4096LL);

        //	Get game title:
        ifile.getline(buffer.get(), 4096LL);
        {
            const auto nCharsRead{ ifile.gcount() };
            if (nCharsRead > 0)
            {
                const auto szCharsRead{ static_cast<std::size_t>(ra::to_unsigned(nCharsRead)) };
                buffer[szCharsRead - 1U] = '\0';	//	Turn that endline into an end-string
                g_pCurrentGameData->SetGameTitle(buffer.get()); // Loading Local from file...?
            } 
        }

        while (!ifile.eof())
        {
            ifile.getline(buffer.get(), 4096);
            {
                const auto nCharsRead{ ifile.gcount() };
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
                        auto& newAch{ g_pLocalAchievements->AddAchievement() };
                        const auto szCharsRead{ static_cast<std::size_t>(ra::to_unsigned(nCharsRead)) };
                        buffer[szCharsRead-1U] = '\0';	//	Turn that endline into an end-string

                        newAch.ParseLine(buffer.get());
                    }
                }
            }
        }

        return TRUE;
    }
    else
    {
        rapidjson::Document doc;
        rapidjson::IStreamWrapper isw{ ifile };
        doc.ParseStream(isw);

        if (doc.HasParseError())
        {
            ASSERT(!"Could not parse file?!");
            return FALSE;
        }

        //ASSERT( doc["Success"].GetBool() );
        g_pCurrentGameData->ParseData(doc);
        nGameID = g_pCurrentGameData->GetGameID();

        //	Rich Presence
        {
            std::wostringstream oss;
            oss << g_sHomeDir << RA_DIR_DATA << nGameID << L"-Rich.txt";
            _WriteBufferToFile(oss.str(), g_pCurrentGameData->RichPresencePatch());
        }
        g_RichPresenceInterpreter.ParseFromString(g_pCurrentGameData->RichPresencePatch().c_str());

        const auto& AchievementsData{ doc["Achievements"] };
        for (const auto& achData : AchievementsData.GetArray())
        {
            //	Parse into correct boxes
            const auto nFlags{ achData["Flags"].GetUint() };
            if ((nFlags == 3U) && (m_nSetType == Type::Core))
            {
                auto& newAch{ AddAchievement() };
                newAch.Parse(achData);
                newAch.SetActive(TRUE); // Activate core by default
            }
            else if ((nFlags == 5) && (m_nSetType == Type::Unofficial))
            {
                auto& newAch{ AddAchievement() };
                newAch.Parse(achData);
            }
        }

        if (m_nSetType != Type::Core)
            return TRUE;

        const auto& LeaderboardsData{ doc["Leaderboards"] };
        for (const auto& lbData : LeaderboardsData.GetArray())
        {
            //"Leaderboards":[{"ID":"2","Mem":"STA:0xfe10=h0000_0xhf601=h0c_d0xhf601!=h0c_0xfff0=0_0xfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 1","Description":"Complete this act in the fastest time!"},
            RA_Leaderboard lb{ lbData["ID"].GetUint() };

            lb.SetTitle(lbData["Title"].GetString());
            lb.SetDescription(lbData["Description"].GetString());

            const auto nFormat{ MemValue::ParseFormat(lbData["Format"].GetString()) };
            lb.ParseFromString(lbData["Mem"].GetString(), nFormat);

            ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>().AddLeaderboard(lb);
        }

        // calculate the total number of points for the core set, and pre-fetch badge images
        auto nTotalPoints{0U};
        for (size_t i = 0; i < g_pCoreAchievements->NumAchievements(); ++i)
        {
            const auto& ach{ g_pCoreAchievements->GetAchievement(i) };
            ra::services::g_ImageRepository.FetchImage(ra::services::ImageType::Badge, ach.BadgeImageURI());
            nTotalPoints += ach.Points();
        }

        if (RAUsers::LocalUser().IsLoggedIn())
        {
            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

            //	Loaded OK: post a request for unlocks
            PostArgs args;
            args['u'] = RAUsers::LocalUser().Username();
            args['t'] = RAUsers::LocalUser().Token();
            args['g'] = std::to_string(nGameID);
            args['h'] = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? "1" : "0";

            RAWeb::CreateThreadedHTTPRequest(RequestUnlocks, args);
            std::string sTitle{ "Loaded " };
            sTitle += g_pCurrentGameData->GameTitle();

            std::string sSubTitle;
            {
                std::ostringstream oss;
                oss << g_pCoreAchievements->NumAchievements() << " achievements, Total Score " << nTotalPoints;
                sSubTitle = oss.str();
            }
            
            g_PopupWindows.AchievementPopups().AddMessage({ sTitle, sSubTitle });
        }

        return TRUE;
    }

}

void AchievementSet::SaveProgress(const char* sSaveStateFilename)
{
    if (!RAUsers::LocalUser().IsLoggedIn())
        return;

    if (sSaveStateFilename == nullptr)
        return;

    std::wstring sAchievementStateFile = ra::Widen(sSaveStateFilename) + L".rap";
    FILE* pf = nullptr;
    _wfopen_s(&pf, sAchievementStateFile.c_str(), L"w");
    if (pf == nullptr)
    {
        ASSERT(!"Could not save progress!");
        return;
    }

    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        Achievement* pAch = &m_Achievements[i];
        if (pAch->Active())
        {
            std::string sProgress = pAch->CreateStateString(RAUsers::LocalUser().Username());
            fwrite(sProgress.data(), sizeof(char), sProgress.length(), pf);
        }
    }

    fclose(pf);
}

void AchievementSet::LoadProgress(const char* sLoadStateFilename)
{
    long nFileSize;

    if (!RAUsers::LocalUser().IsLoggedIn())
        return;

    if (sLoadStateFilename == nullptr)
        return;

    std::wstring sAchievementStateFile = ra::Widen(sLoadStateFilename) + L".rap";

    auto pRawFile = _BulkReadFileToBuffer(sAchievementStateFile.c_str(), nFileSize);
    if (pRawFile != nullptr)
    {
        const char* pIter = pRawFile;
        while (*pIter)
        {
            char* pUnused;
            unsigned int nID = strtoul(pIter, &pUnused, 10);
            Achievement* pAch = Find(nID);
            if (pAch != nullptr && pAch->Active())
            {
                pIter = pAch->ParseStateString(pIter, RAUsers::LocalUser().Username());
            }
            else
            {
                // achievement no longer exists, or is no longer active, skip to next one
                Achievement ach;
                ach.SetID(nID);
                pIter = ach.ParseStateString(pIter, "");
            }
        }

        delete[] pRawFile;
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

BOOL AchievementSet::Unlock(ra::AchievementID nAchID)
{
    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        if (m_Achievements[i].ID() == nAchID)
        {
            m_Achievements[i].SetActive(FALSE);
            return TRUE;	//	Update Dlg? //TBD
        }
    }

    RA_LOG("Attempted to unlock achievement %u but failed!\n", nAchID);
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

