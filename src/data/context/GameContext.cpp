#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_md5factory.h"
#include "RA_StringUtils.h"

#include "api\AwardAchievement.hh"
#include "api\DeleteCodeNote.hh"
#include "api\FetchCodeNotes.hh"
#include "api\FetchGameData.hh"
#include "api\FetchUserUnlocks.hh"
#include "api\SubmitLeaderboardEntry.hh"
#include "api\UpdateCodeNote.hh"

#include "data\context\EmulatorContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\LocalBadgesModel.hh"

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

namespace ra {
namespace data {
namespace context {

void GameContext::LoadGame(unsigned int nGameId, Mode nMode)
{
    // remove the current asset from the asset editor
    auto& vmWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    vmWindowManager.AssetEditor.LoadAsset(nullptr);

    // reset the runtime
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    pRuntime.ResetRuntime();

    // reset the GameContext
    m_nMode = nMode;
    m_sGameTitle.clear();
    m_bRichPresenceFromFile = false;
    m_mCodeNotes.clear();
    m_vAssets.ResetLocalId();

    m_vLeaderboards.clear();

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

    // download the game data
    ra::api::FetchGameData::Request request;
    request.GameId = nGameId;

    const auto response = request.Call();
    if (response.Failed())
    {
        m_vAssets.EndUpdate();

        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download game data",
                                                                  ra::Widen(response.ErrorMessage));
        EndLoad();
        return;
    }

    // start fetching the code notes
    RefreshCodeNotes();

    // game properties
    m_sGameTitle = response.Title;
    m_sGameImage = response.ImageIcon;

    // rich presence
    {
        // normalize for MD5 calulation (unix line endings, must end with a line ending)
        std::string sRichPresence = response.RichPresence;
        sRichPresence.erase(std::remove(sRichPresence.begin(), sRichPresence.end(), '\r'), sRichPresence.end());
        if (!sRichPresence.empty() && sRichPresence.back() != '\n')
            sRichPresence.push_back('\n');

        LoadRichPresenceScript(sRichPresence);
        m_sServerRichPresenceMD5 = RAGenerateMD5(sRichPresence);
    }

    if (!response.RichPresence.empty())
    {
        // TODO: this file is written so devs can modify the rich presence without having to restart
        // the game. if the user doesn't have dev permission, there's no reason to write it.
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pRich = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence, std::to_wstring(nGameId));
        pRich->Write(response.RichPresence);
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
        RA_Leaderboard& pLeaderboard = *m_vLeaderboards.emplace_back(std::make_unique<RA_Leaderboard>(pLeaderboardData.Id));
        pLeaderboard.SetTitle(pLeaderboardData.Title);
        pLeaderboard.SetDescription(pLeaderboardData.Description);
        pLeaderboard.ParseFromString(pLeaderboardData.Definition.c_str(), pLeaderboardData.Format);
    }

    ActivateLeaderboards();

    // merge local assets
    std::vector<ra::data::models::AssetModelBase*> vEmptyAssetsList;
    m_vAssets.ReloadAssets(vEmptyAssetsList);

#ifndef RA_UTEST
    DoFrame();
#endif

    // show "game loaded" popup
    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
    const auto nPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        ra::StringPrintf(L"Loaded %s", response.Title),
        ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
        ra::ui::ImageType::Icon, m_sGameImage);

    // get user unlocks asynchronously
    RefreshUnlocks(!bWasPaused, nPopup);

    // finish up
    m_vAssets.EndUpdate();

    EndLoad();
    OnActiveGameChanged();
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
        if (pAchievement != nullptr)
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
                pAchievement->SetState(ra::data::models::AssetState::Waiting);

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

void GameContext::AwardMastery() const
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    ra::api::FetchUserUnlocks::Request masteryRequest;
    masteryRequest.GameId = m_nGameId;
    masteryRequest.Hardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    masteryRequest.CallAsync([this](const ra::api::FetchUserUnlocks::Response& response)
    {
        unsigned int nNumCoreAchievements = 0;
        unsigned int nTotalCoreAchievementPoints = 0;

        bool bActiveCoreAchievement = false;
        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Assets().Count()); ++nIndex)
        {
            const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(Assets().GetItemAt(nIndex));
            if (pAchievement != nullptr && pAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
            {
                if (response.UnlockedAchievements.find(pAchievement->GetID()) == response.UnlockedAchievements.end())
                {
                    bActiveCoreAchievement = true;
                    break;
                }

                ++nNumCoreAchievements;
                nTotalCoreAchievementPoints += pAchievement->GetPoints();
            }
        }

        if (!bActiveCoreAchievement)
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            const bool bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\unlock.wav");

            const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(m_nGameId);
            const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();

            auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
            const auto nPopup = pOverlayManager.QueueMessage(
                ra::StringPrintf(L"%s %s", bHardcore ? L"Mastered" : L"Completed", m_sGameTitle),
                ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
                ra::StringPrintf(L"%s | Play time: %dh%02dm", pConfiguration.GetUsername(), nPlayTimeMinutes / 60, nPlayTimeMinutes % 60),
                ra::ui::ImageType::Icon, m_sGameImage);

            auto* pPopup = pOverlayManager.GetMessage(nPopup);
            if (pPopup != nullptr)
                pPopup->SetPopupType(ra::ui::viewmodels::Popup::Mastery);

            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot))
            {
                std::wstring sPath = ra::StringPrintf(L"%sGame%u.png", pConfiguration.GetScreenshotDirectory(), m_nGameId);
                pOverlayManager.CaptureScreenshot(nPopup, sPath);
            }
        }
    });
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

        bSubmit = false;
    }

    if (pAchievement->IsModified() || // actual modifications
        (bSubmit && pAchievement->GetChanges() != ra::data::models::AssetChanges::None)) // unpublished changes
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Modified ");
        sHeader.insert(sHeader.length() - 8, L"NOT ");
        vmPopup->SetTitle(sHeader);

        bIsError = true;
        bSubmit = false;
        bTakeScreenshot = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetTitle(L"Achievement NOT Unlocked");
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        bIsError = true;
        bSubmit = false;
    }

    if (bSubmit && _RA_HardcoreModeIsActive() && pEmulatorContext.IsMemoryInsecure())
    {
        vmPopup->SetTitle(L"Achievement NOT Unlocked");
        vmPopup->SetErrorDetail(L"Error: RAM insecure");

        bIsError = true;
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
    request.CallAsyncWithRetry([nPopupId, nAchievementId](const ra::api::AwardAchievement::Response& response)
    {
        if (response.Succeeded())
        {
            // success! update the player's score
            ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>().SetScore(response.NewPlayerScore);
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
                pPopup->SetTitle(L"Achievement NOT Unlocked");
                pPopup->SetErrorDetail(response.ErrorMessage.empty() ?
                    L"Error submitting unlock" : ra::Widen(response.ErrorMessage));
                pPopup->RebuildRenderImage();
            }
            else
            {
                std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
                vmPopup->SetTitle(L"Achievement NOT Unlocked");
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

    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Mastery) != ra::ui::viewmodels::PopupLocation::None)
    {
        bool bHasCoreAchievement = false;
        bool bActiveCoreAchievement = false;
        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Assets().Count()); ++nIndex)
        {
            const auto* pCheckAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(Assets().GetItemAt(nIndex));
            if (pCheckAchievement != nullptr && pAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
            {
                bHasCoreAchievement = true;

                if (pCheckAchievement->IsActive())
                {
                    bActiveCoreAchievement = true;
                    break;
                }
            }
        }

        if (!bActiveCoreAchievement && bHasCoreAchievement)
        {
            // delay mastery notification by 500ms to avoid race conditions where the get unlocks API returns
            // before the last achievement was unlocked for the user.
            ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>()
                .ScheduleAsync(std::chrono::milliseconds(500), [this]() { AwardMastery(); });
        }
    }
}

void GameContext::DeactivateLeaderboards() noexcept
{
    for (auto& pLeaderboard : m_vLeaderboards)
        pLeaderboard->SetActive(false);
}

void GameContext::ActivateLeaderboards()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards)) // if not, simply ignore them.
    {
        for (auto& pLeaderboard : m_vLeaderboards)
            pLeaderboard->SetActive(true);
    }
}

void GameContext::ShowSimplifiedScoreboard(ra::LeaderboardID nLeaderboardId, int nScore) const
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) == ra::ui::viewmodels::PopupLocation::None)
        return;

    const auto* pLeaderboard = FindLeaderboard(nLeaderboardId);
    if (!pLeaderboard)
        return;

    ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
    vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard->Title()));

    const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
    auto& pEntryViewModel = vmScoreboard.Entries().Add();
    pEntryViewModel.SetRank(0);
    pEntryViewModel.SetScore(ra::Widen(pLeaderboard->FormatScore(nScore)));
    pEntryViewModel.SetUserName(ra::Widen(pUserName));
    pEntryViewModel.SetHighlighted(true);

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
        pLeaderboard->ID(), std::move(vmScoreboard));
}

void GameContext::SubmitLeaderboardEntry(ra::LeaderboardID nLeaderboardId, int nScore) const
{
    const auto* pLeaderboard = FindLeaderboard(nLeaderboardId);
    if (pLeaderboard == nullptr)
        return;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.WasMemoryModified())
    {
        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
        vmPopup->SetTitle(L"Leaderboard NOT Submitted");
        vmPopup->SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        ShowSimplifiedScoreboard(nLeaderboardId, nScore);
        return;
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
        vmPopup->SetTitle(L"Leaderboard NOT Submitted");
        vmPopup->SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup->SetErrorDetail(L"Submission requires Hardcore mode");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        ShowSimplifiedScoreboard(nLeaderboardId, nScore);
        return;
    }

    if (m_nMode == Mode::CompatibilityTest)
    {
        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
        vmPopup->SetTitle(L"Leaderboard NOT Submitted");
        vmPopup->SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup->SetDetail(L"Leaderboards are not submitted in test mode.");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        ShowSimplifiedScoreboard(nLeaderboardId, nScore);
        return;
    }

    if (pEmulatorContext.IsMemoryInsecure())
    {
        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
        vmPopup->SetTitle(L"Leaderboard NOT Submitted");
        vmPopup->SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup->SetErrorDetail(L"Error: RAM insecure");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        ShowSimplifiedScoreboard(nLeaderboardId, nScore);
        return;
    }

    ra::api::SubmitLeaderboardEntry::Request request;
    request.LeaderboardId = pLeaderboard->ID();
    request.Score = nScore;
    request.GameHash = GameHash();
    request.CallAsyncWithRetry([this, nLeaderboardId = pLeaderboard->ID()](const ra::api::SubmitLeaderboardEntry::Response& response)
    {
        const auto* pLeaderboard = FindLeaderboard(nLeaderboardId);

        if (!response.Succeeded())
        {
            std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
            vmPopup->SetTitle(L"Leaderboard NOT Submitted");
            vmPopup->SetDescription(pLeaderboard ? ra::Widen(pLeaderboard->Title()) : ra::StringPrintf(L"Leaderboard %u", nLeaderboardId));
            vmPopup->SetDetail(!response.ErrorMessage.empty() ? ra::StringPrintf(L"Error: %s", response.ErrorMessage) : L"Error submitting entry");

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
        }
        else if (pLeaderboard)
        {
            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard) != ra::ui::viewmodels::PopupLocation::None)
            {
                ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
                vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard->Title()));

                const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
                constexpr int nEntriesDisplayed = 7; // display is currently hard-coded to show 7 entries

                for (const auto& pEntry : response.TopEntries)
                {
                    auto& pEntryViewModel = vmScoreboard.Entries().Add();
                    pEntryViewModel.SetRank(pEntry.Rank);
                    pEntryViewModel.SetScore(ra::Widen(pLeaderboard->FormatScore(pEntry.Score)));
                    pEntryViewModel.SetUserName(ra::Widen(pEntry.User));

                    if (pEntry.User == pUserName)
                        pEntryViewModel.SetHighlighted(true);

                    if (vmScoreboard.Entries().Count() == nEntriesDisplayed)
                        break;
                }

                if (response.NewRank >= nEntriesDisplayed)
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
                else if (response.BestScore != response.Score)
                {
                    auto* pEntryViewModel = vmScoreboard.Entries().GetItemAt(response.NewRank - 1);
                    if (pEntryViewModel != nullptr)
                        pEntryViewModel->SetScore(ra::StringPrintf(L"(%s) %s", pLeaderboard->FormatScore(response.Score), pLeaderboard->FormatScore(response.BestScore)));
                }

                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
                    pLeaderboard->ID(), std::move(vmScoreboard));
            }
        }
    });
}

void GameContext::LoadRichPresenceScript(const std::string& sRichPresenceScript)
{
    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().ActivateRichPresence(sRichPresenceScript);
}

bool GameContext::HasRichPresence() const
{
    return ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().HasRichPresence();
}

std::wstring GameContext::GetRichPresenceDisplayString() const
{
    return ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetRichPresenceDisplayString();
}

void GameContext::ReloadRichPresenceScript()
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pRich = pLocalStorage.ReadText(ra::services::StorageItemType::RichPresence, std::to_wstring(GameId()));

    std::string sRichPresence;
    if (pRich != nullptr)
    {
        std::string sLine;
        while (pRich->GetLine(sLine))
        {
            sRichPresence.append(sLine);
            sRichPresence.append("\n");
        }

        // remove UTF-8 BOM if present
        if (ra::StringStartsWith(sRichPresence, "\xef\xbb\xbf"))
            sRichPresence.erase(0, 3);
    }

    const auto sFileRichPresenceMD5 = RAGenerateMD5(sRichPresence);
    m_bRichPresenceFromFile = (sFileRichPresenceMD5 != m_sServerRichPresenceMD5);

    LoadRichPresenceScript(sRichPresence);
}

void GameContext::RefreshCodeNotes()
{
    m_mCodeNotes.clear();

    if (m_nGameId == 0)
        return;

    BeginLoad();

    ra::api::FetchCodeNotes::Request request;
    request.GameId = m_nGameId;
    request.CallAsync([this](const ra::api::FetchCodeNotes::Response& response)
    {
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download code notes",
                ra::Widen(response.ErrorMessage));
            return;
        }

        for (const auto& pNote : response.Notes)
            AddCodeNote(pNote.Address, pNote.Author, pNote.Note);

        EndLoad();
    });
}

void GameContext::AddCodeNote(ra::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote)
{
    unsigned int nBytes = 1;

    // attempt to match "X byte", "X Byte", "XX bytes", or "XX Bytes"
    auto nIndex = sNote.find(L"yte");
    if (nIndex != std::string::npos && nIndex >= 3)
    {
        wchar_t c = sNote.at(--nIndex);
        if (c == L'b' || c == L'B')
        {
            c = sNote.at(--nIndex);
            if (c == L'-' || c == L' ')
                c = sNote.at(--nIndex);

            if (c >= L'0' && c <= L'9')
            {
                int nMultiplier = 1;
                nBytes = 0;
                do
                {
                    nBytes += (c - L'0') * nMultiplier;
                    if (nIndex == 0)
                        break;

                    nMultiplier *= 10;
                    c = sNote.at(--nIndex);
                } while (c >= L'0' && c <= L'9');
            }
        }
    }

    if (nBytes == 1)
    {
        // attempt to match "16-bit" or "32-bit"
        nIndex = sNote.find(L"Bit");
        if (nIndex == std::string::npos)
            nIndex = sNote.find(L"bit");

        if (nIndex != std::string::npos && nIndex >= 3)
        {
            wchar_t c = sNote.at(--nIndex);
            if (c == L'-' || c == L' ')
                c = sNote.at(--nIndex);

            if (c == L'6' && sNote.at(nIndex - 1) == L'1')
                nBytes = 2;
            else if (c == L'2' && sNote.at(nIndex - 1) == L'3')
                nBytes = 4;
        }
    }

    m_mCodeNotes.insert_or_assign(nAddress, CodeNote{ sAuthor, sNote, nBytes });
    OnCodeNoteChanged(nAddress, sNote);
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

ra::ByteAddress GameContext::FindCodeNoteStart(ra::ByteAddress nAddress) const
{
    auto pIter = m_mCodeNotes.lower_bound(nAddress);

    // exact match, return it
    if (pIter != m_mCodeNotes.end() && pIter->first == nAddress)
        return nAddress;

    // lower_bound returns the first item _after_ the search value. we want to look at the item _before_.
    if (pIter != m_mCodeNotes.begin())
    {
        --pIter;

        // check to see if the item before the search value contains the search value
        if (pIter->first + pIter->second.Bytes > nAddress)
            return pIter->first;
    }

    return 0xFFFFFFFF;
}

std::wstring GameContext::FindCodeNote(ra::ByteAddress nAddress, MemSize nSize) const
{
    unsigned int nCheckSize = 0;
    switch (nSize)
    {
        case MemSize::SixteenBit:
            nCheckSize = 2;
            break;

        case MemSize::ThirtyTwoBit:
            nCheckSize = 4;
            break;

        default: // 1 byte or less
            nCheckSize = 1;
            break;
    }

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = m_mCodeNotes.lower_bound(nAddress);
    if (pIter != m_mCodeNotes.end())
    {
        const ra::ByteAddress nNoteAddress = pIter->first;
        const unsigned int nNoteSize = pIter->second.Bytes;
        std::wstring sNote = pIter->second.Note;

        const auto iNewLine = sNote.find('\n');
        if (iNewLine != std::string::npos)
        {
            sNote.resize(iNewLine);

            if (sNote.back() == '\r')
                sNote.pop_back();
        }

        // exact match
        if (nAddress == nNoteAddress && nNoteSize == nCheckSize)
            return sNote;

        // check for overlap
        if (nAddress + nCheckSize - 1 >= nNoteAddress)
        {
            if (nCheckSize == 1)
                sNote.append(ra::StringPrintf(L" [%d/%d]", nNoteAddress - nAddress + 1, nNoteSize));
            else
                sNote.append(L" [partial]");

            return sNote;
        }
    }

    // check previous note for overlap
    if (pIter != m_mCodeNotes.begin())
    {
        --pIter;

        const ra::ByteAddress nNoteAddress = pIter->first;
        const unsigned int nNoteSize = pIter->second.Bytes;
        if (nNoteAddress + nNoteSize - 1 >= nAddress)
        {
            std::wstring sNote = pIter->second.Note;

            const auto iNewLine = sNote.find('\n');
            if (iNewLine != std::string::npos)
            {
                sNote.resize(iNewLine);

                if (sNote.back() == '\r')
                    sNote.pop_back();
            }

            if (nCheckSize == 1)
                sNote.append(ra::StringPrintf(L" [%d/%d]", nAddress - nNoteAddress + 1, nNoteSize));
            else
                sNote.append(L" [partial]");

            return sNote;
        }
    }

    return std::wstring();
}

bool GameContext::SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    if (m_nGameId == 0)
        return false;

    ra::api::UpdateCodeNote::Request request;
    request.GameId = m_nGameId;
    request.Address = nAddress;
    request.Note = sNote;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
            AddCodeNote(nAddress, pUserContext.GetUsername(), sNote);
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error saving note for address %s", ra::ByteAddressToString(nAddress)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}

bool GameContext::DeleteCodeNote(ra::ByteAddress nAddress)
{
    if (m_mCodeNotes.find(nAddress) == m_mCodeNotes.end())
        return true;

    if (m_nGameId == 0)
        return false;

    ra::api::DeleteCodeNote::Request request;
    request.GameId = m_nGameId;
    request.Address = nAddress;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            m_mCodeNotes.erase(nAddress);
            OnCodeNoteChanged(nAddress, L"");
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error deleting note for address %s", ra::ByteAddressToString(nAddress)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}

} // namespace context
} // namespace data
} // namespace ra
