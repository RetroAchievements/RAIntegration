#include "AssetUploadViewModel.hh"

#include "api\UpdateAchievement.hh"
#include "api\UpdateLeaderboard.hh"
#include "api\UpdateRichPresence.hh"
#include "api\UploadBadge.hh"

#include "context\UserContext.hh"
#include "context\IRcClient.hh"

#include "data\context\GameContext.hh"

#include "services\Http.hh"

#include "ui\ImageReference.hh"
#include "ui\IImageRepository.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "RA_Defs.h"
#include "util\Log.hh"

#include <rcheevos/include/rc_api_runtime.h>
#include <rcheevos/include/rc_api_editor.h>

namespace ra {
namespace ui {
namespace viewmodels {

AssetUploadViewModel::AssetUploadViewModel() noexcept
{
    SetWindowTitle(L"Uploading Assets");
}

void AssetUploadViewModel::QueueAsset(ra::data::models::AssetModelBase& pAsset)
{
    switch (pAsset.GetType())
    {
        case ra::data::models::AssetType::Achievement:
            QueueAchievement(*(dynamic_cast<ra::data::models::AchievementModel*>(&pAsset)));
            break;

        case ra::data::models::AssetType::Leaderboard:
            QueueLeaderboard(*(dynamic_cast<ra::data::models::LeaderboardModel*>(&pAsset)));
            break;

        case ra::data::models::AssetType::RichPresence:
            QueueRichPresence(*(dynamic_cast<ra::data::models::RichPresenceModel*>(&pAsset)));
            break;

        case ra::data::models::AssetType::MemoryNotes:
            QueueMemoryNotes(*(dynamic_cast<ra::data::models::MemoryNotesModel*>(&pAsset)));
            break;

        default:
            Expects(!"Unsupported asset type");
            break;
    }
}

void AssetUploadViewModel::QueueAchievement(ra::data::models::AchievementModel& pAchievement)
{
    auto& pItem = m_vUploadQueue.emplace_back();
    pItem.pAsset = &pAchievement;

    const auto& sBadge = pAchievement.GetBadge();
    if (ra::util::String::StartsWith(sBadge, L"local\\"))
    {
        pItem.nState = UploadState::WaitingForImage;

        for (const auto& pQueuedItem : m_vUploadQueue)
        {
            const auto* pQueuedAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pQueuedItem.pAsset);
            if (pQueuedAchievement && pQueuedAchievement != &pAchievement && pQueuedAchievement->GetBadge() == sBadge)
            {
                // badge request already queued, do nothing
                return;
            }
        }

        QueueTask([this, sBadge]()
        {
            UploadBadge(sBadge);
        });

        return;
    }

    QueueTask([this, pAchievement = &pAchievement]()
    {
        UploadAchievement(*pAchievement);
    });
}

void AssetUploadViewModel::QueueLeaderboard(ra::data::models::LeaderboardModel& pLeaderboard)
{
    auto& pItem = m_vUploadQueue.emplace_back();
    pItem.pAsset = &pLeaderboard;

    QueueTask([this, pLeaderboard = &pLeaderboard]()
    {
        UploadLeaderboard(*pLeaderboard);
    });
}

void AssetUploadViewModel::QueueRichPresence(ra::data::models::RichPresenceModel& pRichPresence)
{
    auto& pItem = m_vUploadQueue.emplace_back();
    pItem.pAsset = &pRichPresence;

    QueueTask([this, pRichPresence = &pRichPresence]()
    {
        UploadRichPresence(*pRichPresence);
    });
}

void AssetUploadViewModel::QueueMemoryNotes(ra::data::models::MemoryNotesModel& pMemoryNotes)
{
    pMemoryNotes.EnumerateModifiedNotes([this, pNotes = &pMemoryNotes](ra::data::ByteAddress nAddress)
    {
        QueueMemoryNote(*pNotes, nAddress);
        return true;
    });
}

static std::wstring ShortenNote(const std::wstring& sNote)
{
    return sNote.length() > 256 ? (sNote.substr(0, 253) + L"...") : sNote;
}

void AssetUploadViewModel::QueueMemoryNote(ra::data::models::MemoryNotesModel& pMemoryNotes, ra::data::ByteAddress nAddress)
{
    const auto* pOriginalAuthor = pMemoryNotes.GetServerNoteAuthor(nAddress);
    if (pOriginalAuthor != nullptr && !pOriginalAuthor->empty())
    {
        const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
        if (pUserContext.GetDisplayName() != *pOriginalAuthor)
        {
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();

            // author changed - confirm overwrite
            std::wstring sEmpty;
            const auto* pOriginalNote = pMemoryNotes.GetServerNote(nAddress);
            if (pOriginalNote == nullptr)
                pOriginalNote = &sEmpty;

            const auto* pNote = pMemoryNotes.FindNote(nAddress);
            if (pNote == nullptr)
                pNote = &sEmpty;

            ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
            if (!pNote->empty())
            {
                vmPrompt.SetHeader(
                    ra::util::String::Printf(L"Overwrite note for address %s?", pMemoryContext.FormatAddress(nAddress)));

                if (pOriginalNote->length() > 256 || pNote->length() > 256)
                {
                    const auto sNewNoteShort = ShortenNote(*pNote);
                    const auto sOldNoteShort = ShortenNote(*pOriginalNote);
                    vmPrompt.SetMessage(
                        ra::util::String::Printf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                                         *pOriginalAuthor, sOldNoteShort, sNewNoteShort));
                }
                else
                {
                    vmPrompt.SetMessage(
                        ra::util::String::Printf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                                         *pOriginalAuthor, *pOriginalNote, *pNote));
                }
            }
            else
            {
                const auto pNoteShort = ShortenNote(*pOriginalNote);
                vmPrompt.SetHeader(ra::util::String::Printf(L"Delete note for address %s?", pMemoryContext.FormatAddress(nAddress)));
                vmPrompt.SetMessage(ra::util::String::Printf(L"Are you sure you want to delete %s's note:\n\n%s", *pOriginalAuthor, pNoteShort));
            }

            vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
            vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
            if (vmPrompt.ShowModal() != ra::ui::DialogResult::Yes)
                return;
        }
    }

    auto& pItem = m_vUploadQueue.emplace_back();
    pItem.pAsset = &pMemoryNotes;
    pItem.nExtra = ra::to_signed(nAddress);

    QueueTask([this, pNotes = &pMemoryNotes]()
    {
        UploadMemoryNotes(*pNotes);
    });
}

void AssetUploadViewModel::OnBegin()
{
    SetMessage(ra::util::String::Printf(L"Uploading %d items...", TaskCount()));
}

void AssetUploadViewModel::UploadBadge(const std::wstring& sBadge)
{
    const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
    const std::wstring sFilename = pImageRepository.GetFilename(ra::ui::ImageType::Badge, ra::util::String::Narrow(sBadge));

    ra::api::UploadBadge::Request request;
    request.ImageFilePath = sFilename;

    ra::api::UploadBadge::Response response;
    {
        // only allow one badge upload at a time to prevent a race condition on server that could
        // result in non-unique image IDs being returned.
        std::lock_guard<std::mutex> pLock(m_pMutex);
        response = request.Call();
    }

    if (response.Result == ra::api::ApiResult::Incomplete)
    {
        Rest();
        UploadBadge(sBadge);
        return;
    }
    std::vector<ra::data::models::AchievementModel*> vAffectedAchievements;
    {
        // if the upload succeeded, update the badge property for each associated achievement and queue the
        // achievement update. if the upload failed, set the error message and don't queue the achievement update.
        for (auto& pItem : m_vUploadQueue)
        {
            if (pItem.nState == UploadState::WaitingForImage)
            {
                auto* pQueuedAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pItem.pAsset);
                if (pQueuedAchievement && pQueuedAchievement->GetBadge() == sBadge)
                {
                    if (response.Succeeded())
                    {
                        RA_LOG_INFO("Changed badge on achievement %u to %s", pQueuedAchievement->GetID(), response.BadgeId);
                        pQueuedAchievement->SetBadge(ra::util::String::Widen(response.BadgeId));
                        vAffectedAchievements.push_back(pQueuedAchievement);
                    }
                    else
                    {
                        pItem.sErrorMessage = response.ErrorMessage;
                        pItem.nState = UploadState::Failed;
                    }
                }
            }
        }
    }

    // the affected achievements can be uploaded now - will be empty on failure
    for (auto* pAchievement : vAffectedAchievements)
    {
        QueueTask([this, pAchievement]()
        {
            UploadAchievement(*pAchievement);
        });
    }
}

void AssetUploadViewModel::UploadAchievement(ra::data::models::AchievementModel& pAchievement)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    ra::api::UpdateAchievement::Request request;
    request.GameId = pGameContext.GetGameId(pAchievement.GetSubsetID());
    request.Title = pAchievement.GetName();
    request.Description = pAchievement.GetDescription();
    request.Trigger = pAchievement.GetTrigger();
    request.Points = pAchievement.GetPoints();
    request.Badge = ra::util::String::Narrow(pAchievement.GetBadge());

    switch (pAchievement.GetAchievementType())
    {
        case ra::data::models::AchievementType::None:
            request.Type = RC_ACHIEVEMENT_TYPE_STANDARD;
            break;
        case ra::data::models::AchievementType::Missable:
            request.Type = RC_ACHIEVEMENT_TYPE_MISSABLE;
            break;
        case ra::data::models::AchievementType::Progression:
            request.Type = RC_ACHIEVEMENT_TYPE_PROGRESSION;
            break;
        case ra::data::models::AchievementType::Win:
            request.Type = RC_ACHIEVEMENT_TYPE_WIN;
            break;
        default:
            request.Type = ra::etoi(pAchievement.GetAchievementType());
            break;
    }

    if (pAchievement.GetCategory() == ra::data::models::AssetCategory::Local)
    {
        request.Category = ra::to_unsigned(ra::etoi(ra::data::models::AssetCategory::Unofficial));
    }
    else
    {
        request.Category = ra::to_unsigned(ra::etoi(pAchievement.GetCategory()));
        request.AchievementId = pAchievement.GetID();
    }

    const auto& response = request.Call();

    // update the achievement model
    if (response.Succeeded())
    {
        if (pAchievement.GetCategory() == ra::data::models::AssetCategory::Local)
        {
            pAchievement.SetCategory(ra::data::models::AssetCategory::Unofficial);
            pAchievement.SetID(response.AchievementId);
        }

        pAchievement.UpdateLocalCheckpoint();
        pAchievement.UpdateServerCheckpoint();
    }
    else if (response.Result == ra::api::ApiResult::Incomplete)
    {
        Rest();
        UploadAchievement(pAchievement);
        return;
    }

    // update the queue
    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (auto& pScan : m_vUploadQueue)
    {
        if (pScan.pAsset == &pAchievement)
        {
            pScan.sErrorMessage = response.ErrorMessage;

            if (response.ErrorMessage == "Invalid state")
            {
                // generic API failure. Try to guess what went wrong
                if (pAchievement.GetName().empty())
                    pScan.sErrorMessage = "Title is required";
                else if (pAchievement.GetDescription().empty())
                    pScan.sErrorMessage = "Description is required";
                else if (pAchievement.GetTrigger().empty())
                    pScan.sErrorMessage = "At least one condition is required";
            }

            pScan.nState = response.Succeeded() ? UploadState::Success : UploadState::Failed;
            break;
        }
    }
}

void AssetUploadViewModel::UploadLeaderboard(ra::data::models::LeaderboardModel& pLeaderboard)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    ra::api::UpdateLeaderboard::Request request;
    request.GameId = pGameContext.GetGameId(pLeaderboard.GetSubsetID());
    request.Title = pLeaderboard.GetName();
    request.Description = pLeaderboard.GetDescription();
    request.StartTrigger = pLeaderboard.GetStartTrigger();
    request.SubmitTrigger = pLeaderboard.GetSubmitTrigger();
    request.CancelTrigger = pLeaderboard.GetCancelTrigger();
    request.ValueDefinition = pLeaderboard.GetValueDefinition();
    request.Format = pLeaderboard.GetValueFormat();
    request.LowerIsBetter = pLeaderboard.IsLowerBetter();

    if (pLeaderboard.GetCategory() != ra::data::models::AssetCategory::Local)
        request.LeaderboardId = pLeaderboard.GetID();

    const auto& response = request.Call();

    // update the leaderboard model
    if (response.Succeeded())
    {
        if (pLeaderboard.GetCategory() == ra::data::models::AssetCategory::Local)
        {
            pLeaderboard.SetCategory(ra::data::models::AssetCategory::Core);
            pLeaderboard.SetID(response.LeaderboardId);
        }

        pLeaderboard.UpdateLocalCheckpoint();
        pLeaderboard.UpdateServerCheckpoint();
    }
    else if (response.Result == ra::api::ApiResult::Incomplete)
    {
        Rest();
        UploadLeaderboard(pLeaderboard);
        return;
    }

    // update the queue
    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (auto& pScan : m_vUploadQueue)
    {
        if (pScan.pAsset == &pLeaderboard)
        {
            pScan.sErrorMessage = response.ErrorMessage;

            if (response.ErrorMessage == "Invalid state")
            {
                // generic API failure. Try to guess what went wrong
                if (pLeaderboard.GetName().empty())
                    pScan.sErrorMessage = "Title is required";
                else if (pLeaderboard.GetDescription().empty())
                    pScan.sErrorMessage = "Description is required";
                else if (pLeaderboard.GetStartTrigger().empty())
                    pScan.sErrorMessage = "At least one start condition is required";
                else if (pLeaderboard.GetSubmitTrigger().empty())
                    pScan.sErrorMessage = "At least one submit condition is required";
                else if (pLeaderboard.GetCancelTrigger().empty())
                    pScan.sErrorMessage = "At least one cancel condition is required";
                else if (pLeaderboard.GetValueDefinition().empty())
                    pScan.sErrorMessage = "At least one value condition is required";
            }

            pScan.nState = response.Succeeded() ? UploadState::Success : UploadState::Failed;
            break;
        }
    }
}

void AssetUploadViewModel::UploadRichPresence(ra::data::models::RichPresenceModel& pRichPresence)
{
    std::string sErrorMessage;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    ra::api::UpdateRichPresence::Request request;
    request.GameId = pGameContext.ActiveGameId();
    request.Script = pRichPresence.GetScript();

    const auto& response = request.Call();

    // update the rich presence model
    if (response.Succeeded())
    {
        if (pRichPresence.GetCategory() == ra::data::models::AssetCategory::Local)
            pRichPresence.SetCategory(ra::data::models::AssetCategory::Core);

        pRichPresence.UpdateLocalCheckpoint();
        pRichPresence.UpdateServerCheckpoint();
    }
    else if (response.Result == ra::api::ApiResult::Incomplete)
    {
        Rest();
        UploadRichPresence(pRichPresence);
        return;
    }

    // update the queue
    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (auto& pScan : m_vUploadQueue)
    {
        if (pScan.pAsset == &pRichPresence)
        {
            pScan.sErrorMessage = sErrorMessage;
            pScan.nState = response.Succeeded() ? UploadState::Success : UploadState::Failed;
            break;
        }
    }
}

void AssetUploadViewModel::UploadMemoryNotes(ra::data::models::MemoryNotesModel& pNotes)
{
    rc_api_update_code_notes_request_t api_params;

    std::unordered_map<ra::data::ByteAddress, std::string> vUtf8Notes;
    std::vector<rc_api_update_code_note_entry_t> vEntries;
    {
        std::lock_guard<std::mutex> pLock(m_pMutex);
        for (auto& pScan : m_vUploadQueue)
        {
            if (pScan.pAsset == &pNotes && pScan.nState == UploadState::None)
            {
                auto& pEntry = vEntries.emplace_back();
                pEntry.address = ra::to_unsigned(pScan.nExtra);

                const auto* pNote = pNotes.FindMemoryNoteModel(pEntry.address);
                if (pNote != nullptr && !pNote->GetNote().empty())
                    vUtf8Notes[pEntry.address] = ra::util::String::Narrow(pNote->GetNote());

                pScan.nState = UploadState::Uploading;
            }
        }

        if (vEntries.empty())
            return;

        for (auto& pEntry : vEntries)
        {
            const auto pIter = vUtf8Notes.find(pEntry.address);
            if (pIter != vUtf8Notes.end())
                pEntry.note = pIter->second.c_str();
        }

        memset(&api_params, 0, sizeof(api_params));
        const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
        api_params.username = pUserContext.GetUsername().c_str();
        api_params.api_token = pUserContext.GetApiToken().c_str();
        api_params.game_id = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().GameId();
        api_params.num_entries = gsl::narrow_cast<uint32_t>(vEntries.size());
        api_params.entries = vEntries.data();
    }

    auto& pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();
    rc_api_request_t api_request;
    auto nResult = rc_api_init_update_code_notes_request_hosted(&api_request, &api_params, pClient.GetHost());
    std::string sErrorMessage = rc_error_str(nResult);

    if (nResult == RC_OK)
    {
        bool bRetry = false;
        std::string sResponseBuffer;

        do
        {
            bRetry = false;

            rc_api_server_response_t api_response;
            pClient.SendRequest(api_request, api_response, sResponseBuffer);

            rc_api_update_code_notes_response_t response;
            nResult = rc_api_process_update_code_notes_server_response(&response, &api_response);
            if (nResult == RC_OK || nResult == RC_ACCESS_DENIED)
            {
                std::lock_guard<std::mutex> pLock(m_pMutex);
                for (auto& pScan : m_vUploadQueue)
                {
                    if (pScan.pAsset == &pNotes && pScan.nState == UploadState::Uploading)
                    {
                        for (auto& pEntry : vEntries)
                        {
                            if (pEntry.address == ra::to_unsigned(pScan.nExtra))
                            {
                                pScan.nState = UploadState::Success;

                                for (uint32_t i = 0; i < response.num_access_denied_addresses; i++)
                                {
                                    if (response.access_denied_addresses[i] == ra::to_unsigned(pScan.nExtra))
                                    {
                                        pScan.nState = UploadState::Failed;
                                        pScan.sErrorMessage = response.response.error_message;
                                        break;
                                    }
                                }

                                if (pScan.nState == UploadState::Success)
                                {
                                    if (pEntry.note)
                                    {
                                        const auto* pNote = pNotes.FindNote(pEntry.address);
                                        if (pNote)
                                            pNotes.SetServerNote(pEntry.address, *pNote);
                                    }
                                    else
                                        pNotes.SetServerNote(pEntry.address, L"");
                                }

                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                bRetry = api_response.http_status_code == ra::etoi(ra::services::Http::StatusCode::TooManyRequests)
                    || api_response.http_status_code == RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
                if (!bRetry)
                {
                    if (response.response.error_message)
                        sErrorMessage = response.response.error_message;
                    else
                        sErrorMessage = rc_error_str(nResult);
                }
            }

            rc_api_destroy_update_code_notes_response(&response);

            if (!bRetry)
                break;

            Rest();
        } while (true);
    }

    rc_api_destroy_request(&api_request);

    if (nResult != RC_OK)
    {
        std::lock_guard<std::mutex> pLock(m_pMutex);
        for (auto& pScan : m_vUploadQueue)
        {
            if (pScan.pAsset == &pNotes && pScan.nState == UploadState::Uploading)
            {
                for (auto& pEntry : vEntries)
                {
                    if (pEntry.address == ra::to_unsigned(pScan.nExtra))
                    {
                        pScan.nState = UploadState::Failed;
                        pScan.sErrorMessage = sErrorMessage;
                        break;
                    }
                }
            }
        }
    }
}

bool AssetUploadViewModel::HasFailures() const noexcept
{
    for (const auto& pItem : m_vUploadQueue)
    {
        if (pItem.nState == UploadState::Failed)
            return true;
    }

    return false;
}

void AssetUploadViewModel::ShowResults() const
{
    int nSuccessful = 0;
    int nFailed = 0;
    bool bAborted = false;
    for (const auto& pItem : m_vUploadQueue)
    {
        switch (pItem.nState)
        {
            case UploadState::Success:
                ++nSuccessful;
                break;

            case UploadState::Failed:
                ++nFailed;
                break;

            default:
                bAborted = true;
                break;
        }
    }

    auto sMessage = ra::util::String::Printf(L"%d items successfully uploaded.", nSuccessful);
    if (nFailed)
    {
        sMessage.append(ra::util::String::Printf(L"\n\n%d items failed:", nFailed));

        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        for (const auto& pItem : m_vUploadQueue)
        {
            if (pItem.nState == UploadState::Failed)
            {
                switch (pItem.pAsset->GetType())
                {
                    case ra::data::models::AssetType::MemoryNotes:
                        sMessage.append(ra::util::String::Printf(L"\n* Memory Note %s: %s",
                            pMemoryContext.FormatAddress(ra::to_unsigned(pItem.nExtra)),
                            pItem.sErrorMessage));
                        break;

                    default:
                        sMessage.append(ra::util::String::Printf(L"\n* %s: %s",
                            pItem.pAsset->GetName(),
                            pItem.sErrorMessage));
                        break;
                }
            }
        }
    }

    if (bAborted)
    {
        RA_LOG_INFO("Publish canceled. %u successful, %u unsuccessful, %u skipped", nSuccessful, nFailed, m_vUploadQueue.size() - nSuccessful - nFailed);
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Publish canceled.", sMessage);
    }
    else if (nFailed)
    {
        RA_LOG_INFO("Publish failed. %u successful, %u unsuccessful", nSuccessful, nFailed);
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Publish failed.", sMessage);
    }
    else
    {
        RA_LOG_INFO("Publish succeeded. %u successful", nSuccessful);
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Publish succeeded.", sMessage);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
