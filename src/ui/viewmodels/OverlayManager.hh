#ifndef RA_UI_OVERLAY_MANAGER_H
#define RA_UI_OVERLAY_MANAGER_H

#include "RA_Interface.h"

#include "OverlayViewModel.hh"
#include "ChallengeIndicatorViewModel.hh"
#include "PopupMessageViewModel.hh"
#include "ScoreboardViewModel.hh"
#include "ScoreTrackerViewModel.hh"

#include "data\Types.hh"
#include "data\context\GameContext.hh"

#include "ui\ImageReference.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayManager : 
    protected ra::ui::IImageRepository::NotifyTarget,
    protected ra::data::context::GameContext::NotifyTarget
{
public:    
    GSL_SUPPRESS_F6 OverlayManager() = default;
    ~OverlayManager() noexcept = default;
    OverlayManager(const OverlayManager&) noexcept = delete;
    OverlayManager& operator=(const OverlayManager&) noexcept = delete;
    OverlayManager(OverlayManager&&) noexcept = delete;
    OverlayManager& operator=(OverlayManager&&) noexcept = delete;

    void InitializeNotifyTargets();

    /// <summary>
    /// Updates the overlay.
    /// </summary>
    /// <param name="pInput">The emulator input state.</param>
    void Update(const ControllerInput& pInput);

    /// <summary>
    /// Renders the overlay.
    /// </summary>
    /// <param name="pSurface">The surface to render to.</param>
    /// <param name="bRedrawAll">True to redraw everything even if it hasn't changed or moved.</param>
    void Render(ra::ui::drawing::ISurface& pSurface, bool bRedrawAll);

    /// <summary>
    /// Requests the overlay be redrawn.
    /// </summary>
    void RequestRender();

    /// <summary>
    /// Starts the animation to show the overlay.
    /// </summary>
    void ShowOverlay()
    {
        m_vmOverlay.Activate();
    }

    /// <summary>
    /// Starts the animation to hide the overlay.
    /// </summary>
    void HideOverlay()
    {
        m_vmOverlay.Deactivate();
    }

    /// <summary>
    /// Determines if the overlay is completely covering the emulator screen.
    /// </summary>
    bool IsOverlayFullyVisible() const noexcept
    {
        return m_vmOverlay.CurrentState() == OverlayViewModel::State::Visible;
    }

    /// <summary>
    /// Refreshes the currently visible overlay page
    /// </summary>
    void RefreshOverlay()
    {
        if (m_vmOverlay.CurrentState() != OverlayViewModel::State::Hidden)
        {
            m_vmOverlay.CurrentPage().Refresh();
            RequestRender();
        }
    }

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(std::unique_ptr<PopupMessageViewModel>& pMessage);

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(ra::ui::viewmodels::Popup nPopup, const std::wstring& sTitle, const std::wstring& sDescription)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        vmMessage->SetPopupType(nPopup);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        vmMessage->SetDetail(sDetail);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(ra::ui::viewmodels::Popup nPopup, const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        vmMessage->SetDetail(sDetail);
        vmMessage->SetPopupType(nPopup);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        vmMessage->SetImage(nImageType, sImageName);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        std::unique_ptr<PopupMessageViewModel> vmMessage(new PopupMessageViewModel);
        vmMessage->SetTitle(sTitle);
        vmMessage->SetDescription(sDescription);
        vmMessage->SetDetail(sDetail);
        vmMessage->SetImage(nImageType, sImageName);
        return QueueMessage(vmMessage);
    }

    /// <summary>
    /// Gets the message associated to the specified unique identifier.
    /// </summary>
    /// <param name="nId">The unique identifier of the popup.</param>
    /// <returns>Requested popup view model, <c>nullptr</c> if not found.</returns>
    PopupMessageViewModel* GetMessage(int nId) noexcept
    {
        for (auto& pMessage : m_vPopupMessages)
        {
            if (pMessage->GetPopupId() == nId)
                return pMessage.get();
        }

        return nullptr;
    }

    /// <summary>
    /// Captures a screenshot with the associated message
    /// </summary>
    void CaptureScreenshot(int nMessageId, const std::wstring& sPath);

    /// <summary>
    /// Advances the frame counter to indicate the capture screen is no longer valid.
    /// </summary>
    void AdvanceFrame() noexcept { ++m_nFrameId; }

    /// <summary>
    /// Adds a score tracker for a leaderboard.
    /// </summary>
    ScoreTrackerViewModel& AddScoreTracker(ra::LeaderboardID nLeaderboardId);

    /// <summary>
    /// Removes the score tracker associated to the specified leaderboard.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the leaderboard associated to the tracker.</param>
    void RemoveScoreTracker(ra::LeaderboardID nLeaderboardId);

    /// <summary>
    /// Gets the score tracker associated to the specified leaderboard.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the leaderboard associated to the tracker.</param>
    /// <returns>Requested score tracker view model, <c>nullptr</c> if not found.</returns>
    ScoreTrackerViewModel* GetScoreTracker(ra::LeaderboardID nLeaderboardId) noexcept
    {
        for (auto& pTracker : m_vScoreTrackers)
        {
            if (ra::to_unsigned(pTracker->GetPopupId()) == nLeaderboardId)
                return pTracker.get();
        }

        return nullptr;
    }

    /// <summary>
    /// Adds a scoreboard for a leaderboard.
    /// </summary>
    void QueueScoreboard(ra::LeaderboardID nLeaderboardId, ScoreboardViewModel&& vmScoreboard);

    /// <summary>
    /// Gets the scoreboard associated to the specified leaderboard.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the leaderboard associated to the scoreboard.</param>
    /// <returns>Requested scoreboard view model, <c>nullptr</c> if not found.</returns>
    ScoreboardViewModel* GetScoreboard(ra::LeaderboardID nLeaderboardId) noexcept
    {
        for (auto& pScoreboard : m_vScoreboards)
        {
            if (ra::to_unsigned(pScoreboard.GetPopupId()) == nLeaderboardId)
                return &pScoreboard;
        }

        return nullptr;
    }

    /// <summary>
    /// Adds a challenge indicator for an achievement.
    /// </summary>
    ChallengeIndicatorViewModel& AddChallengeIndicator(ra::AchievementID nAchievementId, ra::ui::ImageType imageType, const std::string& sImageName);

    /// <summary>
    /// Removes the challenge indicator associated to the specified achievement.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the achievement associated to the indicator.</param>
    void RemoveChallengeIndicator(ra::AchievementID nAchievementId);
    
    /// <summary>
    /// Gets the challenge indicator associated to the specified achievement.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the achievement associated to the indicator.</param>
    /// <returns>Requested challenge indicator view model, <c>nullptr</c> if not found.</returns>
    const ChallengeIndicatorViewModel* GetChallengeIndicator(ra::AchievementID nAchievementId) const noexcept
    {
        for (auto& pIndicator : m_vChallengeIndicators)
        {
            if (ra::to_unsigned(pIndicator->GetPopupId()) == nAchievementId)
                return pIndicator.get();
        }

        return nullptr;
    }

    /// <summary>
    /// Clears all popups.
    /// </summary>
    virtual void ClearPopups();

    /// <summary>
    /// Sets the function to call when there's something to be rendered.
    /// </summary>
    void SetRenderRequestHandler(std::function<void()>&& fHandleRenderRequest)
    {
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        pImageRepository.RemoveNotifyTarget(*this);
        pImageRepository.AddNotifyTarget(*this);
        m_fHandleRenderRequest = std::move(fHandleRenderRequest);
    }

    /// <summary>
    /// Sets the function to call when there's something to be rendered.
    /// </summary>
    void SetShowRequestHandler(std::function<void()>&& fHandleShowRequest) noexcept
    {
        m_fHandleShowRequest = std::move(fHandleShowRequest);
    }

    /// <summary>
    /// Sets the function to call when there's something to be rendered.
    /// </summary>
    void SetHideRequestHandler(std::function<void()>&& fHandleHideRequest) noexcept
    {
        m_fHandleHideRequest = std::move(fHandleHideRequest);
    }

    /// <summary>
    /// Gets whether or not there is something visible to render on the overlay
    /// </summary>
    bool NeedsRender() const noexcept;

protected:
    ra::ui::viewmodels::OverlayViewModel m_vmOverlay;
    std::deque<std::unique_ptr<PopupMessageViewModel>> m_vPopupMessages;
    std::vector<std::unique_ptr<ScoreTrackerViewModel>> m_vScoreTrackers;
    std::vector<std::unique_ptr<ChallengeIndicatorViewModel>> m_vChallengeIndicators;
    std::deque<ScoreboardViewModel> m_vScoreboards;

    bool m_bIsRendering = false;
    bool m_bRenderRequestPending = false;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override { RefreshOverlay(); }
    void OnEndGameLoad() override { RefreshOverlay(); }

    // IImageRepository::NotifyTarget
    void OnImageChanged(ImageType, const std::string&) override
    {
        RequestRender();
    }

private:
    struct PopupLocations
    {
        int nTopLeftY;
        int nTopMiddleY;
        int nTopRightY;
        int nBottomLeftY;
        int nBottomMiddleY;
        int nBottomRightY;
    };

    static ra::ui::Position GetRenderLocation(const ra::ui::viewmodels::PopupViewModelBase& vmPopup,
        int nX, int nY, const ra::ui::drawing::ISurface& pSurface, const PopupLocations& pPopupLocations);
    static void AdjustLocationForPopup(PopupLocations& pPopupLocations, const ra::ui::viewmodels::PopupViewModelBase& vmPopup);

    void UpdateActiveMessage(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed);
    void UpdateActiveScoreboard(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed);
    void UpdateScoreTrackers(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed);
    void UpdateChallengeIndicators(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed);
    void UpdatePopup(ra::ui::drawing::ISurface& pSurface, const PopupLocations& pPopupLocations, double fElapsed, ra::ui::viewmodels::PopupViewModelBase& vmPopup);

    void UpdateOverlay(ra::ui::drawing::ISurface& pSurface, double fElapsed);

    void ProcessScreenshots();
    std::unique_ptr<ra::ui::drawing::ISurface> RenderScreenshot(const ra::ui::drawing::ISurface& pClientSurface, const PopupMessageViewModel& vmPopup);

    bool m_bRedrawAll = false;
    std::chrono::steady_clock::time_point m_tLastRender{};
    std::chrono::steady_clock::time_point m_tLastRequestRender{};
    std::function<void()> m_fHandleRenderRequest;
    std::function<void()> m_fHandleShowRequest;
    std::function<void()> m_fHandleHideRequest;

    std::atomic<int> m_nPopupId{ 0 };

    int m_nFrameId = 0;

    struct Screenshot
    {
        int nFrameId = 0;
        std::unique_ptr<ra::ui::drawing::ISurface> pScreen;
        std::map<int, std::wstring> vMessages;
    };
    std::vector<Screenshot> m_vScreenshotQueue;
    std::mutex m_pScreenshotQueueMutex;
    bool m_bProcessingScreenshots = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_MANAGER_H
