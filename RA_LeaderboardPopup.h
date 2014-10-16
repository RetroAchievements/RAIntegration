#pragma once

#include <wtypes.h>
#include "RA_AchievementOverlay.h"
#include <vector>
#include <queue>

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
	LeaderboardPopup();

	void Update( ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused );
	void Render( HDC hDC, RECT& rcDest );

	void Reset();
	BOOL Activate( unsigned int nLBID );
	BOOL Deactivate( unsigned int nLBID );

	void ShowScoreboard( unsigned int nLBID );

private:
	float GetOffsetPct() const;

private:
	PopupState m_nState;
	float m_fScoreboardShowTimer;
	std::vector<unsigned int> m_vActiveLBIDs;
	std::queue<unsigned int> m_vScoreboardQueue;
};
