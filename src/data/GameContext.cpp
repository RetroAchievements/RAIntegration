#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Defs.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "api\AwardAchievement.hh"
#include "api\DeleteCodeNote.hh"
#include "api\FetchCodeNotes.hh"
#include "api\FetchGameData.hh"
#include "api\FetchUserUnlocks.hh"
#include "api\SubmitLeaderboardEntry.hh"
#include "api\UpdateCodeNote.hh"

#include "data\EmulatorContext.hh"
#include "data\UserContext.hh"

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

namespace ra {
namespace data {

static void RefreshOverlay()
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.RefreshOverlay();
}

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

void GameContext::LoadGame(unsigned int nGameId, Mode nMode)
{
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    pRuntime.ActivateRichPresence(nullptr);

    m_nMode = nMode;
    m_sGameTitle.clear();
    m_pRichPresence = nullptr;
    m_mCodeNotes.clear();
    m_nNextLocalId = GameContext::FirstLocalId;

    if (!m_vAchievements.empty())
    {
        for (auto& pAchievement : m_vAchievements)
            pAchievement->SetActive(false);
        m_vAchievements.clear();
    }

    if (!m_vLeaderboards.empty())
    {
        for (auto& pLeaderboard : m_vLeaderboards)
            pLeaderboard->SetActive(false);
        m_vLeaderboards.clear();
    }

    if (nGameId == 0)
    {
        m_sGameHash.clear();

        if (m_nGameId != 0)
        {
            m_nGameId = 0;
            OnActiveGameChanged();
        }
        return;
    }

    m_nGameId = nGameId;

    ra::api::FetchGameData::Request request;
    request.GameId = nGameId;

    const auto response = request.Call();
    if (response.Failed())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download game data",
                                                                  ra::Widen(response.ErrorMessage));
        return;
    }

    RefreshCodeNotes();

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif

    // game properties
    m_sGameTitle = response.Title;
    m_sGameImage = response.ImageIcon;

    // rich presence
    if (!response.RichPresence.empty())
    {
        LoadRichPresenceScript(response.RichPresence);

        // TODO: this file is written so devs can modify the rich presence without having to restart
        // the game. if the user doesn't have dev permission, there's no reason to write it.
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pRich = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence, std::to_wstring(nGameId));
        pRich->Write(response.RichPresence);
    }

    // achievements
    const bool bWasPaused = pRuntime.IsPaused();
    pRuntime.SetPaused(true);

    unsigned int nNumCoreAchievements = 0;
    unsigned int nTotalCoreAchievementPoints = 0;
    for (const auto& pAchievementData : response.Achievements)
    {
        auto& pAchievement = NewAchievement(ra::itoe<Achievement::Category>(pAchievementData.CategoryId));
        pAchievement.SetID(pAchievementData.Id);
        CopyAchievementData(pAchievement, pAchievementData);

#ifndef RA_UTEST
        // prefetch the achievement image
        pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievementData.BadgeName);
#endif

        if (pAchievementData.CategoryId == ra::to_unsigned(ra::etoi(Achievement::Category::Core)))
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
        pLeaderboard.ParseFromString(pLeaderboardData.Definition.c_str(), pLeaderboardData.Format.c_str());
    }

    ActivateLeaderboards();

    // merge local achievements
    m_nNextLocalId = GameContext::FirstLocalId;
    MergeLocalAchievements();

#ifndef RA_UTEST
    g_AchievementsDialog.UpdateAchievementList();
#endif

    // show "game loaded" popup
    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
    const auto nPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        ra::StringPrintf(L"Loaded %s", response.Title),
        ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
        ra::ui::ImageType::Icon, m_sGameImage);

    // get user unlocks asynchronously
    RefreshUnlocks(!bWasPaused, nPopup);

    OnActiveGameChanged();
}

void GameContext::OnActiveGameChanged()
{
    RefreshOverlay();

    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnActiveGameChanged();
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

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    ra::api::FetchUserUnlocks::Request request;
    request.GameId = m_nGameId;
    request.Hardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    request.CallAsync([this, bUnpause, nPopup](const ra::api::FetchUserUnlocks::Response& response)
    {
        UpdateUnlocks(response.UnlockedAchievements, bUnpause, nPopup);
    });
}

void GameContext::UpdateUnlocks(const std::set<unsigned int>& vUnlockedAchievements, bool bUnpause, int nPopup)
{
    std::set<unsigned int> vLockedAchievements;
    for (auto& pAchievement : m_vAchievements)
    {
        vLockedAchievements.insert(pAchievement->ID());
        pAchievement->SetActive(false);
    }

    for (const auto nUnlockedAchievement : vUnlockedAchievements)
        vLockedAchievements.erase(nUnlockedAchievement);

#ifndef RA_UTEST
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
#endif
    // activate any core achievements the player hasn't earned and pre-fetch the locked image
    for (auto nAchievementId : vLockedAchievements)
    {
        auto* pAchievement = FindAchievement(nAchievementId);
        if (pAchievement && pAchievement->GetCategory() == Achievement::Category::Core)
        {
            pAchievement->SetActive(true);

#ifndef RA_UTEST
            if (!pAchievement->BadgeImageURI().empty())
                pImageRepository.FetchImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI() + "_lock");
#endif
        }
    }

    if (bUnpause)
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().SetPaused(false);

#ifndef RA_UTEST
    g_AchievementsDialog.UpdateActiveAchievements();
#endif

    if (nPopup)
    {
        auto* pPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().GetMessage(nPopup);
        if (pPopup != nullptr)
        {
            size_t nUnlockedCoreAchievements = 0U;
            for (auto& pAchievement : m_vAchievements)
            {
                if (pAchievement->GetCategory() == Achievement::Category::Core && !pAchievement->Active())
                    ++nUnlockedCoreAchievements;
            }

            pPopup->SetDetail(ra::StringPrintf(L"You have earned %u achievements", nUnlockedCoreAchievements));
            pPopup->RebuildRenderImage();
        }
    }

    RefreshOverlay();
}

void GameContext::EnumerateFilteredAchievements(std::function<bool(const Achievement&)> callback) const
{
#ifdef RA_UTEST
    EnumerateAchievements(callback);
#else
    for (auto nID : g_AchievementsDialog.FilteredAchievements())
    {
        const auto* pAchievement = FindAchievement(nID);
        if (pAchievement != nullptr && !callback(*pAchievement))
            break;
    }
#endif
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

        // field 1: ID
        ra::Tokenizer pTokenizer(sLine);
        const auto nId = pTokenizer.ReadNumber();
        if (!pTokenizer.Consume(':'))
            continue;

        bool bIsNew = false;
        Achievement* pAchievement = nullptr;
        if (nId != 0)
            pAchievement = FindAchievement(nId);
        if (!pAchievement)
        {
            bIsNew = true;

            if (nId >= m_nNextLocalId)
                m_nNextLocalId = nId + 1;

            // append new local achievement to collection
            pAchievement = m_vAchievements.emplace_back(std::make_unique<Achievement>()).get();
            Ensures(pAchievement != nullptr);
            pAchievement->SetCategory(Achievement::Category::Local);
            pAchievement->SetID(nId);
        }

        // field 2: trigger
        std::string sTrigger;
        if (pTokenizer.PeekChar() == '"')
        {
            sTrigger = pTokenizer.ReadQuotedString();
        }
        else
        {
            // unquoted trigger requires special parsing because flags also use colons
            const char* pTrigger = pTokenizer.GetPointer(pTokenizer.CurrentPosition());
            const char* pScan = pTrigger;
            while (*pScan && (*pScan != ':' || strchr("ABCNPRabcnpr", pScan[-1]) != nullptr))
                pScan++;

            sTrigger.assign(pTrigger, pScan - pTrigger);
            pTokenizer.Advance(sTrigger.length());
        }

        if (!pTokenizer.Consume(':'))
            continue;

        // field 3: title
        std::string sTitle;
        if (pTokenizer.PeekChar() == '"')
            sTitle = pTokenizer.ReadQuotedString();
        else
            sTitle = pTokenizer.ReadTo(':');
        if (!pTokenizer.Consume(':'))
            continue;
        if (pAchievement->Title() != sTitle)
        {
            pAchievement->SetTitle(sTitle);
            pAchievement->SetModified(true);
        }

        // field 4: description
        std::string sDescription;
        if (pTokenizer.PeekChar() == '"')
            sDescription = pTokenizer.ReadQuotedString();
        else
            sDescription = pTokenizer.ReadTo(':');
        if (!pTokenizer.Consume(':'))
            continue;
        if (pAchievement->Description() != sDescription)
        {
            pAchievement->SetDescription(sDescription);
            pAchievement->SetModified(true);
        }

        // field 5: progress (unused)
        pTokenizer.AdvanceTo(':');
        if (!pTokenizer.Consume(':'))
            continue;

        // field 6: progress max (unused)
        pTokenizer.AdvanceTo(':');
        if (!pTokenizer.Consume(':'))
            continue;

        // field 7: progress format (unused)
        pTokenizer.AdvanceTo(':');
        if (!pTokenizer.Consume(':'))
            continue;

        // field 8: author
        const std::string sAuthor = pTokenizer.ReadTo(':');
        if (bIsNew)
            pAchievement->SetAuthor(sAuthor);
        if (!pTokenizer.Consume(':'))
            continue;

        // field 9: points
        const auto nPoints = pTokenizer.ReadNumber();
        if (!pTokenizer.Consume(':'))
            continue;
        if (pAchievement->Points() != nPoints)
        {
            pAchievement->SetPoints(nPoints);
            pAchievement->SetModified(true);
        }

        // field 10: created date
        const auto nCreated = pTokenizer.ReadNumber();
        if (bIsNew)
            pAchievement->SetCreatedDate(nCreated);
        if (!pTokenizer.Consume(':'))
            continue;

        // field 11: modified date
        const auto nModified = pTokenizer.ReadNumber();
        pAchievement->SetModifiedDate(nModified);
        if (!pTokenizer.Consume(':'))
            continue;

        // field 12: up votes (unused)
        pTokenizer.AdvanceTo(':');
        if (!pTokenizer.Consume(':'))
            continue;

        // field 13: down votes (unused)
        pTokenizer.AdvanceTo(':');
        if (!pTokenizer.Consume(':'))
            continue;

        // field 14: badge
        const auto sBadge = pTokenizer.ReadTo(':');
        if (pAchievement->BadgeImageURI() != sBadge)
        {
            pAchievement->SetBadgeImage(sBadge);
            pAchievement->SetModified(true);
        }

        // line is valid, parse the trigger
        if (bIsNew)
        {
            pAchievement->ParseTrigger(sTrigger.c_str());
            pAchievement->SetModified(false);
        }
        else
        {
            auto sExistingTrigger = pAchievement->CreateMemString();
            if (sExistingTrigger != sTrigger)
            {
                pAchievement->ParseTrigger(sTrigger.c_str());
                pAchievement->SetModified(true);
            }
        }
    }

    // assign unique ids to any new achievements without one
    for (auto& pAchievement : m_vAchievements)
    {
        if (pAchievement->ID() == 0)
            pAchievement->SetID(m_nNextLocalId++);
    }
}

static void WriteEscaped(ra::services::TextWriter& pData, const std::string& sText)
{
    size_t nToEscape = 0;
    for (auto c : sText)
    {
        if (c == ':' || c == '"' || c == '\\')
            ++nToEscape;
    }

    if (nToEscape == 0)
    {
        pData.Write(sText);
        return;
    }

    std::string sEscaped;
    sEscaped.reserve(sText.length() + nToEscape + 2);
    sEscaped.push_back('"');
    for (auto c : sText)
    {
        if (c == '"' || c == '\\')
            sEscaped.push_back('\\');
        sEscaped.push_back(c);
    }
    sEscaped.push_back('"');
    pData.Write(sEscaped);
}

bool GameContext::SaveLocal() const
{
    // Commits local achievements to the file
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(GameId()));
    if (pData == nullptr)
        return false;

    pData->WriteLine(_RA_IntegrationVersion()); // version used to create the file
    pData->WriteLine(GameTitle());

    for (const auto& pAchievement : m_vAchievements)
    {
        if (!pAchievement || pAchievement->GetCategory() != Achievement::Category::Local)
            continue;

        // field 1: ID
        pData->Write(std::to_string(pAchievement->ID()));
        // field 2: trigger
        pData->Write(":\"");
        pData->Write(pAchievement->CreateMemString());
        pData->Write("\":");
        // field 3: title
        WriteEscaped(*pData, pAchievement->Title());
        pData->Write(":");
        // field 4: description
        WriteEscaped(*pData, pAchievement->Description());
        // field 5: progress
        // field 6: progress max
        // field 7: progress format
        pData->Write("::::");
        // field 8: author
        pData->Write(pAchievement->Author());
        pData->Write(":");
        // field 9: points
        pData->Write(std::to_string(pAchievement->Points()));
        pData->Write(":");
        // field 10: created date
        pData->Write(std::to_string(pAchievement->CreatedDate()));
        pData->Write(":");
        // field 11: modified date
        pData->Write(std::to_string(pAchievement->ModifiedDate()));
        // field 12: up votes
        // field 13: down votes
        pData->Write(":::");
        // field 14: badge
        pData->Write(pAchievement->BadgeImageURI());
        pData->WriteLine();
    }

    return true;
}

Achievement& GameContext::NewAchievement(Achievement::Category nType)
{
    Achievement& pAchievement = *m_vAchievements.emplace_back(std::make_unique<Achievement>());
    pAchievement.SetCategory(nType);
    pAchievement.SetID(m_nNextLocalId++);
    return pAchievement;
}

bool GameContext::RemoveAchievement(ra::AchievementID nAchievementId)
{
    for (auto pIter = m_vAchievements.begin(); pIter != m_vAchievements.end(); ++pIter)
    {
        if (*pIter && (*pIter)->ID() == nAchievementId)
        {
            m_vAchievements.erase(pIter);
            return true;
        }
    }

    return false;
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
        for (const auto& pAchievement : m_vAchievements)
        {
            if (pAchievement->GetCategory() == Achievement::Category::Core)
            {
                if (response.UnlockedAchievements.find(pAchievement->ID()) == response.UnlockedAchievements.end())
                {
                    bActiveCoreAchievement = true;
                    break;
                }

                ++nNumCoreAchievements;
                nTotalCoreAchievementPoints += pAchievement->Points();
            }
        }

        if (!bActiveCoreAchievement)
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            const bool bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\unlock.wav");

            auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
            const auto nPopup = pOverlayManager.QueueMessage(
                ra::StringPrintf(L"%s %s", bHardcore ? L"Mastered" : L"Completed", m_sGameTitle),
                ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
                ra::ui::ImageType::Icon, m_sGameImage);

            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot))
            {
                std::wstring sPath = ra::StringPrintf(L"%sGame%u.png", pConfiguration.GetScreenshotDirectory(), m_nGameId);
                pOverlayManager.CaptureScreenshot(nPopup, sPath);
            }
        }
    });
}

void GameContext::AwardAchievement(ra::AchievementID nAchievementId) const
{
    auto* pAchievement = FindAchievement(nAchievementId);
    if (pAchievement == nullptr)
        return;

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    bool bTakeScreenshot = pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot);
    bool bSubmit = false;
    bool bIsError = false;

    std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
    vmPopup->SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->Title(), pAchievement->Points()));
    vmPopup->SetDetail(ra::Widen(pAchievement->Description()));
    vmPopup->SetImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());

    switch (pAchievement->GetCategory())
    {
        case Achievement::Category::Local:
            vmPopup->SetTitle(L"Local Achievement Unlocked");
            bSubmit = false;
            bTakeScreenshot = false;
            break;

        case Achievement::Category::Unofficial:
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

    if (pAchievement->Modified())
    {
        auto sHeader = vmPopup->GetTitle();
        sHeader.insert(0, L"Modified ");
        sHeader.insert(sHeader.length() - 8, L"NOT ");
        vmPopup->SetTitle(sHeader);

        bIsError = true;
        bSubmit = false;
        bTakeScreenshot = false;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (bSubmit && pEmulatorContext.WasMemoryModified())
    {
        vmPopup->SetTitle(L"Achievement NOT Unlocked");
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        bIsError = true;
        bSubmit = false;
    }

    int nPopupId = -1;

    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredNotifications))
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(
            bIsError ? L"Overlay\\acherror.wav" : L"Overlay\\unlock.wav");

        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        nPopupId = pOverlayManager.QueueMessage(vmPopup);

        if (bTakeScreenshot)
        {
            std::wstring sPath = ra::StringPrintf(L"%s%u.png", pConfiguration.GetScreenshotDirectory(), nAchievementId);
            pOverlayManager.CaptureScreenshot(nPopupId, sPath);
        }
    }

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

                const auto& pGameContext = ra::services::ServiceLocator::Get<GameContext>();
                const auto* pAchievement = pGameContext.FindAchievement(nAchievementId);
                if (pAchievement != nullptr)
                {
                    vmPopup->SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->Title(), pAchievement->Points()));
                    vmPopup->SetImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());
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

    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotification))
    {
        bool bHasCoreAchievement = false;
        bool bActiveCoreAchievement = false;
        for (const auto& pCheckAchievement : m_vAchievements)
        {
            if (pCheckAchievement->GetCategory() == Achievement::Category::Core)
            {
                bHasCoreAchievement = true;

                if (pCheckAchievement->Active())
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

void GameContext::ReloadAchievements(Achievement::Category nCategory)
{
    if (nCategory == Achievement::Category::Local)
    {
        auto pIter = m_vAchievements.begin();
        while (pIter != m_vAchievements.end())
        {
            if ((*pIter)->GetCategory() == nCategory)
                pIter = m_vAchievements.erase(pIter);
            else
                ++pIter;
        }

        MergeLocalAchievements();
    }
    else
    {
        // TODO
    }

#ifndef RA_UTEST
    g_AchievementsDialog.UpdateAchievementList();
#endif
}

bool GameContext::ReloadAchievement(ra::AchievementID nAchievementId)
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
    if (pAchievement.GetCategory() == Achievement::Category::Local)
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

void GameContext::SubmitLeaderboardEntry(ra::LeaderboardID nLeaderboardId, unsigned int nScore) const
{
    const auto* pLeaderboard = FindLeaderboard(nLeaderboardId);
    if (pLeaderboard == nullptr)
        return;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (pEmulatorContext.WasMemoryModified())
    {
        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmPopup(new ra::ui::viewmodels::PopupMessageViewModel);
        vmPopup->SetTitle(L"Leaderboard NOT Submitted");
        vmPopup->SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup->SetErrorDetail(L"Error: RAM tampered with");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmPopup);
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
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards))
            {
                ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
                vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard->Title()));

                const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
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
    if (sRichPresenceScript.empty())
    {
        m_pRichPresence = nullptr;
    }
    else
    {
        const int nSize = rc_richpresence_size(sRichPresenceScript.c_str());
        if (nSize < 0)
        {
            // parse error occurred
            RA_LOG("rc_richpresence_size returned %d", nSize);
            m_pRichPresence = nullptr;

            const std::string sErrorRP = ra::StringPrintf("Display:\nParse error %d\n", nSize);
            const int nSize2 = rc_richpresence_size(sErrorRP.c_str());
            if (nSize2 > 0)
            {
                m_pRichPresenceBuffer = std::make_shared<std::vector<unsigned char>>(nSize2);
                auto* pRichPresence = rc_parse_richpresence(m_pRichPresenceBuffer->data(), sErrorRP.c_str(), nullptr, 0);
                m_pRichPresence = pRichPresence;
            }
        }
        else
        {
            // allocate space and parse again
            m_pRichPresenceBuffer = std::make_shared<std::vector<unsigned char>>(nSize);
            auto* pRichPresence = rc_parse_richpresence(m_pRichPresenceBuffer->data(), sRichPresenceScript.c_str(), nullptr, 0);
            m_pRichPresence = pRichPresence;
        }
    }

    auto* pRichPresence = static_cast<rc_richpresence_t*>(m_pRichPresence);
    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().ActivateRichPresence(pRichPresence);
}

bool GameContext::HasRichPresence() const noexcept
{
    return (m_pRichPresence != nullptr);
}

std::wstring GameContext::GetRichPresenceDisplayString() const
{
    if (m_pRichPresence == nullptr)
        return std::wstring(L"No Rich Presence defined.");

    auto* pRichPresence = static_cast<rc_richpresence_t*>(m_pRichPresence);

    // we evaluate the memrefs in AchievementRuntime::Process - don't evaluate them again
    auto* pRichPresenceMemRefs = pRichPresence->memrefs;
    pRichPresence->memrefs = nullptr;

    std::string sRichPresence;
    sRichPresence.resize(512);
    const auto nLength = rc_evaluate_richpresence(pRichPresence, sRichPresence.data(),
        gsl::narrow_cast<unsigned int>(sRichPresence.capacity()), rc_peek_callback, nullptr, nullptr);
    sRichPresence.resize(nLength);

    // restore memrefs pointer
    pRichPresence->memrefs = pRichPresenceMemRefs;

    return ra::Widen(sRichPresence);
}

void GameContext::ReloadRichPresenceScript()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pRich = pLocalStorage.ReadText(ra::services::StorageItemType::RichPresence, std::to_wstring(pGameContext.GameId()));
    if (pRich == nullptr)
        return;

    std::string sRichPresence, sLine;
    while (pRich->GetLine(sLine))
    {
        sRichPresence.append(sLine);
        sRichPresence.append("\n");
    }

    LoadRichPresenceScript(sRichPresence);
}

void GameContext::RefreshCodeNotes()
{
    m_mCodeNotes.clear();

    if (m_nGameId == 0)
        return;

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

#ifndef RA_UTEST
        g_MemoryDialog.RepopulateCodeNotes();
#endif
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

        // exact match
        if (nAddress == nNoteAddress && nNoteSize == nCheckSize)
            return pIter->second.Note;

        // check for overlap
        if (nAddress + nCheckSize - 1 >= nNoteAddress)
        {
            if (nCheckSize == 1)
                return ra::StringPrintf(L"%s [%d/%d]", pIter->second.Note, nNoteAddress - nAddress + 1, nNoteSize);

            return pIter->second.Note + L" [partial]";
        }
    }

    // check previous note for overlap
    if (pIter != m_mCodeNotes.begin())
    {
        --pIter;

        if (pIter->first + pIter->second.Bytes - 1 >= nAddress)
        {
            if (nCheckSize == 1)
                return ra::StringPrintf(L"%s [%d/%d]", pIter->second.Note, nAddress - pIter->first + 1, pIter->second.Bytes);

            return pIter->second.Note + L" [partial]";
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
            const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
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

} // namespace data
} // namespace ra
