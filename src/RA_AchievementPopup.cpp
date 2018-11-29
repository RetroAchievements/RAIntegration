#include "RA_AchievementPopup.h"

#include "RA_Core.h"

#include "ui\drawing\gdi\GDISurface.hh"

namespace {
const float POPUP_DIST_Y_TO_PCT = 0.856f; // Where on screen to end up
const float POPUP_DIST_Y_FROM_PCT = 0.4f; // Amount of screens to travel
const char* FONT_TO_USE = "Tahoma";

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
    ASSERT(MessagesPresent()); // ActiveMessage() dereferences!
    const std::wstring sSoundPath = g_sHomeDir + RA_DIR_OVERLAY + MSG_SOUND[m_vMessages.front().Type()];
    PlaySoundW(sSoundPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void AchievementPopup::AddMessage(MessagePopup&& msg)
{
    // request the image now, so it's ready when the popup gets displayed
    if (msg.Image().Type() != ra::ui::ImageType::None)
        ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>().FetchImage(msg.Image().Type(), msg.Image().Name());

    m_vMessages.emplace(std::move(msg));
    PlayAudio();
}

void AchievementPopup::Update(_UNUSED ControllerInput, float fDelta, _UNUSED bool, bool bPaused)
{
    if (bPaused)
        fDelta = 0.0F;
    fDelta = std::clamp(fDelta, 0.0F, 0.3F); // Limit this!
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
        // Fading in.
        float fDelta = (APPEAR_AT - m_fTimer);
        fDelta *= fDelta; // Quadratic
        fVal = fDelta;
    }
    else if (m_fTimer < FADEOUT_AT)
    {
        // Faded in - held
        fVal = 0.0f;
    }
    else if (m_fTimer < FINISH_AT)
    {
        // Fading out
        float fDelta = (FADEOUT_AT - m_fTimer);
        fDelta *= fDelta; // Quadratic
        fVal = (fDelta);
    }
    else
    {
        // Finished!
        fVal = 1.0f;
    }

    return fVal;
}

const ra::ui::drawing::ISurface& MessagePopup::GetRendered()
{
    if (m_pSurface != nullptr)
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
        if (!pImageRepository.HasReferencedImageChanged(m_hMessageImage))
            return *m_pSurface;
    }

    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    auto nFontTitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
    const auto sTitle = ra::Widen(Title());
    auto szTitle = pSurface->MeasureText(nFontTitle, sTitle);

    auto nFontSubtitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUBTITLE, ra::ui::FontStyles::Normal);
    const auto sSubTitle = ra::Widen(Subtitle());
    auto szSubTitle = pSurface->MeasureText(nFontSubtitle, sSubTitle);

    // create the actual surface
    const int nWidth = 64 + 6 + std::max(szTitle.Width, szSubTitle.Width) + 8 + 2;
    const int nHeight = 64 + 2;

    pSurface = pSurfaceFactory.CreateTransparentSurface(nWidth, nHeight);
    m_pSurface = std::move(pSurface);

    int nX = 0;
    int nY = 0;
    const ra::ui::Color nColorBlack(0, 0, 0);
    const ra::ui::Color nColorPopup(251, 102, 0);
    const ra::ui::Color nColorBackground(0, 255, 0, 255);
    const int nShadowOffset = 2;

    // background
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), nColorBackground);

    // image
    if (Image().Type() != ra::ui::ImageType::None)
    {
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, 64, 64, nColorBlack);
        m_pSurface->DrawImage(nX, nY, 64, 64, Image());
        nX += 64 + 6;
    }

    // title
    m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szTitle.Width + 8, szTitle.Height, nColorBlack);
    m_pSurface->FillRectangle(nX, nY, szTitle.Width + 8, szTitle.Height, nColorPopup);
    m_pSurface->WriteText(nX + 4, nY - 1, nFontTitle, nColorBlack, sTitle);

    // subtitle
    if (!sSubTitle.empty())
    {
        nY += 32 + 2;
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szSubTitle.Width + 8, szSubTitle.Height, nColorBlack);
        m_pSurface->FillRectangle(nX, nY, szSubTitle.Width + 8, szSubTitle.Height, nColorPopup);
        m_pSurface->WriteText(nX + 4, nY - 1, nFontSubtitle, nColorBlack, sSubTitle);
    }

    m_pSurface->SetOpacity(0.85);

    // return final result
    return *m_pSurface;
}

void AchievementPopup::Render(HDC hDC, const RECT& rcDest)
{
    if (!MessagesPresent())
        return;

    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcDest);

    float fFadeInY = GetYOffsetPct() * (POPUP_DIST_Y_FROM_PCT * static_cast<float>(pSurface.GetHeight()));
    fFadeInY += (POPUP_DIST_Y_TO_PCT * static_cast<float>(pSurface.GetHeight()));
    pSurface.DrawSurface(10, static_cast<int>(fFadeInY), m_vMessages.front().GetRendered());
}

void AchievementPopup::Clear()
{
    while (!m_vMessages.empty())
        m_vMessages.pop();
}
