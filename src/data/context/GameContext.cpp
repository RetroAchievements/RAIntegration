#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_md5factory.h"
#include "RA_StringUtils.h"

#include "api\AwardAchievement.hh"
#include "api\FetchGameData.hh"
#include "api\FetchUserUnlocks.hh"
#include "api\SubmitLeaderboardEntry.hh"

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
#include "services\RcheevosClient.hh"
#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"
#include "services\impl\StringTextReader.hh"

#include "ui\ImageReference.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\ScoreboardViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\src\rcheevos\rc_client_internal.h>

namespace ra {
namespace data {
namespace context {

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
            if (LoadDocument(pDocument, *pData) && pDocument.HasMember("RichPresencePatch") && pDocument["RichPresencePatch"].IsString())
                sOldRichPresence = pDocument["RichPresencePatch"].GetString();
        }
    }

    // download the game data
#if 1
    ra::services::RcheevosClient::Synchronizer pSynchronizer;

    auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
    pClient.BeginLoadGame(sGameHash, nGameId,
        [](int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata) {
            auto* pSynchronizer = static_cast<ra::services::RcheevosClient::Synchronizer*>(pUserdata);
            Expects(pSynchronizer != nullptr);

            if (nResult != RC_OK)
            {
                auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
                pGameContext.m_nGameId = 0;

                pSynchronizer->SetErrorMessage(sErrorMessage);
            }

            pSynchronizer->Notify();
        },
        &pSynchronizer);

    // TODO: does this have to be synchronous? function returns void
    pSynchronizer.Wait();

    if (!pSynchronizer.GetErrorMessage().empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to load game data",
                                                                  ra::Widen(pSynchronizer.GetErrorMessage()));
    }
    else
    {
        auto* pGame = rc_client_get_game_info(pClient.GetClient());
        if (pGame != nullptr && pGame->id != 0)
        {
            rc_client_user_game_summary_t pSummary;
            rc_client_get_user_game_summary(pClient.GetClient(), &pSummary);

            // show "game loaded" popup
            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
            std::wstring sDescription =
                ra::StringPrintf(L"%u achievements, %u points", pSummary.num_core_achievements, pSummary.points_core);
            if (pSummary.num_unsupported_achievements)
                sDescription += ra::StringPrintf(L" (%u unsupported)", pSummary.num_unsupported_achievements);

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                ra::StringPrintf(L"Loaded %s", pGame->title), sDescription,
                ra::StringPrintf(L"You have earned %u achievements", pSummary.num_unlocked_achievements),
                ra::ui::ImageType::Icon, m_sGameImage);
        }
    }
#else

    ra::api::FetchGameData::Request request;
    request.GameId = nGameId;

    const auto response = request.Call();
    if (response.Failed())
    {
        m_vAssets.EndUpdate();
        m_nGameId = 0;

        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download game data",
                                                                  ra::Widen(response.ErrorMessage));
        EndLoad();
        return;
    }

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    const auto nServerConsoleId = ra::itoe<ConsoleID>(response.ConsoleId);
    if (nServerConsoleId != pConsoleContext.Id())
    {
        if (nServerConsoleId == ConsoleID::GB && pConsoleContext.Id() == ConsoleID::GBC)
        {
            // GB memory map is just a subset of GBC, and GBC runs GB games, so allow loading a
            // GB game in GBC context.
        }
        else if (nServerConsoleId == ConsoleID::DS && pConsoleContext.Id() == ConsoleID::DSi)
        {
            // DS memory map is just a subset of DSi, and DSi runs DS games, so allow loading a
            // DS game in DSi context.
        }
        else if (ra::etoi(nServerConsoleId) == RC_CONSOLE_EVENTS)
        {
            // Hashes for Events are generated by modifying an existing ROM in a very specific manner
            // To generate the correct hash, Events are loaded through the console associated to the
            // unmodified ROM and therefore will never match the console context.
        }
        else
        {
            m_vAssets.EndUpdate();
            m_nGameId = 0;

            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Identified game does not match expected console.",
                ra::StringPrintf(L"The game being loaded is associated to the %s console, but the emulator has initialized "
                    "the %s console. This is not allowed as the memory maps may not be compatible between consoles.",
                    rc_console_name(response.ConsoleId), pConsoleContext.Name()));

            EndLoad();
            return;
        }
    }

    // start fetching the code notes
    {
        BeginLoad();
        auto pCodeNotes = std::make_unique<ra::data::models::CodeNotesModel>();
        pCodeNotes->Refresh(m_nGameId,
            [this](ra::ByteAddress nAddress, const std::wstring& sNewNote) {
                OnCodeNoteChanged(nAddress, sNewNote);
            },
            [this]() { EndLoad(); });
        m_vAssets.Append(std::move(pCodeNotes));
    }

    // game properties
    m_sGameTitle = response.Title;
    m_sGameImage = response.ImageIcon;

    // rich presence
    {
        auto pRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
        pRichPresence->SetScript(response.RichPresence);
        pRichPresence->CreateServerCheckpoint();
        pRichPresence->CreateLocalCheckpoint();
        m_vAssets.Append(std::move(pRichPresence));
    }

    // achievements
    const bool bWasPaused = pRuntime.IsPaused();
    pRuntime.SetPaused(true);

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif

    unsigned int nNumCoreAchievements = 0;
    unsigned int nTotalCoreAchievementPoints = 0;
    for (const auto& pAchievementData : response.Achievements)
    {
        // if the server has provided an unexpected category (usually 0), ignore it.
        const auto nCategory = ra::itoe<ra::data::models::AssetCategory>(pAchievementData.CategoryId);
        if (nCategory != ra::data::models::AssetCategory::Core && nCategory != ra::data::models::AssetCategory::Unofficial)
            continue;

        auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
        vmAchievement->SetID(pAchievementData.Id);
        vmAchievement->SetName(ra::Widen(pAchievementData.Title));
        vmAchievement->SetDescription(ra::Widen(pAchievementData.Description));
        vmAchievement->SetCategory(nCategory);
        vmAchievement->SetPoints(pAchievementData.Points);
        vmAchievement->SetAuthor(ra::Widen(pAchievementData.Author));
        vmAchievement->SetBadge(ra::Widen(pAchievementData.BadgeName));
        vmAchievement->SetTrigger(pAchievementData.Definition);
        vmAchievement->SetCreationTime(pAchievementData.Created);
        vmAchievement->SetUpdatedTime(pAchievementData.Updated);
        vmAchievement->CreateServerCheckpoint();
        vmAchievement->CreateLocalCheckpoint();
        m_vAssets.Append(std::move(vmAchievement));

#ifndef RA_UTEST
        // prefetch the achievement image
        pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievementData.BadgeName);
#endif

        if (nCategory == ra::data::models::AssetCategory::Core)
        {
            ++nNumCoreAchievements;
            nTotalCoreAchievementPoints += pAchievementData.Points;
        }
    }

    // leaderboards
    for (const auto& pLeaderboardData : response.Leaderboards)
    {
        auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
        vmLeaderboard->SetID(pLeaderboardData.Id);
        vmLeaderboard->SetName(ra::Widen(pLeaderboardData.Title));
        vmLeaderboard->SetDescription(ra::Widen(pLeaderboardData.Description));
        vmLeaderboard->SetCategory(ra::data::models::AssetCategory::Core);
        vmLeaderboard->SetValueFormat(ra::itoe<ValueFormat>(pLeaderboardData.Format));
        vmLeaderboard->SetLowerIsBetter(pLeaderboardData.LowerIsBetter);
        vmLeaderboard->SetHidden(pLeaderboardData.Hidden);
        vmLeaderboard->SetDefinition(pLeaderboardData.Definition);
        vmLeaderboard->CreateServerCheckpoint();
        vmLeaderboard->CreateLocalCheckpoint();
        m_vAssets.Append(std::move(vmLeaderboard));
    }

    ActivateLeaderboards();

    // merge local assets
    std::vector<ra::data::models::AssetModelBase*> vEmptyAssetsList;
    m_vAssets.ReloadAssets(vEmptyAssetsList);

#ifndef RA_UTEST
    DoFrame();
#endif

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

    // show "game loaded" popup
    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
    const auto nPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        ra::StringPrintf(L"Loaded %s", response.Title),
        ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
        ra::ui::ImageType::Icon, m_sGameImage);

    // get user unlocks asynchronously
    RefreshUnlocks(!bWasPaused, nPopup);
#endif

    // finish up
    m_vAssets.EndUpdate();

    EndLoad();
    OnActiveGameChanged();
}

void GameContext::InitializeFromRcheevosClient()
{
    const auto* pClient = ra::services::ServiceLocator::Get<ra::services::RcheevosClient>().GetClient();
    const auto* pGame = rc_client_get_game_info(pClient);
    m_sGameTitle = ra::Widen(pGame->title);

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif

    for (auto* pSubset = pClient->game->subsets; pSubset; pSubset = pSubset->next)
    {
        // achievements
        auto* pAchievementData = pSubset->achievements;
        auto* pAchievementStop = pAchievementData + pSubset->public_.num_achievements;
        for (; pAchievementData < pAchievementStop; ++pAchievementData)
        {
            // if the server has provided an unexpected category (usually 0), ignore it.
            ra::data::models::AssetCategory nCategory;
            switch (pAchievementData->public_.category)
            {
                case RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE:
                    nCategory = ra::data::models::AssetCategory::Core;
                    break;
                case RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL:
                    nCategory = ra::data::models::AssetCategory::Unofficial;
                    break;
                default:
                    continue;
            }

            auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
            vmAchievement->SetID(pAchievementData->public_.id);
            vmAchievement->SetName(ra::Widen(pAchievementData->public_.title));
            vmAchievement->SetDescription(ra::Widen(pAchievementData->public_.description));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(pAchievementData->public_.points);
            //vmAchievement->SetAuthor(ra::Widen(pAchievementData->author));
            vmAchievement->SetBadge(ra::Widen(pAchievementData->public_.badge_name));
            //vmAchievement->SetTrigger(pAchievementData->trigger);
            //vmAchievement->SetCreationTime(pAchievementData->created);
            //vmAchievement->SetUpdatedTime(pAchievementData->updated);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();

            switch (pAchievementData->public_.state)
            {
                case RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE:
                    vmAchievement->SetState(ra::data::models::AssetState::Active);
                    break;
                case RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE:
                    vmAchievement->SetState(ra::data::models::AssetState::Inactive);
                    break;
                case RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED:
                    vmAchievement->SetState(ra::data::models::AssetState::Triggered);
                    break;
                case RC_CLIENT_ACHIEVEMENT_STATE_DISABLED:
                    vmAchievement->SetState(ra::data::models::AssetState::Disabled);
                    break;
            }

            m_vAssets.Append(std::move(vmAchievement));

#ifndef RA_UTEST
            // prefetch the achievement image
            pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievementData->public_.badge_name);
#endif
        }

        // leaderboards
        auto* pLeaderboardData = pSubset->leaderboards;
        auto* pLeaderboardStop = pLeaderboardData + pSubset->public_.num_leaderboards;
        for (; pLeaderboardData < pLeaderboardStop; ++pLeaderboardData)
        {
            auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
            vmLeaderboard->SetID(pLeaderboardData->public_.id);
            vmLeaderboard->SetName(ra::Widen(pLeaderboardData->public_.title));
            vmLeaderboard->SetDescription(ra::Widen(pLeaderboardData->public_.description));
            vmLeaderboard->SetCategory(ra::data::models::AssetCategory::Core);
            vmLeaderboard->SetValueFormat(ra::itoe<ValueFormat>(pLeaderboardData->format));
            vmLeaderboard->SetLowerIsBetter(pLeaderboardData->public_.lower_is_better);
            //vmLeaderboard->SetHidden(pLeaderboardData->hidden);
            //vmLeaderboard->SetDefinition(pLeaderboardData.Definition);
            vmLeaderboard->CreateServerCheckpoint();
            vmLeaderboard->CreateLocalCheckpoint();

            switch (pAchievementData->public_.state)
            {
                case RC_CLIENT_LEADERBOARD_STATE_ACTIVE:
                    vmLeaderboard->SetState(ra::data::models::AssetState::Active);
                    break;
                case RC_CLIENT_LEADERBOARD_STATE_INACTIVE:
                    vmLeaderboard->SetState(ra::data::models::AssetState::Inactive);
                    break;
                case RC_CLIENT_LEADERBOARD_STATE_TRACKING:
                    vmLeaderboard->SetState(ra::data::models::AssetState::Primed);
                    break;
                case RC_CLIENT_LEADERBOARD_STATE_DISABLED:
                    vmLeaderboard->SetState(ra::data::models::AssetState::Disabled);
                    break;
            }

            m_vAssets.Append(std::move(vmLeaderboard));
        }
    }

    // merge local assets
    std::vector<ra::data::models::AssetModelBase*> vEmptyAssetsList;
    m_vAssets.ReloadAssets(vEmptyAssetsList);
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

void GameContext::RefreshUnlocks(bool bUnpause, int nPopup)
{
    if (m_nMode == Mode::CompatibilityTest)
    {
        std::set<unsigned int> vUnlockedAchievements;
        UpdateUnlocks(vUnlockedAchievements, bUnpause, nPopup);
        return;
    }

    BeginLoad();

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    ra::api::FetchUserUnlocks::Request request;
    request.GameId = m_nGameId;
    request.Hardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    request.CallAsync([this, bUnpause, nPopup](const ra::api::FetchUserUnlocks::Response& response)
    {
        UpdateUnlocks(response.UnlockedAchievements, bUnpause, nPopup);
        EndLoad();
    });
}

void GameContext::UpdateUnlocks(const std::set<unsigned int>& vUnlockedAchievements, bool bUnpause, int nPopup)
{
    // start by identifying all available achievements
    std::set<unsigned int> vLockedAchievements;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Assets().Count()); ++nIndex)
    {
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(Assets().GetItemAt(nIndex));
        if (pAchievement != nullptr)
            vLockedAchievements.insert(pAchievement->GetID());
    }

    // deactivate anything the player has already unlocked
    size_t nUnlockedCoreAchievements = 0U;
    for (const auto nUnlockedAchievement : vUnlockedAchievements)
    {
        vLockedAchievements.erase(nUnlockedAchievement);
        auto* pAchievement = Assets().FindAchievement(nUnlockedAchievement);

        // only core achievements will be automatically activated. also, they're all we want to count
        if (pAchievement != nullptr && pAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
        {
            pAchievement->SetState(ra::data::models::AssetState::Inactive);
            ++nUnlockedCoreAchievements;
        }
    }

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif
    // activate any core achievements the player hasn't earned and pre-fetch the locked image
    for (auto nAchievementId : vLockedAchievements)
    {
        auto* pAchievement = Assets().FindAchievement(nAchievementId);
        if (pAchievement && pAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
        {
            if (!pAchievement->IsActive() && pAchievement->GetState() != ra::data::models::AssetState::Disabled)
            {
                // modified assets should start in the inactive state
                if (pAchievement->GetChanges() != ra::data::models::AssetChanges::None)
                    pAchievement->SetState(ra::data::models::AssetState::Inactive);
                else
                    pAchievement->SetState(ra::data::models::AssetState::Waiting);
            }

#ifndef RA_UTEST
            if (!pAchievement->GetBadge().empty())
                pImageRepository.FetchImage(ra::ui::ImageType::Badge, ra::Narrow(pAchievement->GetBadge()) + "_lock");
#endif
        }
    }

    if (bUnpause)
    {
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().SetPaused(false);
#ifndef RA_UTEST
        auto& pAssetList = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().AssetList;
        pAssetList.SetProcessingActive(true);
#endif
    }

    if (nPopup)
    {
        auto* pPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().GetMessage(nPopup);
        if (pPopup != nullptr)
        {
            const auto nDisabledAchievements = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().DetectUnsupportedAchievements();
            if (nDisabledAchievements != 0)
            {
                const auto sDescription = ra::StringPrintf(L"%s (%s unsupported)", pPopup->GetDescription(), nDisabledAchievements);
                pPopup->SetDescription(sDescription);
            }

            pPopup->SetDetail(ra::StringPrintf(L"You have earned %u achievements", nUnlockedCoreAchievements));
            pPopup->RebuildRenderImage();
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

void GameContext::AwardAchievement(ra::AchievementID nAchievementId)
{
    auto* pAchievement = Assets().FindAchievement(nAchievementId);
    if (pAchievement == nullptr)
        return;

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    bool bTakeScreenshot = pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot);
    bool bSubmit = false;
    bool bIsError = false;

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->GetName(), pAchievement->GetPoints()));
    vmPopup->SetDetail(ra::Widen(pAchievement->GetDescription()));
    vmPopup->SetImage(ra::ui::ImageType::Badge, ra::Narrow(pAchievement->GetBadge()));
    vmPopup->SetPopupType(ra::ui::viewmodels::Popup::AchievementTriggered);

    switch (pAchievement->GetCategory())
    {
        case ra::data::models::AssetCategory::Local:
            vmPopup->SetTitle(L"Local Achievement Unlocked");
            bSubmit = false;
            bTakeScreenshot = false;
            break;

        case ra::data::models::AssetCategory::Unofficial:
            vmPopup->SetTitle(L"Unofficial Achievement Unlocked");
            bSubmit = false;
            break;

        default:
            vmPopup->SetTitle(L"Achievement Unlocked");
            bSubmit = true;
            break;
    }

    if (m_nMode == Mode::CompatibilityTest)
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Test ");
        vmPopup->SetTitle(sHeader);

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement->GetID(), "test compatibility mode");
        bSubmit = false;
    }

    if (pAchievement->IsModified() || // actual modifications
        (bSubmit && pAchievement->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Modified ");
        if (pAchievement->GetCategory() != ra::data::models::AssetCategory::Local)
            sHeader.append(L" LOCALLY");
        vmPopup->SetTitle(sHeader);

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement->GetID(), pAchievement->IsModified() ? "modified" : "unpublished");
        bIsError = true;
        bSubmit = false;
        bTakeScreenshot = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetTitle(L"Achievement Unlocked LOCALLY");
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement->GetID(), "RAM tampered with");
        bIsError = true;
        bSubmit = false;
    }

    if (bSubmit && _RA_HardcoreModeIsActive() && pEmulatorContext.IsMemoryInsecure())
    {
        vmPopup->SetTitle(L"Achievement Unlocked LOCALLY");
        vmPopup->SetErrorDetail(L"Error: RAM insecure");

        RA_LOG_INFO("Achievement %u not unlocked - %s", pAchievement->GetID(), "RAM insecure");
        bIsError = true;
        bSubmit = false;
    }

    if (bSubmit && pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
    {
        vmPopup->SetTitle(L"Offline Achievement Unlocked");
        bSubmit = false;
    }

    int nPopupId = -1;

    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(
        bIsError ? L"Overlay\\acherror.wav" : L"Overlay\\unlock.wav");

    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered) != ra::ui::viewmodels::PopupLocation::None)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        nPopupId = pOverlayManager.QueueMessage(vmPopup);

        if (bTakeScreenshot)
        {
            std::wstring sPath = ra::StringPrintf(L"%s%u.png", pConfiguration.GetScreenshotDirectory(), nAchievementId);
            pOverlayManager.CaptureScreenshot(nPopupId, sPath);
        }
    }

    pAchievement->SetState(ra::data::models::AssetState::Triggered);

    if (!bSubmit)
        return;

    ra::api::AwardAchievement::Request request;
    request.AchievementId = nAchievementId;
    request.Hardcore = _RA_HardcoreModeIsActive();
    request.GameHash = GameHash();
    request.CallAsyncWithRetry([this, nPopupId, nAchievementId](const ra::api::AwardAchievement::Response& response)
    {
        if (response.Succeeded())
        {
            // TODO: just read score from rc_client->user (when client handles unlocks)
            // success! update the player's score
            ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>().SetScore(response.NewPlayerScore);

            if (response.AchievementsRemaining == 0)
                CheckForMastery();
        }
        else if (ra::StringStartsWith(response.ErrorMessage, "User already has "))
        {
            // if server responds with "Error: User already has hardcore and regular achievements awarded." or
            // "Error: User already has this achievement awarded.", just ignore the error. There's no reason to
            // notify the user that the unlock failed because it had happened previously.
        }
        else if (nPopupId != -1)
        {
            // an error occurred, update the popup to display the error message
            auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
            auto pPopup = pOverlayManager.GetMessage(nPopupId);
            if (pPopup != nullptr)
            {
                pPopup->SetTitle(L"Achievement Unlock FAILED");
                pPopup->SetErrorDetail(response.ErrorMessage.empty() ?
                    L"Error submitting unlock" : ra::Widen(response.ErrorMessage));
                pPopup->RebuildRenderImage();
            }
            else
            {
                std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
                vmPopup->SetTitle(L"Achievement Unlock FAILED");
                vmPopup->SetErrorDetail(response.ErrorMessage.empty() ?
                    L"Error submitting unlock" : ra::Widen(response.ErrorMessage));
                vmPopup->SetPopupType(ra::ui::viewmodels::Popup::AchievementTriggered);

                const auto& pGameContext = ra::services::ServiceLocator::Get<GameContext>();
                const auto* pAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
                if (pAchievement != nullptr)
                {
                    vmPopup->SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->GetName(), pAchievement->GetPoints()));
                    vmPopup->SetImage(ra::ui::ImageType::Badge, ra::Narrow(pAchievement->GetBadge()));
                }
                else
                {
                    vmPopup->SetDescription(ra::StringPrintf(L"Achievement %u", nAchievementId));
                }

                ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\acherror.wav");
                pOverlayManager.QueueMessage(vmPopup);
            }
        }
    });
}

void GameContext::CheckForMastery()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Mastery) == ra::ui::viewmodels::PopupLocation::None)
        return;

    // if multiple achievements unlock the mastery at the same time, only show the popup once
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    if (m_nMasteryPopupId != 0 && pOverlayManager.GetMessage(m_nMasteryPopupId))
        return;

    // validate that all core assets are unlocked
    const auto& pAssets = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().Assets();
    unsigned nTotalCoreAchievementPoints = 0;
    size_t nNumCoreAchievements = -0;
    bool bActiveCoreAchievement = false;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pAssets.Count()); ++nIndex)
    {
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAssets.GetItemAt(nIndex));
        if (pAchievement != nullptr && pAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
        {
            ++nNumCoreAchievements;
            nTotalCoreAchievementPoints += pAchievement->GetPoints();

            if (pAchievement->IsActive())
            {
                bActiveCoreAchievement = true;
                break;
            }
        }
    }

    // if they are, show the mastery notification
    if (!bActiveCoreAchievement && nNumCoreAchievements > 0)
    {
        const bool bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);

        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(m_nGameId);
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();

        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmMessage(new ra::ui::viewmodels::PopupMessageViewModel);
        vmMessage->SetTitle(ra::StringPrintf(L"%s %s", bHardcore ? L"Mastered" : L"Completed", m_sGameTitle));
        vmMessage->SetDescription(ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints));
        vmMessage->SetDetail(ra::StringPrintf(L"%s | Play time: %dh%02dm", pConfiguration.GetUsername(), nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
        vmMessage->SetImage(ra::ui::ImageType::Icon, m_sGameImage);
        vmMessage->SetPopupType(ra::ui::viewmodels::Popup::Mastery);

        // if multiple achievements unlock the mastery at the same time, only show the popup once
        if (m_nMasteryPopupId != 0 && pOverlayManager.GetMessage(m_nMasteryPopupId))
            return;

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\unlock.wav");
        m_nMasteryPopupId = pOverlayManager.QueueMessage(vmMessage);

        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot))
        {
            std::wstring sPath = ra::StringPrintf(L"%sGame%u.png", pConfiguration.GetScreenshotDirectory(), m_nGameId);
            pOverlayManager.CaptureScreenshot(m_nMasteryPopupId, sPath);
        }
    }
}

void GameContext::DeactivateLeaderboards()
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Assets().Count()); ++nIndex)
    {
        auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(Assets().GetItemAt(nIndex));
        if (pLeaderboard != nullptr)
            pLeaderboard->SetState(ra::data::models::AssetState::Inactive);
    }
}

void GameContext::ActivateLeaderboards()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards)) // if not, simply ignore them.
    {
        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Assets().Count()); ++nIndex)
        {
            auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(Assets().GetItemAt(nIndex));
            if (pLeaderboard != nullptr && pLeaderboard->GetCategory() == ra::data::models::AssetCategory::Core)
                pLeaderboard->SetState(ra::data::models::AssetState::Waiting);
        }
    }
}

void GameContext::ShowSimplifiedScoreboard(ra::LeaderboardID nLeaderboardId, int nScore) const
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) == ra::ui::viewmodels::PopupLocation::None)
        return;

    const auto* pLeaderboard = Assets().FindLeaderboard(nLeaderboardId);
    if (!pLeaderboard)
        return;

    ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
    vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard->GetName()));

    const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
    auto& pEntryViewModel = vmScoreboard.Entries().Add();
    pEntryViewModel.SetRank(0);
    pEntryViewModel.SetScore(ra::Widen(pLeaderboard->FormatScore(nScore)));
    pEntryViewModel.SetUserName(ra::Widen(pUserName));
    pEntryViewModel.SetHighlighted(true);

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
        pLeaderboard->GetID(), std::move(vmScoreboard));
}

void GameContext::SubmitLeaderboardEntry(ra::LeaderboardID nLeaderboardId, int nScore) const
{
    const auto* pLeaderboard = Assets().FindLeaderboard(nLeaderboardId);
    if (pLeaderboard == nullptr)
        return;

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::Widen(pLeaderboard->GetName()));
    std::wstring sTitle = L"Leaderboard NOT Submitted";
    bool bSubmit = true;

    switch (pLeaderboard->GetCategory())
    {
        case ra::data::models::AssetCategory::Local:
            sTitle.insert(0, L"Local ");
            bSubmit = false;
            break;

        case ra::data::models::AssetCategory::Unofficial:
            sTitle.insert(0, L"Unofficial ");
            bSubmit = false;
            break;

        default:
            break;
    }

    if (m_nMode == Mode::CompatibilityTest)
    {
        vmPopup->SetDetail(L"Leaderboards are not submitted in test mode.");
        bSubmit = false;
    }

    if (pLeaderboard->IsModified() || // actual modifications
        (bSubmit && pLeaderboard->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        sTitle.insert(0, L"Modified ");
        bSubmit = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");
        bSubmit = false;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (bSubmit)
    {
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            vmPopup->SetErrorDetail(L"Submission requires Hardcore mode");
            bSubmit = false;
        }
        else if (pEmulatorContext.IsMemoryInsecure())
        {
            vmPopup->SetErrorDetail(L"Error: RAM insecure");
            bSubmit = false;
        }
        else if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
        {
            vmPopup->SetDetail(L"Leaderboards are not submitted in offline mode.");
            bSubmit = false;
        }
    }

    if (!bSubmit)
    {
        vmPopup->SetTitle(sTitle);
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        ShowSimplifiedScoreboard(nLeaderboardId, nScore);
        return;
    }

    ra::api::SubmitLeaderboardEntry::Request request;
    request.LeaderboardId = pLeaderboard->GetID();
    request.Score = nScore;
    request.GameHash = GameHash();
    request.CallAsyncWithRetry([this, nLeaderboardId = pLeaderboard->GetID()](const ra::api::SubmitLeaderboardEntry::Response& response)
    {
        const auto* pLeaderboard = Assets().FindLeaderboard(nLeaderboardId);

        if (!response.Succeeded())
        {
            std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
            vmPopup->SetTitle(L"Leaderboard NOT Submitted");
            vmPopup->SetDescription(pLeaderboard ? pLeaderboard->GetName() : ra::StringPrintf(L"Leaderboard %u", nLeaderboardId));
            vmPopup->SetDetail(!response.ErrorMessage.empty() ? ra::StringPrintf(L"Error: %s", response.ErrorMessage) : L"Error submitting entry");

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        }
        else if (pLeaderboard)
        {
            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) != ra::ui::viewmodels::PopupLocation::None)
            {
                ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
                vmScoreboard.SetHeaderText(pLeaderboard->GetName());

                const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
                constexpr int nEntriesDisplayed = 7; // display is currently hard-coded to show 7 entries
                bool bSeenPlayer = false;

                for (const auto& pEntry : response.TopEntries)
                {
                    auto& pEntryViewModel = vmScoreboard.Entries().Add();
                    pEntryViewModel.SetRank(pEntry.Rank);
                    pEntryViewModel.SetScore(ra::Widen(pLeaderboard->FormatScore(pEntry.Score)));
                    pEntryViewModel.SetUserName(ra::Widen(pEntry.User));

                    if (pEntry.User == pUserName)
                    {
                        bSeenPlayer = true;
                        pEntryViewModel.SetHighlighted(true);

                        if (response.BestScore != response.Score)
                            pEntryViewModel.SetScore(ra::StringPrintf(L"(%s) %s", pLeaderboard->FormatScore(response.Score), pLeaderboard->FormatScore(response.BestScore)));
                    }

                    if (vmScoreboard.Entries().Count() == nEntriesDisplayed)
                        break;
                }

                if (!bSeenPlayer)
                {
                    auto* pEntryViewModel = vmScoreboard.Entries().GetItemAt(6);
                    if (pEntryViewModel != nullptr)
                    {
                        pEntryViewModel->SetRank(response.NewRank);

                        if (response.BestScore != response.Score)
                            pEntryViewModel->SetScore(ra::StringPrintf(L"(%s) %s", pLeaderboard->FormatScore(response.Score), pLeaderboard->FormatScore(response.BestScore)));
                        else
                            pEntryViewModel->SetScore(ra::Widen(pLeaderboard->FormatScore(response.BestScore)));

                        pEntryViewModel->SetUserName(ra::Widen(pUserName));
                        pEntryViewModel->SetHighlighted(true);
                    }
                }

                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
                    pLeaderboard->GetID(), std::move(vmScoreboard));
            }
        }
    });
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
