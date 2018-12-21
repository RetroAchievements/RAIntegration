#include "RA_AchievementSet.h"

#include "RA_Core.h"
#include "RA_Dlg_Achievement.h" // RA_httpthread.h
#include "RA_Dlg_AchEditor.h" // RA_httpthread.h
#include "RA_User.h"
#include "RA_PopupWindows.h"
#include "RA_httpthread.h"
#include "RA_RichPresence.h"
#include "RA_md5factory.h"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

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
void RASetAchievementCollection(AchievementSet::Type Type)
{
    if (g_nActiveAchievementSet == Type)
        return;

    auto& pAchievementRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    for (size_t i = 0U; i < g_pActiveAchievements->NumAchievements(); ++i)
    {
        const auto& pAchievement = g_pActiveAchievements->GetAchievement(i);
        if (pAchievement.Active())
            pAchievementRuntime.DeactivateAchievement(pAchievement.ID());
    }

    g_nActiveAchievementSet = Type;
    g_pActiveAchievements = *ACH_SETS.at(ra::etoi(Type));

    for (size_t i = 0U; i < g_pActiveAchievements->NumAchievements(); ++i)
    {
        auto& pAchievement = g_pActiveAchievements->GetAchievement(i);
        if (pAchievement.Active())
        {
            // deactivate and reactivate to re-register the achievement with the runtime
            pAchievement.SetActive(false);
            pAchievement.SetActive(true);
        }
    }
}

//static 
void AchievementSet::OnRequestUnlocks(const rapidjson::Document& doc)
{
    if (!doc.HasMember("Success") || doc["Success"].GetBool() == false)
    {
        ASSERT(!"Invalid unlocks packet!");
        return;
    }

    const auto nGameID{ doc["GameID"].GetUint() };
    const auto bHardcoreMode{ doc["HardcoreMode"].GetBool() };
    const auto& UserUnlocks{ doc["UserUnlocks"] };

    std::set<unsigned int> nLockedCoreAchievements;
    for (size_t i = 0U; i < g_pCoreAchievements->NumAchievements(); ++i)
    {
        auto& pAchievement = g_pCoreAchievements->GetAchievement(i);
        pAchievement.SetActive(false);
        nLockedCoreAchievements.insert(pAchievement.ID());
    }

    std::set<unsigned int> nLockedUnofficialAchievements;
    for (size_t i = 0U; i < g_pUnofficialAchievements->NumAchievements(); ++i)
    {
        auto& pAchievement = g_pUnofficialAchievements->GetAchievement(i);
        pAchievement.SetActive(false);
        nLockedUnofficialAchievements.insert(pAchievement.ID());
    }

    for (const auto& unlocked : UserUnlocks.GetArray())
    {
        // IDs could be present in either core or unofficial:
        const auto nUnlockedAchievementId = unlocked.GetUint();
        nLockedCoreAchievements.erase(nUnlockedAchievementId);
        nLockedUnofficialAchievements.erase(nUnlockedAchievementId);
    }

    // activate any core achievements the player hasn't earned and pre-fetch the locked image
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    for (auto nAchievementId : nLockedCoreAchievements)
    {
        auto* pAchievement = g_pCoreAchievements->Find(nAchievementId);
        if (pAchievement)
        {
            pAchievement->SetActive(true);
            pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI() + "_lock");
        }
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

void AchievementSet::Clear() noexcept
{
    for (auto& ach : m_Achievements)
        ach.Clear();

    m_Achievements.clear();
    m_bProcessingActive = TRUE;
}

void AchievementSet::Test()
{
    if (!m_bProcessingActive)
        return;

    for (auto& pAchievement : m_Achievements)
    {
        if (pAchievement.Active())
            pAchievement.SetDirtyFlag(Achievement::DirtyFlags::Conditions);
    }

    std::vector<ra::services::AchievementRuntime::Change> vChanges;
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    pRuntime.Process(vChanges);

    for (const auto& pChange : vChanges)
    {
        switch (pChange.nType)
        {
            case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            {
#ifndef RA_UTEST
                RA_CausePause();
#endif
                std::wstring sMessage = ra::StringPrintf(L"Pause on Reset: %s", Find(pChange.nId)->Title());
                ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            {
                auto* pAchievement = Find(pChange.nId);
                pAchievement->SetActive(false);

#ifndef RA_UTEST
                //	Reverse find where I am in the list:
                unsigned int nOffset = 0;
                for (nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
                {
                    if (pAchievement == &g_pActiveAchievements->GetAchievement(nOffset))
                        break;
                }

                ASSERT(nOffset < NumAchievements());
                if (nOffset < NumAchievements())
                {
                    g_AchievementsDialog.ReloadLBXData(nOffset);

                    if (g_AchievementEditorDialog.ActiveAchievement() == pAchievement)
                        g_AchievementEditorDialog.LoadAchievement(pAchievement, TRUE);
                }

                if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
                {
                    if (g_nActiveAchievementSet != Type::Core)
                    {
                        g_PopupWindows.AchievementPopups().AddMessage(MessagePopup(
                            "Test: Achievement Unlocked",
                            ra::StringPrintf("%s (%u) (Unofficial)", pAchievement->Title().c_str(), pAchievement->Points()),
                            PopupMessageType::AchievementUnlocked, ra::ui::ImageType::Badge, pAchievement->BadgeImageURI()));
                    }
                    else if (pAchievement->Modified())
                    {
                        g_PopupWindows.AchievementPopups().AddMessage(MessagePopup(
                            "Modified: Achievement Unlocked",
                            ra::StringPrintf("%s (%u) (Unofficial)", pAchievement->Title().c_str(), pAchievement->Points()),
                            PopupMessageType::AchievementUnlocked, ra::ui::ImageType::Badge, pAchievement->BadgeImageURI()));
                    }
                    else if (g_bRAMTamperedWith)
                    {
                        g_PopupWindows.AchievementPopups().AddMessage(MessagePopup(
                            "(RAM tampered with!): Achievement Unlocked",
                            ra::StringPrintf("%s (%u) (Unofficial)", pAchievement->Title().c_str(), pAchievement->Points()),
                            PopupMessageType::AchievementError, ra::ui::ImageType::Badge, pAchievement->BadgeImageURI()));
                    }
                    else
                    {
                        PostArgs args;
                        args['u'] = RAUsers::LocalUser().Username();
                        args['t'] = RAUsers::LocalUser().Token();
                        args['a'] = std::to_string(pAchievement->ID());
                        args['h'] = _RA_HardcoreModeIsActive() ? "1" : "0";

                        RAWeb::CreateThreadedHTTPRequest(RequestSubmitAwardAchievement, args);
                    }
                }
#endif

                if (pAchievement->GetPauseOnTrigger())
                {
#ifndef RA_UTEST
                    RA_CausePause();
#endif
                    std::wstring sMessage = ra::StringPrintf(L"Pause on Trigger: %s", pAchievement->Title());
                    ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                }

                break;
            }
        }
    }
}

void AchievementSet::Reset() noexcept
{
    for (Achievement& ach : m_Achievements)
        ach.Reset();

    m_bProcessingActive = TRUE;
}

bool AchievementSet::SaveToFile() const
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    // Commits local achievements to the file
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(static_cast<unsigned int>(pGameContext.GameId())));
    if (pData == nullptr)
        return false;

    pData->WriteLine(_RA_IntegrationVersion()); // version used to create the file
    pData->WriteLine(pGameContext.GameTitle());

    for (auto& ach : *g_pLocalAchievements)
    {
        pData->Write(std::to_string(ach.ID()));
        pData->Write(":");
        pData->Write(ach.CreateMemString());
        pData->Write(":");
        pData->Write(ach.Title()); // TODO: escape colons
        pData->Write(":");
        pData->Write(ach.Description()); // TODO: escape colons
        pData->Write("::::");            // progress indicator/max/format
        pData->Write(ach.Author());
        pData->Write(":");
        pData->Write(std::to_string(ach.Points()));
        pData->Write(":");
        pData->Write(std::to_string(ach.CreatedDate()));
        pData->Write(":");
        pData->Write(std::to_string(ach.ModifiedDate()));
        pData->Write(":::"); // upvotes/downvotes
        pData->Write(ach.BadgeImageURI());

        pData->WriteLine();
    }

    return true;
}

//	static: fetches both core and unofficial
BOOL AchievementSet::FetchFromWebBlocking(unsigned int nGameID)
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
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::GameData, std::to_wstring(nGameID));
        if (pData != nullptr)
        {
            rapidjson::Document patchData;
            patchData.CopyFrom(doc["PatchData"], doc.GetAllocator());
            SaveDocument(patchData, *pData.get());
        }

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
bool AchievementSet::LoadFromFile(unsigned int nGameID)
{
    if (nGameID == 0)
        return true;

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

    if (m_nSetType == Type::Local)
    {
        auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::UserAchievements, std::to_wstring(nGameID));
        if (pData == nullptr)
            return false;

        std::string sLine;
        pData->GetLine(sLine); // version used to create the file
        pData->GetLine(sLine); // game title

        while (pData->GetLine(sLine))
        {
            // achievement lines start with the achievement id
            if (!sLine.empty() && isdigit(sLine.front()))
            {
                auto& newAch = g_pLocalAchievements->AddAchievement();
                newAch.ParseLine(sLine.c_str());
            }
        }

        return true;
    }
    else
    {
        auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, std::to_wstring(nGameID));
        if (pData == nullptr)
            return false;
              
        rapidjson::Document doc;
        if (!LoadDocument(doc, *pData.get()))
        {
            ASSERT(!"Could not parse file?!");
            return false;
        }

        if (m_nSetType == Type::Core)
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
            pGameContext.LoadGame(nGameID, ra::Widen(doc["Title"].GetString()));

            if (doc.HasMember("RichPresencePatch"))
            {
                std::string sRichPresence;
                if (!doc["RichPresencePatch"].IsNull())
                    sRichPresence = doc["RichPresencePatch"].GetString();

                auto pRich = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence, std::to_wstring(nGameID));
                pRich->Write(sRichPresence);
            }
        }

        const auto& AchievementsData{ doc["Achievements"] };
        for (const auto& achData : AchievementsData.GetArray())
        {
            //	Parse into correct boxes
            const auto nFlags{ achData["Flags"].GetUint() };
            if ((nFlags == 3U) && (m_nSetType == Type::Core))
            {
                auto& newAch{ AddAchievement() };
                newAch.Parse(achData);
            }
            else if ((nFlags == 5) && (m_nSetType == Type::Unofficial))
            {
                auto& newAch{ AddAchievement() };
                newAch.Parse(achData);
            }
        }

        if (m_nSetType != Type::Core)
            return true;

        const auto& LeaderboardsData{ doc["Leaderboards"] };
        for (const auto& lbData : LeaderboardsData.GetArray())
        {
            //"Leaderboards":[{"ID":"2","Mem":"STA:0xfe10=h0000_0xhf601=h0c_d0xhf601!=h0c_0xfff0=0_0xfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 1","Description":"Complete this act in the fastest time!"},
            RA_Leaderboard lb{ lbData["ID"].GetUint() };

            lb.SetTitle(lbData["Title"].GetString());
            lb.SetDescription(lbData["Description"].GetString());
            lb.ParseFromString(lbData["Mem"].GetString(), lbData["Format"].GetString());

            ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>()
                .AddLeaderboard(std::move(lb));
        }

        // calculate the total number of points for the core set, and pre-fetch badge images
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        auto nTotalPoints{0U};
        for (size_t i = 0; i < g_pCoreAchievements->NumAchievements(); ++i)
        {
            const auto& ach{ g_pCoreAchievements->GetAchievement(i) };
            pImageRepository.FetchImage(ra::ui::ImageType::Badge, ach.BadgeImageURI());
            nTotalPoints += ach.Points();
        }

        if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

            //	Loaded OK: post a request for unlocks
            PostArgs args;
            args['u'] = RAUsers::LocalUser().Username();
            args['t'] = RAUsers::LocalUser().Token();
            args['g'] = std::to_string(nGameID);
            args['h'] = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? "1" : "0";

            RAWeb::CreateThreadedHTTPRequest(RequestUnlocks, args);
            std::string sTitle{ "Loaded " };
            sTitle += ra::Narrow(pGameContext.GameTitle());

            std::string sSubTitle;
            {
                std::ostringstream oss;
                oss << g_pCoreAchievements->NumAchievements() << " achievements, Total Score " << nTotalPoints;
                sSubTitle = oss.str();
            }

            if (doc.HasMember("ImageIcon"))
            {
                // value will be "/Images/001234.png" - extract the "001234"
                std::string sImageName = doc["ImageIcon"].GetString();
                auto nIndex = sImageName.find_last_of('/');
                if (nIndex != std::string::npos)
                    sImageName.erase(0, nIndex + 1);
                nIndex = sImageName.find_last_of('.');
                if (nIndex != std::string::npos)
                    sImageName.erase(nIndex);

                g_PopupWindows.AchievementPopups().AddMessage({ sTitle, sSubTitle, PopupMessageType::Info, ra::ui::ImageType::Icon, sImageName });
            }
            else
            {
                g_PopupWindows.AchievementPopups().AddMessage({ sTitle, sSubTitle });
            }
        }

        return true;
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

BOOL AchievementSet::HasUnsavedChanges()
{
    for (size_t i = 0; i < NumAchievements(); ++i)
    {
        if (m_Achievements[i].Modified())
            return TRUE;
    }

    return FALSE;
}
