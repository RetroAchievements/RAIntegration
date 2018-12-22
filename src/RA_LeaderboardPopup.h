#ifndef RA_LEADERBOARDPOPUP_H
#define RA_LEADERBOARDPOPUP_H
#pragma once

#include "RA_AchievementOverlay.h"

#include "ui\drawing\ISurface.hh"

//	Graphic to display current leaderboard progress

class LeaderboardPopup
{
    enum class PopupState
    {
        ShowingProgress,
        ShowingScoreboard
    };

public:
    void Update(_In_ float fDelta);
    /* clang-format off */
    // Mark nTextColorr as const. It's assigned so it can't be const.
    // Comment the suppression out if changes are made then put it back if it still gives a false alarm
    GSL_SUPPRESS(con.4) 
    void Render(_In_ ra::ui::drawing::ISurface& pSurface);
    /* clang-format on */

    void Reset();
    BOOL Activate(_In_ ra::LeaderboardID nLBID);
    BOOL Deactivate(_In_ ra::LeaderboardID nLBID);

    void ShowScoreboard(ra::LeaderboardID nLBID);

private:
    float GetOffsetPct() const noexcept;

private:
    PopupState m_nState{};
    float m_fScoreboardShowTimer{};
    std::vector<unsigned int> m_vActiveLBIDs;
    std::queue<unsigned int> m_vScoreboardQueue;

    std::unique_ptr<ra::ui::drawing::ISurface> m_pScoreboardSurface;
};

#endif // !RA_LEADERBOARDPOPUP_H
