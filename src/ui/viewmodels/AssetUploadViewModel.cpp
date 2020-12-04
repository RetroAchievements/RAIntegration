#include "AssetUploadViewModel.hh"

#include "api\UpdateAchievement.hh"
#include "api\UploadBadge.hh"

#include "data\context\GameContext.hh"

#include "ui\ImageReference.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

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
    //auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
    //const std::wstring sFilename = pImageRepostiory.GetFilename(ra::ui::ImageType::Badge, ra::Narrow(sBadge));
    const std::wstring sFilename = sBadge; // pImageRepostiory.GetFilename(ra::ui::ImageType::Badge, ra::Narrow(sBadge));

    ra::api::UploadBadge::Request request;
    request.ImageFilePath = sFilename;
    const auto response = request.Call();

    // if the upload failed, set the error message on all the associated achievements (including this one)
    if (!response.Succeeded())
    {
        std::lock_guard<std::mutex> pLock(m_pMutex);
        for (auto& pItem : m_vUploadQueue)
        {
            if (pItem.nState == UploadState::WaitingForImage)
            {
                auto* pQueuedAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pItem.pAsset);
                if (pQueuedAchievement && pQueuedAchievement->GetBadge() == sBadge)
                {
                    pItem.sErrorMessage = response.ErrorMessage;
                    pItem.nState = UploadState::Failed;
                }
            }
        }

        return;
    }

    // upload succeeded. update the badge property for each associated achievement and identify any achievements that
    // need to be reprocessed because the image wasn't ready when they were previously given a chance to process.
    std::vector<ra::data::models::AchievementModel*> vAffectedAchievements;

    {
        std::lock_guard<std::mutex> pLock(m_pMutex);
        for (auto& pItem : m_vUploadQueue)
        {
            if (pItem.nState == UploadState::WaitingForImage)
            {
                auto* pQueuedAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pItem.pAsset);
                if (pQueuedAchievement && pQueuedAchievement->GetBadge() == sBadge)
                {
                    pQueuedAchievement->SetBadge(ra::Widen(response.BadgeId));
                    vAffectedAchievements.push_back(pQueuedAchievement);
                }
            }
        }
    }

    // the affected achievements can be uploaded now
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

    UploadItem* pItem = nullptr;
    for (auto& pScan : m_vUploadQueue)
    {
        if (pScan.pAsset == &pAchievement)
        {
            pItem = &pScan;
            break;
        }
    }
    Expects(pItem != nullptr);

    const auto& response = request.Call();
    if (response.Succeeded())
    {
        if (pAchievement.GetCategory() == ra::data::models::AssetCategory::Local)
        {
            pAchievement.SetCategory(ra::data::models::AssetCategory::Unofficial);
            pAchievement.SetID(response.AchievementId);
        }

        pAchievement.UpdateLocalCheckpoint();
        pAchievement.UpdateServerCheckpoint();

        pItem->nState = UploadState::Success;
    }
    else
    {
        pItem->sErrorMessage = response.ErrorMessage;
        pItem->nState = UploadState::Failed;
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
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Publish canceled.", sMessage);
    else if (nFailed)
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Publish failed.", sMessage);
    else
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Publish succeeded.", sMessage);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
