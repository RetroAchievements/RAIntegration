#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

#include <queue>
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

// Making this non-const with copying deleted, the rvalue references are leaking.
//	Graphic to display an obtained achievement
class MessagePopup
{
public:
    MessagePopup() noexcept = default;
    MessagePopup(const std::string& sTitle, const std::string& sSubtitle,
                 ra::PopupMessageType nMsgType, ra::services::ImageType nImageType,
                 const std::string& sImageName) :
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
    ~MessagePopup() noexcept = default; /*no pointers*/
    MessagePopup(const MessagePopup&) = delete; /* can't delete it but shouldn't use it */
    MessagePopup& operator=(const MessagePopup&) = delete; /*all members are const*/
    /* Seems to do the trick, now we have expected of 1 Message popups in the queue instead of 2 */
    MessagePopup(MessagePopup&& b) noexcept :
        m_sMessageTitle{ std::move(b.m_sMessageTitle)},
        m_sMessageSubtitle{ std::move(b.m_sMessageSubtitle) },
        m_nMessageType{b.m_nMessageType},
        m_hMessageImage{std::move(b.m_hMessageImage)}
    {
        m_nMessageType = ra::PopupMessageType::Info;
    }
    MessagePopup& operator=(MessagePopup&&) noexcept = delete; /*never saw a need to assign*/
public:
    _NODISCARD inline auto& Title() const { return m_sMessageTitle; }
    _NODISCARD inline auto& Subtitle() const { return m_sMessageSubtitle; }
    _NODISCARD inline auto Type() const { return m_nMessageType; }
    _NODISCARD inline auto Image() const { return m_hMessageImage.GetHBitmap(); }

private:
    std::string m_sMessageTitle;
    std::string m_sMessageSubtitle;
    ra::PopupMessageType m_nMessageType{ ra::PopupMessageType::Info };
    ra::services::ImageReference m_hMessageImage;
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
