#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_md5factory.h"
#include "RA_StringUtils.h"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\CodeNotesModel.hh"
#include "data\models\LocalBadgesModel.hh"
#include "data\models\RichPresenceModel.hh"

#include "services\AchievementRuntime.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ILocalStorage.hh"
#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"
#include "services\impl\StringTextReader.hh"

#include "ui\ImageReference.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\ScoreboardViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace data {
namespace context {

static bool ValidateConsole(int nServerConsoleId)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    if (ra::etoi(pConsoleContext.Id()) != nServerConsoleId)
    {
        switch (nServerConsoleId)
        {
            case RC_CONSOLE_GAMEBOY:
                // GB memory map is just a subset of GBC, and GBC runs GB games, so allow loading a
                // GB game in GBC context.
                if (pConsoleContext.Id() == ra::etoi(ConsoleID::GBC))
                    return true;

                break;

            case RC_CONSOLE_NINTENDO_DS:
                // DS memory map is just a subset of DSi, and DSi runs DS games, so allow loading a
                // DS game in DSi context.
                if (pConsoleContext.Id() == ra::etoi(ConsoleID::DSi))
                    return true;

                break;

            case RC_CONSOLE_EVENTS:
                // Hashes for Events are generated by modifying an existing ROM in a very specific manner
                // To generate the correct hash, Events are loaded through the console associated to the
                // unmodified ROM and therefore will never match the console context.
                return true;

            default:
                break;
        }

        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            L"Identified game does not match expected console.",
            ra::StringPrintf(
                L"The game being loaded is associated to the %s console, but the emulator has initialized "
                "the %s console. This is not allowed as the memory maps may not be compatible between "
                "consoles.",
                rc_console_name(nServerConsoleId), pConsoleContext.Name()));

        return false;
    }

    return true;
}

void GameContext::LoadGame(unsigned int nGameId, const std::string& sGameHash, Mode nMode)
{
    OnBeforeActiveGameChanged();

    // remove the current asset from the asset editor
    auto& vmWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    vmWindowManager.AssetEditor.LoadAsset(nullptr, true);

    // reset the runtime
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    pRuntime.ResetRuntime();

    // reset the GameContext
    m_nMode = nMode;
    m_sGameTitle.clear();
    m_vAssets.ResetLocalId();

    m_vAssets.BeginUpdate();

    m_vAssets.Clear();

    // if not loading a game, finish up and return
    if (nGameId == 0)
    {
        m_vAssets.EndUpdate();

        m_sGameHash.clear();

        if (m_nGameId != 0)
        {
            m_nGameId = 0;
            OnActiveGameChanged();
        }
        return;
    }

    // start the load process
    BeginLoad();
    m_nGameId = nGameId;

    // create a model for managing badges
    auto pLocalBadges = std::make_unique<ra::data::models::LocalBadgesModel>();
    pLocalBadges->CreateServerCheckpoint();
    pLocalBadges->CreateLocalCheckpoint();
    m_vAssets.Append(std::move(pLocalBadges));

    // capture the old rich presence data so we can tell if it changed on the server
    std::string sOldRichPresence = "[NONE]";
    {
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, std::to_wstring(nGameId));
        if (pData != nullptr)
        {
            rapidjson::Document pDocument;
            if (LoadDocument(pDocument, *pData) && pDocument.HasMember("RichPresencePatch") &&
                pDocument["RichPresencePatch"].IsString())
                sOldRichPresence = pDocument["RichPresencePatch"].GetString();
        }
    }

    // enable spectator mode to prevent unlocks when testing compatibility
    rc_client_set_spectator_mode_enabled(pRuntime.GetClient(), nMode == Mode::CompatibilityTest);

    const bool bWasPaused = pRuntime.IsPaused();
    pRuntime.SetPaused(true);

    // download the game data
    struct LoadGameUserData
    {
        bool bWasPaused = false;
        std::string sOldRichPresence;
    }* pLoadGameUserData;
    pLoadGameUserData = new LoadGameUserData;
    pLoadGameUserData->bWasPaused = bWasPaused;
    pLoadGameUserData->sOldRichPresence = std::move(sOldRichPresence);

    pRuntime.BeginLoadGame(sGameHash, nGameId,
        [](int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata) {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLoadGameUserData = static_cast<struct LoadGameUserData*>(pUserdata);
            pGameContext.FinishLoadGame(nResult, sErrorMessage, pLoadGameUserData->bWasPaused,
                                        pLoadGameUserData->sOldRichPresence);
            delete pLoadGameUserData;
        },
        pLoadGameUserData);
}

void GameContext::FinishLoadGame(int nResult, const char* sErrorMessage, bool bWasPaused, const std::string& sOldRichPresence)
{
    if (nResult != RC_OK)
    {
        m_nGameId = 0;

        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to load game data",
                                                                    ra::Widen(sErrorMessage));
    }
    else
    {
        BeginLoad();
        auto pCodeNotes = std::make_unique<ra::data::models::CodeNotesModel>();
        pCodeNotes->Refresh(
            m_nGameId,
            [this](ra::ByteAddress nAddress, const std::wstring& sNewNote) {
                OnCodeNoteChanged(nAddress, sNewNote);
            },
            [this]() { EndLoad(); });
        m_vAssets.Append(std::move(pCodeNotes));

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        auto* pGame = rc_client_get_game_info(pClient.GetClient());
        if (pGame == nullptr || pGame->id == 0)
        {
            // invalid hash
        }
        else if (!ValidateConsole(pGame->console_id))
        {
            m_nGameId = 0;
            m_sGameTitle.clear();
        }
        else
        {
            rc_client_user_game_summary_t pSummary;
            rc_client_get_user_game_summary(pClient.GetClient(), &pSummary);

            // show "game loaded" popup
            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
            std::wstring sDescription = ra::StringPrintf(L"%u achievements, %u points",
                                                            pSummary.num_core_achievements, pSummary.points_core);
            if (pSummary.num_unsupported_achievements)
                sDescription += ra::StringPrintf(L" (%u unsupported)", pSummary.num_unsupported_achievements);

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                ra::StringPrintf(L"Loaded %s", pGame->title), sDescription,
                ra::StringPrintf(L"You have earned %u achievements", pSummary.num_unlocked_achievements),
                ra::ui::ImageType::Icon, pGame->badge_name);

            // merge local assets
            std::vector<ra::data::models::AssetModelBase*> vEmptyAssetsList;
            m_vAssets.ReloadAssets(vEmptyAssetsList);
        }
    }

    if (!bWasPaused)
    {
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().SetPaused(false);
#ifndef RA_UTEST
        auto& pAssetList = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().AssetList;
        pAssetList.SetProcessingActive(true);
#endif
    }

    // activate rich presence (or remove if not defined)
    auto* pRichPresence = m_vAssets.FindRichPresence();
    if (pRichPresence)
    {
        // if the server value differs from the local value, the model will appear as Unpublished
        if (pRichPresence->GetChanges() != ra::data::models::AssetChanges::None)
        {
            // populate another model with the old script so the string gets normalized correctly
            ra::data::models::RichPresenceModel pOldRichPresenceModel;
            pOldRichPresenceModel.SetScript(sOldRichPresence);

            // if the old value matches the current value, then the value on the server changed and
            // there are no local modifications. revert to the server state.
            if (pRichPresence->GetScript() == pOldRichPresenceModel.GetScript())
                pRichPresence->RestoreServerCheckpoint();
        }

        if (pRichPresence->GetScript().empty() && pRichPresence->GetChanges() == ra::data::models::AssetChanges::None)
        {
            const auto nIndex = m_vAssets.FindItemIndex(ra::data::models::AssetModelBase::TypeProperty,
                                                        ra::etoi(ra::data::models::AssetType::RichPresence));
            m_vAssets.RemoveAt(nIndex);
        }
        else
        {
            pRichPresence->Activate();
        }
    }

    // modified assets should start in the inactive state
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pAsset = m_vAssets.GetItemAt(nIndex);
        if (pAsset != nullptr && pAsset->GetChanges() != ra::data::models::AssetChanges::None)
        {
            if (pAsset->IsActive())
                pAsset->SetState(ra::data::models::AssetState::Inactive);
        }
    }

    // finish up
    m_vAssets.EndUpdate();

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    pRuntime.SyncAssets();

    EndLoad();
    OnActiveGameChanged();
}

void GameContext::InitializeFromAchievementRuntime(const std::map<uint32_t, std::string> mAchievementDefinitions,
                                                   const std::map<uint32_t, std::string> mLeaderboardDefinitions)
{
    const auto* pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();
    const auto* pGame = rc_client_get_game_info(pClient);
    m_sGameTitle = ra::Widen(pGame->title);

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif

    for (auto* pSubset = pClient->game->subsets; pSubset; pSubset = pSubset->next)
    {
        // achievements
        auto* pAchievementData = pSubset->achievements;
        if (pAchievementData != nullptr)
        {
            const auto* pAchievementStop = pAchievementData + pSubset->public_.num_achievements;
            for (; pAchievementData < pAchievementStop; ++pAchievementData)
            {
                // if the server has provided an unexpected category (usually 0), ignore it.
                ra::data::models::AssetCategory nCategory = ra::data::models::AssetCategory::None;
                switch (pAchievementData->public_.category)
                {
                    case RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE:
                        nCategory = ra::data::models::AssetCategory::Core;
                        break;

                    case RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL:
                        nCategory = ra::data::models::AssetCategory::Unofficial;

                        // all unofficial achievements should start inactive.
                        // rc_client automatically activates them.
                        pAchievementData->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

                        if (pAchievementData->trigger)
                            pAchievementData->trigger->state = RC_TRIGGER_STATE_INACTIVE;
                        break;

                    default:
                        continue;
                }

                auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();

                const auto sDefinition = mAchievementDefinitions.find(pAchievementData->public_.id);
                if (sDefinition != mAchievementDefinitions.end())
                    vmAchievement->Attach(*pAchievementData, nCategory, sDefinition->second);
                else
                    vmAchievement->Attach(*pAchievementData, nCategory, "");

                m_vAssets.Append(std::move(vmAchievement));

#ifndef RA_UTEST
                // prefetch the achievement image
                pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievementData->public_.badge_name);
#endif
            }
        }

        // leaderboards
        auto* pLeaderboardData = pSubset->leaderboards;
        if (pLeaderboardData != nullptr)
        {
            const auto* pLeaderboardStop = pLeaderboardData + pSubset->public_.num_leaderboards;
            for (; pLeaderboardData < pLeaderboardStop; ++pLeaderboardData)
            {
                auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();

                const auto nCategory = ra::data::models::AssetCategory::Core; // all published leaderboards are core

                const auto sDefinition = mLeaderboardDefinitions.find(pLeaderboardData->public_.id);
                if (sDefinition != mLeaderboardDefinitions.end())
                    vmLeaderboard->Attach(*pLeaderboardData, nCategory, sDefinition->second);
                else
                    vmLeaderboard->Attach(*pLeaderboardData, nCategory, "");

                m_vAssets.Append(std::move(vmLeaderboard));
            }
        }
    }
}

void GameContext::OnBeforeActiveGameChanged()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnBeforeActiveGameChanged();
    }
}

void GameContext::OnActiveGameChanged()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnActiveGameChanged();
    }
}

void GameContext::BeginLoad()
{
    if (m_nLoadCount.fetch_add(1) == 0)
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnBeginGameLoad();
        }
    }
}

void GameContext::EndLoad()
{
    if (m_nLoadCount.fetch_sub(1) == 1)
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnEndGameLoad();
        }
    }
}

void GameContext::DoFrame()
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            pItem->DoFrame();
    }
}

void GameContext::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnCodeNoteChanged(nAddress, sNewNote);
        }
    }
}

} // namespace context
} // namespace data
} // namespace ra
