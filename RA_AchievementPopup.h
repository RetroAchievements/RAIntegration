#pragma once

#include <wtypes.h>
#include "RA_AchievementOverlay.h"
#include <queue>

namespace
{
	enum OVERLAY_MESSAGE_TYPE
	{
		MSG_LOGIN,
		MSG_INFO,
		MSG_ACHIEVEMENT_UNLOCKED,
		MSG_ACHIEVEMENT_ERROR,
		MSG_LEADERBOARD_INFO,
		MSG_LEADERBOARD_CANCEL,

		MSG__MAX
	};
};

//	Graphic to display an obtained achievement

#define OVERLAY_MESSAGE_QUEUE_SIZE (5)

class AchievementPopup
{
public:
	AchievementPopup();

	void Update( ControllerInput input, float fDelta, bool bFullScreen, bool bPaused );
	void Render( HDC hDC, RECT& rcDest );

	void AddMessage( const char* sTitle, const char* sMessage, int nMessageType=MSG_INFO, HBITMAP hImage=NULL );
	float GetYOffsetPct() const;

	bool IsActive() const				{ return ( m_vMessages.size() > 0 ); }
	const char* GetTitle() const		{ return m_vMessages.front().m_sMessageTitle; }
	const char* GetDesc() const			{ return m_vMessages.front().m_sMessageDesc; }
	unsigned int GetMessageType() const	{ return m_vMessages.front().m_nMessageType; }
	HBITMAP GetImage() const			{ return m_vMessages.front().m_hMessageImage; }

	void SuppressNextDeltaUpdate()		{ m_bSuppressDeltaUpdate = true; }

	void Clear();
	void PlayAudio();

private:
	void NextMessage();

private:
	struct MessagePopup
	{
		char m_sMessageTitle[1024];
		char m_sMessageDesc[1024];
		HBITMAP m_hMessageImage;
		int m_nMessageType;
	};

	std::queue<MessagePopup> m_vMessages;

	float m_fTimer;
	bool m_bSuppressDeltaUpdate;
};

//extern AchievementPopup g_PopupWindow;
