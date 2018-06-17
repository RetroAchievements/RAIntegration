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
    MessagePopup() noexcept = default;
    ~MessagePopup() noexcept { DeleteBitmap(m_hMessageImage); }
    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType = PopupInfo, HBITMAP hImg = nullptr) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(hImg)
    {
    }
    MessagePopup(const MessagePopup&) = delete;
    MessagePopup& operator=(const MessagePopup&) = delete;
    MessagePopup(MessagePopup&&) noexcept = default;
    MessagePopup& operator=(MessagePopup&&) noexcept = default;
public:
    // This class may as well be a struct
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    // Just return references
    auto& Title() const { return m_sMessageTitle; }
    auto& Subtitle() const { return m_sMessageSubtitle; }
    auto& Type() const { return m_nMessageType; }
    auto& Image() const { return m_hMessageImage; }
#pragma warning(pop)

private:
    // This was wrong in so many ways, you can't use move semantics on const!
    // Have you ever seen "const class_type&&" before? No, of course not, it's undefined.
    std::string m_sMessageTitle;
    std::string m_sMessageSubtitle;
    PopupMessageType m_nMessageType ={};
    HBITMAP m_hMessageImage ={};
};

class AchievementPopup
{
public:

public:
    AchievementPopup();

    void Update(ControllerInput& input, float fDelta, bool bFullScreen, bool bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void AddMessage(const MessagePopup& msg);
    float GetYOffsetPct() const;

    //bool IsActive() const						{ return( m_vMessages.size() > 0 ); }
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    bool MessagesPresent() const { return(m_vMessages.size() > 0); }
    const MessagePopup& ActiveMessage() const { return m_vMessages.front(); }
#pragma warning(pop)

    void Clear();
    void PlayAudio();

private:
    std::queue<MessagePopup> m_vMessages;
    float m_fTimer;
};

//extern AchievementPopup g_PopupWindow;


#endif // !RA_ACHIEVEMENTPOPUP_H
