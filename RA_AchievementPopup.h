#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"

//	Graphic to display an obtained achievement
enum PopupMessageType
{
	PopupLogin,
	PopupInfo,
	PopupAchievementUnlocked,
	PopupAchievementError,
	PopupLeaderboardInfo,
	PopupLeaderboardCancel,
	PopupMessage,

	NumMessageTypes
};

class MessagePopup
{
public:
	MessagePopup( const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType = PopupInfo, HBITMAP hImg = nullptr ) :
		m_sMessageTitle( sTitle ),
		m_sMessageSubtitle( sSubtitle ),
		m_nMessageType( nMsgType ),
		m_hMessageImage( hImg )
	{}
public:
	const std::string& Title() const	{ return m_sMessageTitle; }
	const std::string& Subtitle() const	{ return m_sMessageSubtitle; }
	PopupMessageType Type() const		{ return m_nMessageType; }
	HBITMAP Image() const				{ return m_hMessageImage; }

private:
	const std::string m_sMessageTitle;
	const std::string m_sMessageSubtitle;
	const PopupMessageType m_nMessageType;
	const HBITMAP m_hMessageImage;
};

class AchievementPopup
{
public:

public:
	AchievementPopup();

	void Update( ControllerInput input, float fDelta, bool bFullScreen, bool bPaused );
	void Render( HDC hDC, RECT& rcDest );

	void AddMessage( const MessagePopup& msg );
	float GetYOffsetPct() const;

	//bool IsActive() const						{ return( m_vMessages.size() > 0 ); }
	bool MessagesPresent() const				{ return( m_vMessages.size() > 0 ); }
	const MessagePopup& ActiveMessage() const	{ return m_vMessages.front(); }

	void Clear();
	void PlayAudio();

private:
	std::queue<MessagePopup> m_vMessages;
	float m_fTimer;
};

//extern AchievementPopup g_PopupWindow;
