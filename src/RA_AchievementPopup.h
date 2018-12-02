#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

#include "ra_fwd.h"

#include "ui\drawing\ISurface.hh"

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
    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType, 
        ra::ui::ImageType nImageType, const std::string& sImageName ) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(nImageType, sImageName)
    {
    }

    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType = PopupInfo) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(ra::ui::ImageType::None, "")
    {
    }

    const std::string& Title() const noexcept { return m_sMessageTitle; }
    const std::string& Subtitle() const noexcept { return m_sMessageSubtitle; }
    PopupMessageType Type() const noexcept { return m_nMessageType; }
    const ra::ui::ImageReference& Image() const noexcept { return m_hMessageImage; }

    const ra::ui::drawing::ISurface& GetRendered();

private:
    const std::string m_sMessageTitle;
    const std::string m_sMessageSubtitle;
    const PopupMessageType m_nMessageType;
    ra::ui::ImageReference m_hMessageImage;
    std::unique_ptr<ra::ui::drawing::ISurface> m_pSurface;
};

class AchievementPopup
{
public:
    AchievementPopup();

    void Update(_UNUSED ControllerInput, float fDelta, _UNUSED bool, bool bPaused);
    void Render(_In_ HDC hDC, _In_ const RECT& rcDest);

    void AddMessage(MessagePopup&& msg);
    float GetYOffsetPct() const;

    bool MessagesPresent() const { return(m_vMessages.size() > 0); }

    void Clear();
    void PlayAudio();

private:
    std::queue<MessagePopup> m_vMessages;
    float m_fTimer;
};

#endif // !RA_ACHIEVEMENTPOPUP_H
