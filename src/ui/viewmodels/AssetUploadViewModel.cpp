#include "AssetUploadViewModel.hh"

#include "api\UpdateAchievement.hh"
#include "api\UploadBadge.hh"

#include "data\context\GameContext.hh"

#include "ui\ImageReference.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "RA_Log.h"

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
    if (ra::StringStartsWith(sBadge, L"local\\"))
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

void AssetUploadViewModel::OnBegin()
{
    SetMessage(ra::StringPrintf(L"Uploading %d items...", TaskCount()));
}

void AssetUploadViewModel::UploadBadge(const std::wstring& sBadge)
{
    const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
    const std::wstring sFilename = pImageRepository.GetFilename(ra::ui::ImageType::Badge, ra::Narrow(sBadge));

    ra::api::UploadBadge::Request request;
    request.ImageFilePath = sFilename;

    std::vector<ra::data::models::AchievementModel*> vAffectedAchievements;
    {
        // only allow one badge upload at a time to prevent a race condition on server that could
        // result in non-unique image IDs being returned.
        std::lock_guard<std::mutex> pLock(m_pMutex);
        const auto response = request.Call();

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
                        pQueuedAchievement->SetBadge(ra::Widen(response.BadgeId));
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
    ra::api::UpdateAchievement::Request request;
    request.GameId = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().GameId();
    request.Title = pAchievement.GetName();
    request.Description = pAchievement.GetDescription();
    request.Trigger = pAchievement.GetTrigger();
    request.Points = pAchievement.GetPoints();
    request.Badge = ra::Narrow(pAchievement.GetBadge());

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

    // update the achievement
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

    // update the queue
    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (auto& pScan : m_vUploadQueue)
    {
        if (pScan.pAsset == &pAchievement)
        {
            pScan.sErrorMessage = response.ErrorMessage;
            pScan.nState = response.Succeeded() ? UploadState::Success : UploadState::Failed;
            break;
        }
    }
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

    auto sMessage = ra::StringPrintf(L"%d items successfully uploaded.", nSuccessful);
    if (nFailed)
    {
        sMessage.append(ra::StringPrintf(L"\n\n%d items failed:", nFailed));

        for (const auto& pItem : m_vUploadQueue)
        {
            if (pItem.nState == UploadState::Failed)
                sMessage.append(ra::StringPrintf(L"\n* %s: %s", pItem.pAsset->GetName(), pItem.sErrorMessage));
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
