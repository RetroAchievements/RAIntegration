#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
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
    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType = PopupInfo, HBITMAP hImg = nullptr) :
        m_sMessageTitle(ra::Widen(sTitle)),
        m_sMessageSubtitle(ra::Widen(sSubtitle)),
        m_nMessageType(nMsgType),
        m_hMessageImage(hImg)
    {
    }
public:
    const std::wstring& Title() const { return m_sMessageTitle; }
    const std::wstring& Subtitle() const { return m_sMessageSubtitle; }
    PopupMessageType Type() const { return m_nMessageType; }
    HBITMAP Image() const { return m_hMessageImage; }

private:
    const std::wstring m_sMessageTitle;
    const std::wstring m_sMessageSubtitle;
    const PopupMessageType m_nMessageType;
    const HBITMAP m_hMessageImage;
};

class AchievementPopup
{
public:

public:
    AchievementPopup();

    void Update(ControllerInput input, float fDelta, bool bFullScreen, bool bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void AddMessage(const MessagePopup& msg);
    float GetYOffsetPct() const;

    //bool IsActive() const						{ return( m_vMessages.size() > 0 ); }
    bool MessagesPresent() const { return(m_vMessages.size() > 0); }
    const MessagePopup& ActiveMessage() const { return m_vMessages.front(); }

    void Clear();
    void PlayAudio();

private:
    std::queue<MessagePopup> m_vMessages;
    float m_fTimer;
};

//extern AchievementPopup g_PopupWindow;


#endif // !RA_ACHIEVEMENTPOPUP_H
