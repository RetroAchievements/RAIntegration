#ifndef RA_UI_OVERLAY_MANAGER_H
#define RA_UI_OVERLAY_MANAGER_H

#include "RA_Interface.h"

#include "OverlayViewModel.hh"
#include "PopupMessageViewModel.hh"
#include "ScoreboardViewModel.hh"
#include "ScoreTrackerViewModel.hh"

#include "ui\ImageReference.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayManager : protected ra::ui::IImageRepository::NotifyTarget {
public:    
    GSL_SUPPRESS_F6 OverlayManager() = default;
    ~OverlayManager() noexcept = default;
    OverlayManager(const OverlayManager&) noexcept = delete;
    OverlayManager& operator=(const OverlayManager&) noexcept = delete;
    OverlayManager(OverlayManager&&) noexcept = delete;
    OverlayManager& operator=(OverlayManager&&) noexcept = delete;

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
    int QueueMessage(PopupMessageViewModel&& pMessage);

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        return QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        vmMessage.SetDetail(sDetail);
        return QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        vmMessage.SetImage(nImageType, sImageName);
        return QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        vmMessage.SetDetail(sDetail);
        vmMessage.SetImage(nImageType, sImageName);
        return QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Gets the message associated to the specified unique identifier.
    /// </summary>
    /// <param name="nId">The unique identifier of the popup.</param>
    /// <returns>Requested popup view model, <c>nullptr</c> if not found.</returns>
    PopupMessageViewModel* GetMessage(int nId)
    {
        for (auto& pMessage : m_vPopupMessages)
        {
            if (pMessage.GetPopupId() == nId)
                return &pMessage;
        }

        return nullptr;
    }

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
    ScoreboardViewModel* GetScoreboard(ra::LeaderboardID nLeaderboardId)
    {
        for (auto& pScoreboard : m_vScoreboards)
        {
            if (ra::to_unsigned(pScoreboard.GetPopupId()) == nLeaderboardId)
                return &pScoreboard;
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
    void SetShowRequestHandler(std::function<void()>&& fHandleShowRequest)
    {
        m_fHandleShowRequest = std::move(fHandleShowRequest);
    }

    /// <summary>
    /// Sets the function to call when there's something to be rendered.
    /// </summary>
    void SetHideRequestHandler(std::function<void()>&& fHandleHideRequest)
    {
        m_fHandleHideRequest = std::move(fHandleHideRequest);
    }

    bool NeedsRender() const noexcept;

protected:
    ra::ui::viewmodels::OverlayViewModel m_vmOverlay;
    std::deque<PopupMessageViewModel> m_vPopupMessages;
    std::vector<std::unique_ptr<ScoreTrackerViewModel>> m_vScoreTrackers;
    std::deque<ScoreboardViewModel> m_vScoreboards;

    bool m_bIsRendering = false;
    bool m_bRenderRequestPending = false;

    void OnImageChanged(ImageType, const std::string&) override
    {
        RequestRender();
    }

private:
    void UpdateActiveMessage(ra::ui::drawing::ISurface& pSurface, double fElapsed);
    void UpdateActiveScoreboard(ra::ui::drawing::ISurface& pSurface, double fElapsed);
    void UpdateScoreTrackers(ra::ui::drawing::ISurface& pSurface, double fElapsed);
    void UpdatePopup(ra::ui::drawing::ISurface& pSurface, double fElapsed, ra::ui::viewmodels::PopupViewModelBase& vmPopup);

    void UpdateOverlay(ra::ui::drawing::ISurface& pSurface, double fElapsed);

    bool m_bRedrawAll = false;
    std::chrono::steady_clock::time_point m_tLastRender{};
    std::function<void()> m_fHandleRenderRequest;
    std::function<void()> m_fHandleShowRequest;
    std::function<void()> m_fHandleHideRequest;

    std::atomic<int> m_nPopupId{ 0 };
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_MANAGER_H
