#include "RA_AchievementPopup.h"

#include "RA_Core.h"

#include "ui\drawing\gdi\GDISurface.hh"

#include "ra_math.h"

_CONSTANT_VAR POPUP_DIST_Y_TO_PCT   = 0.856F; // Where on screen to end up
_CONSTANT_VAR POPUP_DIST_Y_FROM_PCT = 0.4F;   // Amount of screens to travel
_CONSTANT_VAR FONT_TO_USE           = "Tahoma";

_CONSTANT_VAR FONT_SIZE_TITLE    = 32;
_CONSTANT_VAR FONT_SIZE_SUBTITLE = 28;

_CONSTANT_VAR START_AT   = 0.0F;
_CONSTANT_VAR APPEAR_AT  = 0.8F;
_CONSTANT_VAR FADEOUT_AT = 4.2F;
_CONSTANT_VAR FINISH_AT  = 5.0F;

inline constexpr std::array<const wchar_t*, 7> MSG_SOUND{
    L"login.wav",
    L"info.wav",
    L"unlock.wav",
    L"acherror.wav",
    L"lb.wav",
    L"lbcancel.wav",
    L"message.wav",
};

void AchievementPopup::PlayAudio()
{
    ASSERT(MessagesPresent()); // ActiveMessage() dereferences!
    const auto sSoundPath =
        ra::StringPrintf(L"%s%s%s", g_sHomeDir, RA_DIR_OVERLAY, MSG_SOUND.at(ra::etoi(m_vMessages.front().Type())));
    PlaySoundW(sSoundPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void AchievementPopup::AddMessage(MessagePopup&& msg)
{
    // request the image now, so it's ready when the popup gets displayed
    if (msg.Image().Type() != ra::ui::ImageType::None)
        ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>().FetchImage(msg.Image().Type(),
                                                                                        msg.Image().Name());

    m_vMessages.emplace(std::move(msg));
    PlayAudio();
}

void AchievementPopup::Update(float fDelta)
{
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

float AchievementPopup::GetYOffsetPct() const noexcept
{
    float fVal = 0.0F;

    if (m_fTimer < APPEAR_AT)
    {
        // Fading in.
        fVal = ra::sqr(APPEAR_AT - m_fTimer); // Quadratic
    }
    else if (m_fTimer < FADEOUT_AT)
    {
        // Faded in - held
        fVal = 0.0F;
    }
    else if (m_fTimer < FINISH_AT)
    {
        // Fading out
        fVal = ra::sqr(FADEOUT_AT - m_fTimer); // Quadratic
    }
    else
    {
        // Finished!
        fVal = 1.0F;
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
    const auto szTitle = pSurface->MeasureText(nFontTitle, sTitle);

    auto nFontSubtitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUBTITLE, ra::ui::FontStyles::Normal);
    const auto sSubTitle = ra::Widen(Subtitle());
    const auto szSubTitle = pSurface->MeasureText(nFontSubtitle, sSubTitle);

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
    constexpr int nShadowOffset = 2;

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

void AchievementPopup::Render(ra::ui::drawing::ISurface& pSurface)
{
    if (!MessagesPresent())
        return;

    float fFadeInY = GetYOffsetPct() * (POPUP_DIST_Y_FROM_PCT * static_cast<float>(pSurface.GetHeight()));
    fFadeInY += (POPUP_DIST_Y_TO_PCT * static_cast<float>(pSurface.GetHeight()));
    pSurface.DrawSurface(10, static_cast<int>(fFadeInY), m_vMessages.front().GetRendered());
}

void AchievementPopup::Clear() noexcept
{
    while (MessagesPresent())
        GSL_SUPPRESS_F6 m_vMessages.pop(); // shouldn't throw if not empty
}
