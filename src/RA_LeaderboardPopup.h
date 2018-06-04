#ifndef RA_LEADERBOARDPOPUP_H
#define RA_LEADERBOARDPOPUP_H
#pragma once

#include <unordered_set>
#include "RA_AchievementOverlay.h"


//	Graphic to display current leaderboard progress

class LeaderboardPopup
{
    using LeaderboardIDs  = std::unordered_set<LeaderboardID>;
    using ScoreboardQueue = std::queue<LeaderboardID>;

public:
    enum PopupState
    {
        State_ShowingProgress,
        State_ShowingScoreboard,
        State__MAX
    };

public:
    LeaderboardPopup() noexcept;

    void Update(ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void Reset();
    BOOL Activate(LeaderboardID nLBID);
    BOOL Deactivate(LeaderboardID nLBID);

    void ShowScoreboard(LeaderboardID nLBID);

private:
    float GetOffsetPct() const;

private:
    // It's not very apparent what the default values are they are move assigned instead of default constructed
    // It's whatever the value is at "0"
    // for m_nState it's LeaderboardPopup::PopupState::State_ShowingProgress
    // for float it's (float)(0.0F)
    // just hover over the assignment operator, this eliminates ambiguity as far as the compiler is concerned
    PopupState m_nState = LeaderboardPopup::PopupState{};

    
    float m_fScoreboardShowTimer = float{};
    LeaderboardIDs m_vActiveLBIDs;
    ScoreboardQueue m_vScoreboardQueue;
};


#endif // !RA_LEADERBOARDPOPUP_H
