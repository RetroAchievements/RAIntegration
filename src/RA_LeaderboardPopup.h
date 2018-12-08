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
    void Render(_In_ ra::ui::drawing::ISurface& pSurface);

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
