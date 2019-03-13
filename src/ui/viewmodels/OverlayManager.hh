#ifndef RA_UI_OVERLAY_MANAGER
#define RA_UI_OVERLAY_MANAGER

#include "PopupMessageViewModel.hh"
#include "ScoreboardViewModel.hh"
#include "ScoreTrackerViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayManager {
public:    
    /// <summary>
    /// Updates the overlay.
    /// </summary>
    /// <param name="fElapsed">The amount of seconds that have passed.</param>
    void Update(double fElapsed);

    /// <summary>
    /// Renders the overlay.
    /// </summary>
    void Render(ra::ui::drawing::ISurface& pSurface) const;
    
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
    ScoreTrackerViewModel& AddScoreTracker(ra::LeaderboardID nLeaderboardId)
    {
        auto pScoreTracker = std::make_unique<ScoreTrackerViewModel>();
        pScoreTracker->SetPopupId(nLeaderboardId);
        pScoreTracker->UpdateRenderImage(0.0);
        return *m_vScoreTrackers.emplace_back(std::move(pScoreTracker));
    }

    /// <summary>
    /// Removes the score tracker associated to the specified leaderboard.
    /// </summary>
    /// <param name="nLeaderboardId">The unique identifier of the leaderboard associated to the tracker.</param>
    void RemoveScoreTracker(ra::LeaderboardID nLeaderboardId)
    {
        for (auto pIter = m_vScoreTrackers.begin(); pIter != m_vScoreTrackers.end(); ++pIter)
        {
            if (ra::to_unsigned((*pIter)->GetPopupId()) == nLeaderboardId)
            {
                m_vScoreTrackers.erase(pIter);
                break;
            }
        }
    }

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
    void ClearPopups();

private:
    void UpdateActiveMessage(double fElapsed);
    void UpdateActiveScoreboard(double fElapsed);

    std::deque<PopupMessageViewModel> m_vPopupMessages;
    std::vector<std::unique_ptr<ScoreTrackerViewModel>> m_vScoreTrackers;
    std::deque<ScoreboardViewModel> m_vScoreboards;
    std::atomic<int> m_nPopupId{ 0 };
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_MANAGER
