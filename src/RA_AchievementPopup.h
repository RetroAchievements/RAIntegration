#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

#include <queue>
#include "ra_fwd.h"
#include "services/ImageRepository.h"

namespace ra {

enum class PopupMessageType
{
    Login,
    Info,
    AchievementUnlocked,
    AchievementError,
    LeaderboardInfo,
    LeaderboardCancel,
    Message
};

namespace enum_sizes {

inline constexpr auto NUM_MESSAGE_TYPES{ 7U };

} // namespace enum_sizes
} // namespace ra

//	Graphic to display an obtained achievement
class MessagePopup
{
public:
     MessagePopup(const std::string& sTitle, const std::string& sSubtitle,
                  ra::PopupMessageType nMsgType, ra::services::ImageType nImageType,
                  const std::string& sImageName ) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(nImageType, sImageName)
    {
    }

    MessagePopup(const std::string& sTitle, const std::string& sSubtitle, ra::PopupMessageType nMsgType = ra::PopupMessageType::Info) :
        m_sMessageTitle(sTitle),
        m_sMessageSubtitle(sSubtitle),
        m_nMessageType(nMsgType),
        m_hMessageImage(ra::services::ImageType::None, "")
    {
    }

public:
    _NODISCARD inline auto& Title() const { return m_sMessageTitle; }
    _NODISCARD inline auto& Subtitle() const { return m_sMessageSubtitle; }
    _NODISCARD inline auto& Type() const { return m_nMessageType; }
    _NODISCARD inline auto Image() const { return m_hMessageImage.GetHBitmap(); }

private:
    const std::string m_sMessageTitle;
    const std::string m_sMessageSubtitle;
    const ra::PopupMessageType m_nMessageType{ ra::PopupMessageType::Info };
    const ra::services::ImageReference m_hMessageImage;
};

struct ControllerInput;
class AchievementPopup
{
public:
    /*_NORETURN */void Update(_In_ ControllerInput& input, _In_ float fDelta, _In_ bool bFullScreen, _In_ bool bPaused);
    /*_NORETURN */void Render(_In_ HDC hDC, _Inout_ RECT& rcDest);

    void AddMessage(MessagePopup&& msg) noexcept;
    _NODISCARD float GetYOffsetPct() const;

    //bool IsActive() const						{ return( m_vMessages.size() > 0 ); }
    _NODISCARD bool MessagesPresent() const { return(m_vMessages.size() > 0); }
    _NODISCARD const MessagePopup& ActiveMessage() const { return m_vMessages.front(); }

    void Clear() noexcept;
    void PlayAudio();

private:
    std::queue<MessagePopup> m_vMessages;
    float m_fTimer{};
};

#endif // !RA_ACHIEVEMENTPOPUP_H
