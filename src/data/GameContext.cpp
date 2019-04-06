#include "GameContext.hh"

#include "Exports.hh"

#include "RA_Dlg_Achievement.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "api\AwardAchievement.hh"
#include "api\FetchGameData.hh"
#include "api\FetchUserUnlocks.hh"
#include "api\SubmitLeaderboardEntry.hh"

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

void GameContext::LoadGame(unsigned int nGameId, Mode nMode)
{
    m_nGameId = nGameId;
    m_nMode = nMode;
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

    if (!m_vLeaderboards.empty())
    {
        for (auto& pLeaderboard : m_vLeaderboards)
            pLeaderboard->SetActive(false);
        m_vLeaderboards.clear();
    }

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

    // show "game loaded" popup
    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
    const auto nPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
        ra::StringPrintf(L"Loaded %s", response.Title),
        ra::StringPrintf(L"%u achievements, %u points", nNumCoreAchievements, nTotalCoreAchievementPoints),
        ra::ui::ImageType::Icon, response.ImageIcon);

    // get user unlocks asynchronously
    RefreshUnlocks(!bWasPaused, nPopup);
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
        if (pAchievement && pAchievement->Category() == ra::etoi(AchievementSet::Type::Core))
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
    if (ActiveAchievementType() == AchievementSet::Type::Core)
    {
        for (int nIndex = 0; nIndex < ra::to_signed(g_pActiveAchievements->NumAchievements()); ++nIndex)
        {
            const Achievement& Ach = g_pActiveAchievements->GetAchievement(nIndex);
            g_AchievementsDialog.OnEditData(nIndex, Dlg_Achievements::Column::Achieved, Ach.Active() ? "No" : "Yes");
        }
    }
    else
    {
        for (int nIndex = 0; nIndex < ra::to_signed(g_pActiveAchievements->NumAchievements()); ++nIndex)
        {
            const Achievement& Ach = g_pActiveAchievements->GetAchievement(nIndex);
            g_AchievementsDialog.OnEditData(nIndex, Dlg_Achievements::Column::Active, Ach.Active() ? "Yes" : "No");
        }
    }
#endif

    if (nPopup)
    {
        auto* pPopup = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().GetMessage(nPopup);
        if (pPopup != nullptr)
        {
            size_t nUnlockedCoreAchievements = 0U;
            for (auto& pAchievement : m_vAchievements)
            {
                if (pAchievement->Category() == ra::etoi(AchievementSet::Type::Core) && !pAchievement->Active())
                    ++nUnlockedCoreAchievements;
            }

            pPopup->SetDetail(ra::StringPrintf(L"You have earned %u achievements", nUnlockedCoreAchievements));
            pPopup->RebuildRenderImage();
        }
    }
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
            pAchievement->SetCategory(ra::etoi(AchievementSet::Type::Local));
            pAchievement->SetID(nId);

#ifndef RA_UTEST
            g_pLocalAchievements->AddAchievement(pAchievement);
#endif
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
            while (*pScan && (*pScan != ':' || strchr("ABCPRabcpr", pScan[-1]) != nullptr))
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
        if (!pAchievement || pAchievement->Category() != ra::etoi(AchievementSet::Type::Local))
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

bool GameContext::RemoveAchievement(unsigned int nAchievementId)
{
    for (auto pIter = m_vAchievements.begin(); pIter != m_vAchievements.end(); ++pIter)
    {
        if (*pIter && (*pIter)->ID() == nAchievementId)
        {
#ifndef RA_UTEST
            // temporary code for compatibility until global collections are eliminated
            switch (ra::itoe<AchievementSet::Type>((*pIter)->Category()))
            {
                case AchievementSet::Type::Core:
                    g_pCoreAchievements->RemoveAchievement(pIter->get());
                    break;

                default:
                case AchievementSet::Type::Unofficial:
                    g_pUnofficialAchievements->RemoveAchievement(pIter->get());
                    break;

                case AchievementSet::Type::Local:
                    g_pLocalAchievements->RemoveAchievement(pIter->get());
                    break;
            }
#endif

            m_vAchievements.erase(pIter);
            return true;
        }
    }

    return false;
}

void GameContext::AwardAchievement(unsigned int nAchievementId) const
{
    auto* pAchievement = FindAchievement(nAchievementId);
    if (pAchievement == nullptr)
        return;

    bool bSubmit = false;
    bool bIsError = false;

    ra::ui::viewmodels::PopupMessageViewModel vmPopup;
    vmPopup.SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->Title(), pAchievement->Points()));
    vmPopup.SetDetail(ra::Widen(pAchievement->Description()));
    vmPopup.SetImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());

    switch (ra::itoe<AchievementSet::Type>(pAchievement->Category()))
    {
        case AchievementSet::Type::Local:
            vmPopup.SetTitle(L"Local Achievement Unlocked");
            bSubmit = false;
            break;

        case AchievementSet::Type::Unofficial:
            vmPopup.SetTitle(L"Unofficial Achievement Unlocked");
            bSubmit = false;
            break;

        default:
            vmPopup.SetTitle(L"Achievement Unlocked");
            bSubmit = true;
            break;
    }

    if (m_nMode == Mode::CompatibilityTest)
    {
        auto sHeader = vmPopup.GetTitle();
        sHeader.insert(0, L"Test ");
        vmPopup.SetTitle(sHeader);

        bSubmit = false;
    }

    if (pAchievement->Modified())
    {
        auto sHeader = vmPopup.GetTitle();
        sHeader.insert(0, L"Modified ");
        if (ActiveAchievementType() != AchievementSet::Type::Local)
            sHeader.insert(sHeader.length() - 8, L"NOT ");
        vmPopup.SetTitle(sHeader);

        bIsError = true;
        bSubmit = false;
    }

#ifndef RA_UTEST
    if (bSubmit && g_bRAMTamperedWith)
    {
        vmPopup.SetTitle(L"Achievement NOT Unlocked");
        vmPopup.SetErrorDetail(L"Error: RAM tampered with");

        bIsError = true;
        bSubmit = false;
    }
#endif

    ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(
        bIsError ? L"Overlay\\acherror.wav" : L"Overlay\\unlock.wav");

    const auto nPopupId =
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(vmPopup));

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
                ra::ui::viewmodels::PopupMessageViewModel vmPopup;
                vmPopup.SetTitle(L"Achievement NOT Unlocked");
                vmPopup.SetErrorDetail(response.ErrorMessage.empty() ?
                    L"Error submitting unlock" : ra::Widen(response.ErrorMessage));

                const auto& pGameContext = ra::services::ServiceLocator::Get<GameContext>();
                const auto* pAchievement = pGameContext.FindAchievement(nAchievementId);
                if (pAchievement != nullptr)
                {
                    vmPopup.SetDescription(ra::StringPrintf(L"%s (%u)", pAchievement->Title(), pAchievement->Points()));
                    vmPopup.SetImage(ra::ui::ImageType::Badge, pAchievement->BadgeImageURI());
                }
                else
                {
                    vmPopup.SetDescription(ra::StringPrintf(L"Achievement %u", nAchievementId));
                }

                ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\acherror.wav");
                pOverlayManager.QueueMessage(std::move(vmPopup));
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

#ifndef RA_UTEST
    if (g_bRAMTamperedWith)
    {
        ra::ui::viewmodels::PopupMessageViewModel vmPopup;
        vmPopup.SetTitle(L"Leaderboard NOT Submitted");
        vmPopup.SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup.SetErrorDetail(L"Error: RAM tampered with");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(vmPopup));
        return;
    }
#endif

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        ra::ui::viewmodels::PopupMessageViewModel vmPopup;
        vmPopup.SetTitle(L"Leaderboard NOT Submitted");
        vmPopup.SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup.SetErrorDetail(L"Submission requires Hardcore mode");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(vmPopup));
        return;
    }

    if (m_nMode == Mode::CompatibilityTest)
    {
        ra::ui::viewmodels::PopupMessageViewModel vmPopup;
        vmPopup.SetTitle(L"Leaderboard NOT Submitted");
        vmPopup.SetDescription(ra::Widen(pLeaderboard->Title()));
        vmPopup.SetDetail(L"Leaderboards are not submitted in test mode.");

        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(vmPopup));
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
            ra::ui::viewmodels::PopupMessageViewModel vmPopup;
            vmPopup.SetTitle(L"Leaderboard NOT Submitted");
            vmPopup.SetDescription(pLeaderboard ? ra::Widen(pLeaderboard->Title()) : ra::StringPrintf(L"Leaderboard %u", nLeaderboardId));
            vmPopup.SetDetail(!response.ErrorMessage.empty() ? ra::StringPrintf(L"Error: %s", response.ErrorMessage) : L"Error submitting entry");

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(vmPopup));
        }
        else if (pLeaderboard)
        {
            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards))
            {
                ra::ui::viewmodels::ScoreboardViewModel vmScoreboard;
                vmScoreboard.SetHeaderText(ra::Widen(pLeaderboard->Title()));

                const auto& pUserName = ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();

                for (const auto& pEntry : response.TopEntries)
                {
                    auto& pEntryViewModel = vmScoreboard.Entries().Add();
                    pEntryViewModel.SetRank(pEntry.Rank);
                    pEntryViewModel.SetScore(ra::Widen(pLeaderboard->FormatScore(pEntry.Score)));
                    pEntryViewModel.SetUserName(ra::Widen(pEntry.User));

                    if (pEntry.User == pUserName)
                        pEntryViewModel.SetHighlighted(true);
                }

                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueScoreboard(
                    pLeaderboard->ID(), std::move(vmScoreboard));
            }
        }
    });
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
