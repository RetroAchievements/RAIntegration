#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"

#include "services\ImageRepository.h"

//	Graphic to display an obtained achievement
class MessagePopup
{
public:
    enum class Type
    {
        Login,
        Info,
        AchievementUnlocked,
        AchievementError,
        LeaderboardInfo,
        LeaderboardCancel,
        Message,
    };

    explicit MessagePopup(const std::string& sTitle, const std::string& sSubtitle, Type nMsgType,
        ra::services::ImageType nImageType, const std::string& sImageName ) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(nImageType, sImageName)
    {
    }

    explicit MessagePopup(const std::string& sTitle, const std::string& sSubtitle = "", Type nMsgType = Type::Info) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType)
    {
    }

public:
    _NODISCARD auto& Title() const noexcept { return m_sMessageTitle; }
    _NODISCARD auto& Subtitle() const noexcept { return m_sMessageSubtitle; }
    _NODISCARD auto GetType() const noexcept { return m_nMessageType; }
    _NODISCARD auto Image() const { return m_hMessageImage.GetHBitmap(); }

private:
    const std::string m_sMessageTitle;
    const std::string m_sMessageSubtitle;
    const Type m_nMessageType{ Type::Info };
    const ra::services::ImageReference m_hMessageImage{ ra::services::ImageType::None, "" };
};

class AchievementPopup
{
    // Used by more than one function
    inline static constexpr auto FINISH_AT  = 5.0F;
public:
    void Update(_UNUSED ControllerInput& __restrict, float fDelta, _UNUSED bool, bool bPaused);
    void Render(_In_ HDC__* __restrict hDC, _In_ RECT& __restrict rcDest);

    void AddMessage(const MessagePopup& msg);
    _NODISCARD float GetYOffsetPct() const noexcept;

    _NODISCARD auto MessagesPresent() const { return(!m_vMessages.empty()); }
    _NODISCARD auto& ActiveMessage() const { return m_vMessages.front(); }

    void Clear();
    void PlayAudio();

private:
    std::queue<MessagePopup> m_vMessages;
    float m_fTimer{ 0.0F };
};

#endif // !RA_ACHIEVEMENTPOPUP_H
