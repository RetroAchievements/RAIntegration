#ifndef RA_UI_ASSETUPLOAD_VIEW_MODEL_H
#define RA_UI_ASSETUPLOAD_VIEW_MODEL_H
#pragma once

#include "ProgressViewModel.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\CodeNotesModel.hh"
#include "data\models\LeaderboardModel.hh"
#include "data\models\RichPresenceModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class AssetUploadViewModel : public ProgressViewModel
{
public:
    GSL_SUPPRESS_F6 AssetUploadViewModel() noexcept;

    void QueueAsset(ra::data::models::AssetModelBase& pAsset);
    void QueueAchievement(ra::data::models::AchievementModel& pAchievement);
    void QueueLeaderboard(ra::data::models::LeaderboardModel& pLeaderboard);
    void QueueRichPresence(ra::data::models::RichPresenceModel& pRichPresence);
    void QueueCodeNotes(ra::data::models::CodeNotesModel& pLeaderboard);
    void QueueCodeNote(ra::data::models::CodeNotesModel& pCodeNotes, ra::data::ByteAddress nAddress);

    bool HasFailures() const noexcept;

    void ShowResults() const;

protected:
    void OnBegin() override;

    virtual void Rest() const noexcept
    {
        // sleep 500-1500ms to allow server to stop throttling us.
        // stagger value so we don't slam it in batches.
        Sleep(rand() % 1000 + 500);
    }

private:
    enum class UploadState
    {
        None = 0,
        Uploading,
        WaitingForImage,
        Success,
        Failed
    };

    struct UploadItem
    {
        ra::data::models::AssetModelBase* pAsset = nullptr;
        std::string sErrorMessage;
        int nExtra = 0;
        UploadState nState = UploadState::None;
    };

    void UploadBadge(const std::wstring& sBadge);
    void UploadAchievement(ra::data::models::AchievementModel& pAchievement);
    void UploadLeaderboard(ra::data::models::LeaderboardModel& pLeaderboard);
    void UploadRichPresence(ra::data::models::RichPresenceModel& pRichPresence);
    void UploadCodeNote(ra::data::models::CodeNotesModel& pNotes, ra::data::ByteAddress nAddress);

    std::vector<UploadItem> m_vUploadQueue;
    std::mutex m_pMutex;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_PROGRESS_VIEW_MODEL_H
