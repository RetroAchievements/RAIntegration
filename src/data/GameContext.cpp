#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Dlg_Achievement.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "api\AwardAchievement.hh"
#include "api\FetchGameData.hh"
#include "api\FetchUserUnlocks.hh"

#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ILocalStorage.hh"
#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"
#include "services\impl\StringTextReader.hh"

#include "ui\ImageReference.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

extern bool g_bRAMTamperedWith;

namespace ra {
namespace data {

static void CopyAchievementData(Achievement& pAchievement,
                                const ra::api::FetchGameData::Response::Achievement& pAchievementData)
{
    pAchievement.SetTitle(pAchievementData.Title);
    pAchievement.SetDescription(pAchievementData.Description);
    pAchievement.SetPoints(pAchievementData.Points);
    pAchievement.SetAuthor(pAchievementData.Author);
    pAchievement.SetCreatedDate(pAchievementData.Created);
    pAchievement.SetModifiedDate(pAchievementData.Updated);
    pAchievement.SetBadgeImage(pAchievementData.BadgeName);
    pAchievement.ParseTrigger(pAchievementData.Definition.c_str());
}

void GameContext::LoadGame(unsigned int nGameId)
{
    m_nGameId = nGameId;
    m_sGameTitle.clear();
    m_pRichPresenceInterpreter.reset();
    m_nNextLocalId = GameContext::FirstLocalId;

    if (!m_vAchievements.empty())
    {
        for (auto& pAchievement : m_vAchievements)
            pAchievement->SetActive(false);
        m_vAchievements.clear();

#ifndef RA_UTEST
        // temporary code for compatibility until global collections are eliminated
        g_pCoreAchievements->Clear();
        g_pLocalAchievements->Clear();
        g_pUnofficialAchievements->Clear();
#endif
    }

#ifndef RA_UTEST
    auto& pLeaderboardManager = ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>();
    pLeaderboardManager.Clear();
#endif

    if (nGameId == 0)
    {
        m_sGameHash.clear();
        return;
    }

    ra::api::FetchGameData::Request request;
    request.GameId = nGameId;

    const auto response = request.Call();
    if (response.Failed())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download game data",
                                                                  ra::Widen(response.ErrorMessage));
        return;
    }

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    // game properties
    m_sGameTitle = response.Title;

    // rich presence
    if (!response.RichPresence.empty())
    {
        ra::services::impl::StringTextReader pRichPresenceTextReader(response.RichPresence);

        m_pRichPresenceInterpreter.reset(new RA_RichPresenceInterpreter());
        if (!m_pRichPresenceInterpreter->Load(pRichPresenceTextReader))
            m_pRichPresenceInterpreter.reset();

        // TODO: this file is written so devs can modify the rich presence without having to restart
        // the game. if the user doesn't have dev permission, there's no reason to write it.
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pRich = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence, std::to_wstring(nGameId));
        pRich->Write(response.RichPresence);
    }

    // achievements
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    const bool bWasPaused = pRuntime.IsPaused();
    pRuntime.SetPaused(true);

    unsigned int nNumCoreAchievements = 0;
    unsigned int nTotalCoreAchievementPoints = 0;
    for (const auto& pAchievementData : response.Achievements)
    {
        auto& pAchievement = NewAchievement(ra::itoe<AchievementSet::Type>(pAchievementData.CategoryId));
        pAchievement.SetID(pAchievementData.Id);
        CopyAchievementData(pAchievement, pAchievementData);

#ifndef RA_UTEST
        // prefetch the achievement image
        pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievementData.BadgeName);
#endif

        if (pAchievementData.CategoryId == ra::to_unsigned(ra::etoi(AchievementSet::Type::Core)))
        {
            ++nNumCoreAchievements;
            nTotalCoreAchievementPoints += pAchievementData.Points;
        }
    }

    // leaderboards
#ifndef RA_UTEST
    for (const auto& pLeaderboardData : response.Leaderboards)
    {
        RA_Leaderboard leaderboard(pLeaderboardData.Id);
        leaderboard.SetTitle(pLeaderboardData.Title);
        leaderboard.SetDescription(pLeaderboardData.Description);
        leaderboard.ParseFromString(pLeaderboardData.Definition.c_str(), pLeaderboardData.Format.c_str());

        pLeaderboardManager.AddLeaderboard(std::move(leaderboard));
    }
#endif

    // merge local achievements
    m_nNextLocalId = GameContext::FirstLocalId;
    MergeLocalAchievements();

    // get user unlocks asynchronously
    ra::api::FetchUserUnlocks::Request request2;
    request2.GameId = nGameId;
    request2.Hardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    request2.CallAsync([this, bWasPaused](const ra::api::FetchUserUnlocks::Response& response)
    {
        std::set<unsigned int> vLockedAchievements;
        for (auto& pAchievement : m_vAchievements)
        {
            vLockedAchievements.insert(pAchievement->ID());
            pAchievement->SetActive(false);
        }

        for (const auto nUnlockedAchievement : response.UnlockedAchievements)
            vLockedAchievements.erase(nUnlockedAchievement);

#ifndef RA_UTEST
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif
        // activate any core achievements the player hasn't earned and pre-fetch the locked image
        for (auto nAchievementId : vLockedAchievements)
        {
            auto* pAchievement = FindAchievement(nAchievementId);
            if (pAchievement && pAchievement->Category() == ra::etoi(AchievementSet::Type::Core))
            {
                pAchievement->SetActive(true);

#ifndef RA_UTEST
                if (!pAchievement->BadgeImageURI().empty())
                    pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI() + "_lock");
#endif
            }
        }

        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().SetPaused(bWasPaused);

#ifndef RA_UTEST
        for (int nIndex = 0; nIndex < ra::to_signed(g_pActiveAchievements->NumAchievements()); ++nIndex)
        {
            const Achievement& Ach = g_pActiveAchievements->GetAchievement(nIndex);
            if (Ach.Active())
                g_AchievementsDialog.OnEditData(nIndex, Dlg_Achievements::Column::Achieved, "No");
        }
#endif
    });

    // show "game loaded" popup
    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        ra::StringPrintf(L"Loaded %s", response.Title),
        ra::StringPrintf(L"%u achievements, Total Score %u", nNumCoreAchievements, nTotalCoreAchievementPoints),
        ra::ui::ImageType::Icon, response.ImageIcon);
}

void GameContext::MergeLocalAchievements()
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::UserAchievements, std::to_wstring(m_nGameId));
    if (pData == nullptr)
        return;

    std::string sLine;
    pData->GetLine(sLine); // version used to create the file
    pData->GetLine(sLine); // game title

    while (pData->GetLine(sLine))
    {
        // achievement lines start with the achievement id
        if (sLine.empty() || !isdigit(sLine.front()))
            continue;

        auto pAchievement = std::make_unique<Achievement>();
        pAchievement->ParseLine(sLine.c_str());

        Achievement* pExistingAchievement = nullptr;
        if (pAchievement->ID() != 0)
            pExistingAchievement = FindAchievement(pAchievement->ID());

        if (pExistingAchievement)
        {
            // merge local data into existing achievement
            if (pExistingAchievement->Title() != pAchievement->Title())
            {
                pExistingAchievement->SetTitle(pAchievement->Title());
                pExistingAchievement->SetModified(true);
            }

            if (pExistingAchievement->Description() != pAchievement->Description())
            {
                pExistingAchievement->SetDescription(pAchievement->Description());
                pExistingAchievement->SetModified(true);
            }

            if (pExistingAchievement->Points() != pAchievement->Points())
            {
                pExistingAchievement->SetPoints(pAchievement->Points());
                pExistingAchievement->SetModified(true);
            }

            if (pExistingAchievement->BadgeImageURI() != pAchievement->BadgeImageURI())
            {
                pExistingAchievement->SetBadgeImage(pAchievement->BadgeImageURI());
                pExistingAchievement->SetModified(true);
            }

            pExistingAchievement->SetModifiedDate(pAchievement->ModifiedDate());

            auto sExistingTrigger = pExistingAchievement->CreateMemString();
            auto sNewTrigger = pAchievement->CreateMemString();
            if (sExistingTrigger != sNewTrigger)
            {
                pExistingAchievement->ParseTrigger(sNewTrigger.c_str());
                pExistingAchievement->SetModified(true);
            }
        }
        else
        {
#ifndef RA_UTEST
            g_pLocalAchievements->AddAchievement(pAchievement.get());
#endif
            if (pAchievement->ID() >= m_nNextLocalId)
                m_nNextLocalId = pAchievement->ID() + 1;

            // append local achievement
            pAchievement->SetCategory(ra::etoi(AchievementSet::Type::Local));
            m_vAchievements.emplace_back(std::move(pAchievement));
        }
    }

    for (auto& pAchievement : m_vAchievements)
    {
        if (pAchievement->ID() == 0)
            pAchievement->SetID(m_nNextLocalId++);
    }
}

Achievement& GameContext::NewAchievement(AchievementSet::Type nType)
{
    Achievement& pAchievement = *m_vAchievements.emplace_back(std::make_unique<Achievement>());
    pAchievement.SetCategory(ra::etoi(nType));
    pAchievement.SetID(m_nNextLocalId++);

#ifndef RA_UTEST
    // temporary code for compatibility until global collections are eliminated
    switch (nType)
    {
        case AchievementSet::Type::Core:
            g_pCoreAchievements->AddAchievement(&pAchievement);
            break;

        default:
        case AchievementSet::Type::Unofficial:
            g_pUnofficialAchievements->AddAchievement(&pAchievement);
            break;

        case AchievementSet::Type::Local:
            g_pLocalAchievements->AddAchievement(&pAchievement);
            break;
    }
#endif

    return pAchievement;
}

void GameContext::AwardAchievement(unsigned int nAchievementId) const
{
    auto* pAchievement = FindAchievement(nAchievementId);
    if (pAchievement == nullptr)
        return;

    bool bSubmit = false;
    bool bIsError = false;
    std::wstring sHeader;
    std::wstring sMessage = ra::StringPrintf(L"%s (%u)", pAchievement->Title(), pAchievement->Points());

    switch (ra::itoe<AchievementSet::Type>(pAchievement->Category()))
    {
        case AchievementSet::Type::Local:
            sHeader = L"Local Achievement Unlocked";
            bSubmit = false;
            break;

        case AchievementSet::Type::Unofficial:
            sHeader = L"Unofficial Achievement Unlocked";
            bSubmit = false;
            break;

        default:
            sHeader = L"Achievement Unlocked";
            bSubmit = true;
            break;
    }

    if (pAchievement->Modified())
    {
        sHeader.insert(0, L"Modified ");
        if (ActiveAchievementType() != AchievementSet::Type::Local)
            sHeader.insert(sHeader.length() - 8, L"NOT ");
        bIsError = true;
        bSubmit = false;
    }

#ifndef RA_UTEST
    if (bSubmit && g_bRAMTamperedWith)
    {
        sHeader = L"RAM Tampered With! Achievement NOT Unlocked";
        bIsError = true;
        bSubmit = false;
    }
#endif

    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(
        bIsError ? L"Overlay\\acherror.wav" : L"Overlay\\unlock.wav");

    const auto nPopupId = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        sHeader, sMessage, ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());

    pAchievement->SetActive(false);

    if (!bSubmit)
        return;

    ra::api::AwardAchievement::Request request;
    request.AchievementId = nAchievementId;
    request.Hardcore = _RA_HardcoreModeIsActive();
    request.GameHash = GameHash();
    request.CallAsyncWithRetry([nPopupId, nAchievementId](const ra::api::AwardAchievement::Response& response)
    {
        if (response.Succeeded())
        {
            // success! update the player's score
            ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().SetScore(response.NewPlayerScore);
        }
        else if (ra::StringStartsWith(response.ErrorMessage, "User already has "))
        {
            // if server responds with "Error: User already has hardcore and regular achievements awarded." or
            // "Error: User already has this achievement awarded.", just ignore the error. There's no reason to
            // notify the user that the unlock failed because it had happened previously.
        }
        else
        {
            // an error occurred, update the popup to display the error message
            auto* pPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().GetMessage(nPopupId);
            if (pPopup != nullptr)
            {
                std::wstring sNewTitle = ra::StringPrintf(L"%s (%s)", pPopup->GetTitle(),
                    response.ErrorMessage.empty() ? "Error" : response.ErrorMessage);

                pPopup->SetTitle(sNewTitle);
                pPopup->RebuildRenderImage();
            }
            else
            {
                const auto& pGameContext = ra::services::ServiceLocator::Get<GameContext>();
                const auto* pAchievement = pGameContext.FindAchievement(nAchievementId);
                if (pAchievement != nullptr)
                {
                    auto sHeader = ra::BuildWString("Error unlocking ", pAchievement->Title());
                    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                        sHeader, ra::Widen(response.ErrorMessage), ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());
                }
                else
                {
                    auto sHeader = ra::BuildWString("Error unlocking achievement ", nAchievementId);
                    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                        sHeader, ra::Widen(response.ErrorMessage));
                }
            }
        }
    });
}

void GameContext::ReloadAchievements(int nCategory)
{
    if (nCategory == ra::etoi(AchievementSet::Type::Local))
    {
        auto pIter = m_vAchievements.begin();
        while (pIter != m_vAchievements.end())
        {
            if ((*pIter)->Category() == nCategory)
            {
#ifndef RA_UTEST
                g_pLocalAchievements->RemoveAchievement(pIter->get());
#endif
                pIter = m_vAchievements.erase(pIter);
            }
            else
            {
                ++pIter;
            }
        }

        MergeLocalAchievements();
    }
    else
    {
        // TODO
    }
}

bool GameContext::ReloadAchievement(unsigned int nAchievementId)
{
    auto pIter = m_vAchievements.begin();
    while (pIter != m_vAchievements.end())
    {
        if ((*pIter)->ID() == nAchievementId)
        {
            if (ReloadAchievement(*pIter->get()))
                return true;

            m_vAchievements.erase(pIter);
            break;
        }

        ++pIter;
    }

    return false;
}

bool GameContext::ReloadAchievement(Achievement& pAchievement)
{
    if (pAchievement.Category() == ra::etoi(AchievementSet::Type::Local))
    {
        // TODO
    }
    else
    {
        ra::api::FetchGameData::Request request;
        request.GameId = m_nGameId;

        const auto response = request.Call();
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download game data",
                                                                      ra::Widen(response.ErrorMessage));
            return true; // prevent deleting achievement if server call failed
        }

        for (const auto& pAchievementData : response.Achievements)
        {
            if (pAchievementData.Id != pAchievement.ID())
                continue;

            CopyAchievementData(pAchievement, pAchievementData);
            pAchievement.SetDirtyFlag(Achievement::DirtyFlags::All);
            pAchievement.SetModified(false);
            return true;
        }
    }

    return false;
}

bool GameContext::HasRichPresence() const noexcept
{
    return (m_pRichPresenceInterpreter != nullptr && m_pRichPresenceInterpreter->Enabled());
}

std::wstring GameContext::GetRichPresenceDisplayString() const
{
    if (m_pRichPresenceInterpreter == nullptr)
        return std::wstring(L"No Rich Presence defined.");

    return ra::Widen(m_pRichPresenceInterpreter->GetRichPresenceString());
}

void GameContext::ReloadRichPresenceScript()
{
    m_pRichPresenceInterpreter.reset(new RA_RichPresenceInterpreter());
    if (!m_pRichPresenceInterpreter->Load())
        m_pRichPresenceInterpreter.reset();
}

} // namespace data
} // namespace ra
