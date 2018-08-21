#ifndef RA_ACHIEVEMENTPOPUP_H
#define RA_ACHIEVEMENTPOPUP_H
#pragma once

// TBD: Should MessagePopup be forward declared to get rid of include queue?
#include <queue>
#include "ra_fwd.h"

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
    // Currently made the input a hard reference to a HBITMAP so it at least doesn't leak here

    MessagePopup(_In_ const std::string& sMessageTitle,
                 _In_ const std::string& sMessageSubtitle,
                 _In_ const ra::PopupMessageType nMessageType = ra::PopupMessageType::Info,
                 _In_ const HBITMAP& hMessageImage = nullptr) noexcept;

    // TBD: Figure out when to free the HBITMAP, it currently makes the user image vanish if deleted after the
    //      lifetime scope of a MessagePopup rvalue.
    ~MessagePopup() noexcept = default;
    MessagePopup(const MessagePopup&) = delete;
    MessagePopup& operator=(const MessagePopup&) = delete;
    MessagePopup(MessagePopup&& b) noexcept;
    MessagePopup& operator=(MessagePopup&&) noexcept = delete; // deleted since all members are const

public:
    _NODISCARD inline auto& Title() const { return m_sMessageTitle; }
    _NODISCARD inline auto& Subtitle() const { return m_sMessageSubtitle; }
    _NODISCARD inline auto Type() const { return m_nMessageType; }
    _NODISCARD inline auto& Image() const { return m_hMessageImage; }

private:
    const std::string m_sMessageTitle;
    const std::string m_sMessageSubtitle;
    const ra::PopupMessageType m_nMessageType{ ra::PopupMessageType::Info };
    const HBITMAP m_hMessageImage{};
};

struct ControllerInput;
class AchievementPopup
{
public:
    void Update(ControllerInput& input, float fDelta, bool bFullScreen, bool bPaused);
    /*_NORETURN */void Render(_In_ HDC hDC, _Inout_ RECT& rcDest);

    void AddMessage(MessagePopup&& msg);
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

//extern AchievementPopup g_PopupWindow;


#endif // !RA_ACHIEVEMENTPOPUP_H
