#ifndef _PROGRESSPOPUP_H_
#define _PROGRESSPOPUP_H_

#include <wtypes.h>
#include "RA_AchievementOverlay.h"

//	Graphic to display an progress towards an achievement


#define OVERLAY_MESSAGE_QUEUE_SIZE (5)

class ProgressPopup
{
public:
	ProgressPopup();

	void Update( ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused );
	void Render( HDC hDC, RECT& rcDest );

	void AddMessage( const char* sTitle, const char* sMessage, int nMessageType=0, HBITMAP hImage=NULL );
	float GetYOffsetPct() const;

	BOOL IsActive() const				{ return m_sMessageTitleQueue[0][0] != '\0'; }
	const char* GetTitle() const		{ return m_sMessageTitleQueue[0]; }
	const char* GetDesc() const			{ return m_sMessageDescQueue[0]; }
	unsigned int GetMessageType() const	{ return m_nMessageType[0]; }
	HBITMAP GetImage() const			{ return m_hMessageImage[0]; }

	void SuppressNextDeltaUpdate()		{ m_bSuppressDeltaUpdate = true; }

	void Clear();

private:
	void NextMessage();

private:
	float m_fTimer;
	char m_sMessageTitleQueue[OVERLAY_MESSAGE_QUEUE_SIZE][1024];
	char m_sMessageDescQueue[OVERLAY_MESSAGE_QUEUE_SIZE][1024];
	HBITMAP m_hMessageImage[OVERLAY_MESSAGE_QUEUE_SIZE];
	int m_nMessageType[OVERLAY_MESSAGE_QUEUE_SIZE];
	BOOL m_bSuppressDeltaUpdate;
};

//extern ProgressPopup g_ProgressPopup;

#endif // _PROGRESSPOPUP_H_