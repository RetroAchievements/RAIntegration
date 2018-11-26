#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"

#include "ui\ImageReference.hh"

#include "ui\drawing\gdi\ResourceRepository.hh"

// Graphic to display an obtained achievement
enum class PopupMessageType : std::size_t
{
    Login,
    Info,
    AchievementUnlocked,
    AchievementError,
    LeaderboardInfo,
    LeaderboardCancel,
    Message,
};

class MessagePopup
{
public:
    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, PopupMessageType nMsgType,
                 ra::ui::ImageType nImageType, const std::string& sImageName) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(nImageType, sImageName)
    {}

    MessagePopup(const std::string& sTitle, const std::string& sSubtitle,
                 PopupMessageType nMsgType = PopupMessageType::Info) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(ra::ui::ImageType::None, "")
    {}

public:
    const std::string& Title() const { return m_sMessageTitle; }
    const std::string& Subtitle() const { return m_sMessageSubtitle; }
    PopupMessageType Type() const { return m_nMessageType; }
    const ra::ui::ImageReference& Image() const { return m_hMessageImage; }

private:
    const std::string m_sMessageTitle;
    const std::string m_sMessageSubtitle;
    const PopupMessageType m_nMessageType;
    const ra::ui::ImageReference m_hMessageImage;
};

class AchievementPopup
{
public:
    void Update(_UNUSED ControllerInput, float fDelta, _UNUSED bool, bool bPaused);
    void Render(_In_ HDC hDC, _In_ const RECT& rcDest);

    void AddMessage(const MessagePopup& msg);
    float GetYOffsetPct() const;

    bool MessagesPresent() const { return (m_vMessages.size() > 0); }
    const MessagePopup& ActiveMessage() const { return m_vMessages.front(); }

    void Clear();
    void PlayAudio();

private:
    ra::ui::drawing::gdi::ResourceRepository m_pResourceRepository;

    std::queue<MessagePopup> m_vMessages;
    float m_fTimer{};
};

#endif // !RA_ACHIEVEMENTPOPUP_H
