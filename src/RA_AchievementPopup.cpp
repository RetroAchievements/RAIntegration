#include "RA_AchievementPopup.h"

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

#include "ui\drawing\gdi\GDISurface.hh"

namespace {
const float POPUP_DIST_Y_TO_PCT = 0.856f;		//	Where on screen to end up
const float POPUP_DIST_Y_FROM_PCT = 0.4f;		//	Amount of screens to travel
const TCHAR* FONT_TO_USE = _T("Tahoma");

const int FONT_SIZE_TITLE = 32;
const int FONT_SIZE_SUBTITLE = 28;

const float START_AT = 0.0f;
const float APPEAR_AT = 0.8f;
const float FADEOUT_AT = 4.2f;
const float FINISH_AT = 5.0f;

const wchar_t* MSG_SOUND[] =
{
    L"login.wav",
    L"info.wav",
    L"unlock.wav",
    L"acherror.wav",
    L"lb.wav",
    L"lbcancel.wav",
    L"message.wav",
};
static_assert(SIZEOF_ARRAY(MSG_SOUND) == NumMessageTypes, "Must match!");
}

AchievementPopup::AchievementPopup() :
    m_fTimer(0.0f)
{
}

void AchievementPopup::PlayAudio()
{
    ASSERT(MessagesPresent());	//	ActiveMessage() dereferences!
    std::wstring sSoundPath = g_sHomeDir + RA_DIR_OVERLAY + MSG_SOUND[ActiveMessage().Type()];
    PlaySoundW(sSoundPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void AchievementPopup::AddMessage(const MessagePopup& msg)
{
    m_vMessages.push(msg);
    PlayAudio();
}

void AchievementPopup::Update(_UNUSED ControllerInput, float fDelta, _UNUSED bool, bool bPaused)
{
    if (bPaused)
        fDelta = 0.0F;
    fDelta = std::clamp(fDelta, 0.0F, 0.3F);	//	Limit this!
    if (m_vMessages.size() > 0)
    {
        m_fTimer += fDelta;
        if (m_fTimer >= FINISH_AT)
        {
            m_vMessages.pop();
            m_fTimer = 0.0F;
        }
    }
}

float AchievementPopup::GetYOffsetPct() const
{
    float fVal = 0.0f;

    if (m_fTimer < APPEAR_AT)
    {
        //	Fading in.
        float fDelta = (APPEAR_AT - m_fTimer);
        fDelta *= fDelta;	//	Quadratic
        fVal = fDelta;
    }
    else if (m_fTimer < FADEOUT_AT)
    {
        //	Faded in - held
        fVal = 0.0f;
    }
    else if (m_fTimer < FINISH_AT)
    {
        //	Fading out
        float fDelta = (FADEOUT_AT - m_fTimer);
        fDelta *= fDelta;	//	Quadratic
        fVal = (fDelta);
    }
    else
    {
        //	Finished!
        fVal = 1.0f;
    }

    return fVal;
}

void AchievementPopup::Render(HDC hDC, RECT& rcDest)
{
    if (!MessagesPresent())
        return;

    const auto sTitle = ra::Widen(ActiveMessage().Title());
    const auto sSubTitle = ra::Widen(ActiveMessage().Subtitle());

    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcDest, m_pResourceRepository);
    auto nFontTitle = pSurface.LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
    auto nFontSubtitle = pSurface.LoadFont(FONT_TO_USE, FONT_SIZE_SUBTITLE, ra::ui::FontStyles::Normal);

    float fFadeInY = GetYOffsetPct() * (POPUP_DIST_Y_FROM_PCT * static_cast<float>(pSurface.GetHeight()));
    fFadeInY += (POPUP_DIST_Y_TO_PCT * static_cast<float>(pSurface.GetHeight()));

    int nX = 10;
    int nY = static_cast<int>(fFadeInY);

    const ra::ui::Color nColorBlack(0, 0, 0);
    const ra::ui::Color nColorPopup(251, 102, 0);
    const int nShadowOffset = 2;

    if (ActiveMessage().Image().Type() != ra::ui::ImageType::None)
    {
        pSurface.FillRectangle(nX + nShadowOffset, nY + nShadowOffset, 64, 64, nColorBlack);
        pSurface.DrawImage(nX, nY, 64, 64, ActiveMessage().Image());
        nX += 64 + 6;
    }

    auto szTitle = pSurface.MeasureText(nFontTitle, sTitle);
    pSurface.FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szTitle.Width + 8, szTitle.Height, nColorBlack);
    pSurface.FillRectangle(nX, nY, szTitle.Width + 8, szTitle.Height, nColorPopup);
    pSurface.WriteText(nX + 4, nY - 1, nFontTitle, nColorBlack, sTitle);

    if (!sSubTitle.empty())
    {
        nY += 32 + 2;
        auto szSubTitle = pSurface.MeasureText(nFontSubtitle, sSubTitle);
        pSurface.FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szSubTitle.Width + 8, szSubTitle.Height, nColorBlack);
        pSurface.FillRectangle(nX, nY, szSubTitle.Width + 8, szSubTitle.Height, nColorPopup);
        pSurface.WriteText(nX + 4, nY - 1, nFontSubtitle, nColorBlack, sSubTitle);
    }
}

void AchievementPopup::Clear()
{
    while (!m_vMessages.empty())
        m_vMessages.pop();
}
