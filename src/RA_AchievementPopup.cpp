#include "RA_AchievementPopup.h"

#ifndef _MEMORY_
#include <memory>
#endif // !_MEMORY_

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"
#include "RA_Interface.h"
#include "ra_utility.h"

#ifndef _INC_MMSYSTEM
#include <MMSystem.h> // PlaySound
#endif // !WIN32_LEAN_AND_MEAN

namespace ra {

_CONSTANT_VAR POPUP_DIST_Y_TO_PCT   = 0.856F; // Where on screen to end up
_CONSTANT_VAR POPUP_DIST_Y_FROM_PCT = 0.4F;   // Amount of screens to travel
_CONSTANT_VAR FONT_TO_USE           = _T("Tahoma");

_CONSTANT_VAR FONT_SIZE_TITLE    = 32;
_CONSTANT_VAR FONT_SIZE_SUBTITLE = 28;

_CONSTANT_VAR START_AT   = 0.0F;
_CONSTANT_VAR APPEAR_AT  = 0.8F;
_CONSTANT_VAR FADEOUT_AT = 4.2F;
_CONSTANT_VAR FINISH_AT  = 5.0F;

inline constexpr std::array<LPCTSTR, enum_sizes::NUM_MESSAGE_TYPES> MSG_SOUND
{
    _T("./Overlay/login.wav"),
    _T("./Overlay/info.wav"),
    _T("./Overlay/unlock.wav"),
    _T("./Overlay/acherror.wav"),
    _T("./Overlay/lb.wav"),
    _T("./Overlay/lbcancel.wav"),
    _T("./Overlay/message.wav"),
};

} // namespace ra

void AchievementPopup::PlayAudio()
{
    ASSERT(MessagesPresent());	//	ActiveMessage() dereferences!
    // maybe it doesn't like rvalues
    const auto myType{ ActiveMessage().Type() };
    PlaySound(ra::MSG_SOUND.at(ra::etoi(myType)), nullptr, SND_FILENAME | SND_ASYNC);
}

// not sure how this worked before with copying implicitly deleted
void AchievementPopup::AddMessage(MessagePopup&& msg) noexcept
{
    m_vMessages.push(std::move(msg));
    PlayAudio();
}

void AchievementPopup::Update(ControllerInput& input, float fDelta, bool bFullScreen, bool bPaused)
{
    if (bPaused)
        fDelta = 0.0F;
    fDelta = std::clamp(fDelta, 0.0F, 0.3F);	//	Limit this!
    if (!m_vMessages.empty())
    {
        m_fTimer += fDelta;
        if (m_fTimer >= ra::FINISH_AT)
        {
            m_vMessages.pop();
            m_fTimer = 0.0F;
        }
    }
}

float AchievementPopup::GetYOffsetPct() const
{
    float fVal{ 0.0F };
    //	Fading in.
    if (m_fTimer < ra::APPEAR_AT)
        fVal = ra::sqr(ra::APPEAR_AT - m_fTimer); // Quadratic
    //	Faded in - held
    else if (m_fTimer < ra::FADEOUT_AT)
        fVal = 0.0F;
    //	Fading out
    else if (m_fTimer < ra::FINISH_AT)
        fVal = ra::sqr(ra::FADEOUT_AT - m_fTimer); // Quadratic
    //	Finished!
    else
        fVal = 1.0F;

    return fVal;
}

void AchievementPopup::Render(HDC hDC, RECT& rcDest)
{
    if (!MessagesPresent())
        return;

    const auto nPixelWidth{ rcDest.right - rcDest.left };

    //SetBkColor( hDC, RGB( 0, 212, 0 ) );
    SetBkColor(hDC, COL_TEXT_HIGHLIGHT);
    SetTextColor(hDC, COL_POPUP);

    // RAII looks like it can be applied here easily
    auto font_deleter =[](HFONT hfont) noexcept
    {
        if (hfont)
        {
            DeleteFont(hfont);
            hfont = nullptr;
        }
    };
    // TBD: Can't use decltype(&DeleteFont) because it's a macro so we used a lambda instead, later there will be a
    // generic class for all HGDIOBJs except HDC that one will have it's own class. Sets destruction at resource
    // acquisition. -Samer
    using font_handle = std::unique_ptr<HFONT__, decltype(font_deleter)>;   
    font_handle hFontTitle
    {
        CreateFont(ra::FONT_SIZE_TITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH, ra::FONT_TO_USE), font_deleter
    };

    font_handle hFontDesc
    {
        CreateFont(ra::FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH, ra::FONT_TO_USE), font_deleter
    };

    auto nTitleX{ 10 };
    auto nDescX{ nTitleX + 2 };

    const auto nHeight{ rcDest.bottom - rcDest.top };

    auto fFadeInY{ GetYOffsetPct() * (ra::POPUP_DIST_Y_FROM_PCT * ra::to_floating(nHeight)) };
    fFadeInY += (ra::POPUP_DIST_Y_TO_PCT * ra::to_floating(nHeight));

    const auto nTitleY{ ra::ftol(fFadeInY) };
    const auto nDescY{ nTitleY + 32 };

    HBITMAP hBitmap = ActiveMessage().Image();
    if (hBitmap != nullptr)
    { 
        DrawImage(hDC, hBitmap, nTitleX, nTitleY, 64, 64);

        nTitleX += 64 + 4 + 2;	//	Negate the 2 from earlier!
        nDescX += 64 + 4;
    }

    const auto sTitle{ " " + ActiveMessage().Title() + " " };
    const auto sSubTitle{ " " + ActiveMessage().Subtitle() + " " };

    SelectFont(hDC, hFontTitle.get());
    TextOut(hDC, nTitleX, nTitleY, NativeStr(sTitle).c_str(), ra::strlen_as_int(sTitle));

    auto lpszTitle{ std::make_unique<SIZE>() };
    GetTextExtentPoint32(hDC, NativeStr(sTitle).c_str(), ra::strlen_as_int(sTitle), lpszTitle.get());

    auto lpszAchievement{ std::make_unique<SIZE>() };
    if (!ActiveMessage().Subtitle().empty())
    {
        SelectFont(hDC, hFontDesc.get());
        TextOut(hDC, nDescX, nDescY, NativeStr(sSubTitle).c_str(), ra::strlen_as_int(sSubTitle));
        GetTextExtentPoint32(hDC, NativeStr(sSubTitle).c_str(), ra::strlen_as_int(sSubTitle), lpszAchievement.get());
    }

    auto pen_deleter = [](HPEN hpen) noexcept
    {
        if (hpen)
        {
            DeletePen(hpen);
            hpen = nullptr;
        }
    };

    using pen_handle = std::unique_ptr<HPEN__, decltype(pen_deleter)>;
    pen_handle hPen{ CreatePen(PS_SOLID, 2, COL_POPUP_SHADOW), pen_deleter };
    SelectPen(hDC, hPen.get());

    MoveToEx(hDC, nTitleX, nTitleY + lpszTitle->cy, nullptr);
    LineTo(hDC, nTitleX + lpszTitle->cx, nTitleY + lpszTitle->cy);	//	right
    LineTo(hDC, nTitleX + lpszTitle->cx, nTitleY + 1);			//	up

    if (!ActiveMessage().Subtitle().empty())
    {
        MoveToEx(hDC, nDescX, nDescY + lpszAchievement->cy, nullptr);
        LineTo(hDC, nDescX + lpszAchievement->cx, nDescY + lpszAchievement->cy);
        LineTo(hDC, nDescX + lpszAchievement->cx, nDescY + 1);
    }
}

void AchievementPopup::Clear() noexcept
{
    while (!m_vMessages.empty())
        m_vMessages.pop();
}


