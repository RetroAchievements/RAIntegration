#ifndef RA_LEADERBOARDPOPUP_H
#define RA_LEADERBOARDPOPUP_H
#pragma once

#include "RA_AchievementOverlay.h"

//	Graphic to display current leaderboard progress

class LeaderboardPopup
{
public:
    enum PopupState
    {
        State_ShowingProgress,
        State_ShowingScoreboard,
        State__MAX
    };

public:
    void Update(_UNUSED ControllerInput, float fDelta, _UNUSED BOOL, BOOL bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void Reset();
    BOOL Activate(ra::LeaderboardID nLBID);
    BOOL Deactivate(ra::LeaderboardID nLBID);

    void ShowScoreboard(ra::LeaderboardID nLBID);

private:
    float GetOffsetPct() const;

private:
    PopupState m_nState{};
    float m_fScoreboardShowTimer{};
    std::vector<unsigned int> m_vActiveLBIDs;
    std::queue<unsigned int> m_vScoreboardQueue;
};


#endif // !RA_LEADERBOARDPOPUP_H
