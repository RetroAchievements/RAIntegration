#ifndef RA_LEADERBOARDPOPUP_H
#define RA_LEADERBOARDPOPUP_H
#pragma once


#include "RA_AchievementOverlay.h"

//	Graphic to display current leaderboard progress

namespace ra {

enum class PopupState { ShowingProgress, ShowingScoreboard };

namespace enum_sizes {

_CONSTANT_VAR POPUPSTATE_MAX{ 2U };

} // namespace enum_sizes
} // namespace ra

class LeaderboardPopup
{
public:
    LeaderboardPopup();

    void Update(ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void Reset();
    BOOL Activate(ra::LeaderboardID nLBID);
    BOOL Deactivate(ra::LeaderboardID nLBID);

    void ShowScoreboard(ra::LeaderboardID nLBID);

private:
    float GetOffsetPct() const;

private:
    ra::PopupState m_nState{};
    float m_fScoreboardShowTimer{};
    std::vector<unsigned int> m_vActiveLBIDs;
    std::queue<unsigned int> m_vScoreboardQueue;
};


#endif // !RA_LEADERBOARDPOPUP_H
