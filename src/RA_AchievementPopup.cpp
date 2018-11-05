#include "RA_AchievementPopup.h"

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"
#include "ra_math.h"

void AchievementPopup::PlayAudio()
{
    constexpr std::array<const wchar_t*, 7> MSG_SOUND
    {
        L"login.wav",
        L"info.wav",
        L"unlock.wav",
        L"acherror.wav",
        L"lb.wav",
        L"lbcancel.wav",
        L"message.wav",
    };
    ASSERT(MessagesPresent());	//	ActiveMessage() dereferences!
    auto niggers = ra::StringPrintf(L"%s%s%s", g_sHomeDir.c_str(), RA_DIR_OVERLAY, MSG_SOUND.at(ra::etoi(ActiveMessage().GetType())));
    PlaySoundW(niggers.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void AchievementPopup::AddMessage(const MessagePopup& msg)
{
    m_vMessages.push(msg);
    PlayAudio();
}

void AchievementPopup::Update(_UNUSED ControllerInput& __restrict, float fDelta, _UNUSED bool, bool bPaused)
{
    if (bPaused)
        fDelta = 0.0F;
    fDelta = std::clamp(fDelta, 0.0F, 0.3F); // Limit this!
    if (MessagesPresent())
    {
        m_fTimer += fDelta;
        if (m_fTimer >= FINISH_AT)
        {
            m_vMessages.pop();
            m_fTimer = 0.0F;
        }
    }
}

float AchievementPopup::GetYOffsetPct() const noexcept
{
    float fVal{};
    _CONSTANT_LOC START_AT   = 0.0F;
    _CONSTANT_LOC APPEAR_AT  = 0.8F;
    _CONSTANT_LOC FADEOUT_AT = 4.2F;
    if (m_fTimer < APPEAR_AT)
    {
        // Fading in.
        const auto fDelta{ APPEAR_AT - m_fTimer };
        fVal = ra::sqr(fDelta); // Quadratic
    }
    else if (m_fTimer < FADEOUT_AT)
    {
        // Faded in - held
        fVal = 0.0F;
    }
    else if (m_fTimer < FINISH_AT)
    {
        //	Fading out
        const auto fDelta{ FADEOUT_AT - m_fTimer };
        fVal = ra::sqr(fDelta); // Quadratic
    }
    else
    {
        // Finished!
        fVal = 1.0F;
    }

    return fVal;
}

void AchievementPopup::Render(HDC__* __restrict hDC, RECT& __restrict rcDest)
{
    if (!MessagesPresent())
        return;

    const auto nPixelWidth = rcDest.right - rcDest.left;

    //SetBkColor( hDC, RGB( 0, 212, 0 ) );
    SetBkColor(hDC, COL_TEXT_HIGHLIGHT);
    SetTextColor(hDC, COL_POPUP);

    _CONSTANT_LOC FONT_SIZE_TITLE = 32;
    _CONSTANT_LOC FONT_TO_USE = _T("Tahoma");
    const auto hFontTitle = CreateFont(FONT_SIZE_TITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
        DEFAULT_PITCH, FONT_TO_USE);

    _CONSTANT_LOC FONT_SIZE_SUBTITLE = 28;
    const auto hFontDesc = CreateFont(FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
        DEFAULT_PITCH, FONT_TO_USE);

    auto nTitleX = 10;
    auto nDescX  = nTitleX + 2;

    const auto nHeight = rcDest.bottom - rcDest.top;
   
    _CONSTANT_LOC POPUP_DIST_Y_FROM_PCT = 0.4F; // Amount of screens to travel
    auto fFadeInY = GetYOffsetPct() * (POPUP_DIST_Y_FROM_PCT * ra::to_floating(nHeight));
    _CONSTANT_LOC POPUP_DIST_Y_TO_PCT = 0.856F; // Where on screen to end up
    fFadeInY += (POPUP_DIST_Y_TO_PCT * ra::to_floating(nHeight));

    const auto nTitleY = ra::ftol(fFadeInY);
    const auto nDescY  = nTitleY + 32;

    const auto hBitmap = ActiveMessage().Image();
    if (hBitmap != nullptr)
    { 
        DrawImage(hDC, hBitmap, nTitleX, nTitleY, 64, 64);

        nTitleX += 64 + 4 + 2;	//	Negate the 2 from earlier!
        nDescX += 64 + 4;
    }

    const auto sTitle{ ra::StringPrintf(_T(" %s "), NativeStr(ActiveMessage().Title()).c_str()) };

    SelectFont(hDC, hFontTitle);
    TextOut(hDC, nTitleX, nTitleY, sTitle.c_str(), sTitle.length());
    SIZE szTitle{};
    GetTextExtentPoint32(hDC, sTitle.c_str(), sTitle.length(), &szTitle);

    SIZE szAchievement{};
    if (!ActiveMessage().Subtitle().empty())
    {
        SelectFont(hDC, hFontDesc);
        const auto sSubTitle{ ra::StringPrintf(_T(" %s "), NativeStr(ActiveMessage().Subtitle()).c_str()) };
        TextOut(hDC, nDescX, nDescY, sSubTitle.c_str(), sSubTitle.length());
        GetTextExtentPoint32(hDC, sSubTitle.c_str(), sSubTitle.length(), &szAchievement);
    }

    const auto hPen = ::CreatePen(PS_SOLID, 2, COL_POPUP_SHADOW);
    SelectPen(hDC, hPen);

    ::MoveToEx(hDC, nTitleX, nTitleY + szTitle.cy, nullptr);
    ::LineTo(hDC, nTitleX + szTitle.cx, nTitleY + szTitle.cy);	//	right
    ::LineTo(hDC, nTitleX + szTitle.cx, nTitleY + 1);			//	up

    if (ActiveMessage().Subtitle().length() > 0)
    {
        ::MoveToEx(hDC, nDescX, nDescY + szAchievement.cy, nullptr);
        ::LineTo(hDC, nDescX + szAchievement.cx, nDescY + szAchievement.cy);
        ::LineTo(hDC, nDescX + szAchievement.cx, nDescY + 1);
    }

    DeletePen(hPen);
    DeleteFont(hFontTitle);
    DeleteFont(hFontDesc);
}

void AchievementPopup::Clear()
{
    while (!m_vMessages.empty())
        m_vMessages.pop();
}
