#include "PopupMessageViewModel.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static _CONSTANT_VAR FONT_TO_USE = "Tahoma";
static _CONSTANT_VAR FONT_SIZE_TITLE = 32;
static _CONSTANT_VAR FONT_SIZE_SUBTITLE = 28;

const StringModelProperty PopupMessageViewModel::TitleProperty("PopupMessageViewModel", "Title", L"");
const StringModelProperty PopupMessageViewModel::DescriptionProperty("PopupMessageViewModel", "Description", L"");
const StringModelProperty PopupMessageViewModel::DetailProperty("PopupMessageViewModel", "Detail", L"");

void PopupMessageViewModel::BeginAnimation()
{
    m_fAnimationProgress = 0.0;

    // left margin 10px
    SetRenderLocationX(10);

    // animate to bottom margin 10px. assume height = 64+2
    m_nInitialY = 0;
    m_nTargetY = 10 + 64 + 2;
    SetRenderLocationY(m_nInitialY);
    SetRenderLocationYRelativePosition(RelativePosition::Far);
}

bool PopupMessageViewModel::UpdateRenderImage(double fElapsed)
{
    const int nOldY = GetRenderLocationY();

    m_fAnimationProgress += fElapsed;
    const int nNewY = GetFadeOffset(m_fAnimationProgress, TOTAL_ANIMATION_TIME, INOUT_TIME, m_nInitialY, m_nTargetY);

    bool bUpdated = false;
    if (nNewY != nOldY)
    {
        SetRenderLocationY(nNewY);
        bUpdated = true;
    }

    if (m_bSurfaceStale)
    {
        m_bSurfaceStale = false;
        CreateRenderImage();
        bUpdated = true;
    }
    else if (m_pSurface == nullptr)
    {
        CreateRenderImage();
        bUpdated = true;
    }
    else
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
        if (pImageRepository.HasReferencedImageChanged(m_hImage))
        {
            CreateRenderImage();
            bUpdated = true;
        }
    }

    return bUpdated;
}

void PopupMessageViewModel::CreateRenderImage()
{
    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    auto nFontTitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
    const auto sTitle = GetTitle();
    const auto szTitle = pSurface->MeasureText(nFontTitle, sTitle);

    auto nFontSubtitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUBTITLE, ra::ui::FontStyles::Normal);
    const auto sSubTitle = GetDescription();
    const auto szSubTitle = pSurface->MeasureText(nFontSubtitle, sSubTitle);

    // create the actual surface
    const int nWidth = 64 + 6 + std::max(szTitle.Width, szSubTitle.Width) + 8 + 2;
    constexpr int nHeight = 64 + 2;

    pSurface = pSurfaceFactory.CreateTransparentSurface(nWidth, nHeight);
    m_pSurface = std::move(pSurface);

    int nX = 0;
    int nY = 0;
    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nShadowOffset = pTheme.ShadowOffset();

    // background
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), Color::Transparent);

    // image
    if (m_hImage.Type() != ra::ui::ImageType::None)
    {
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, 64, 64, pTheme.ColorShadow());
        m_pSurface->DrawImage(nX, nY, 64, 64, m_hImage);
        nX += 64 + 6;
    }

    // title
    m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szTitle.Width + 8, szTitle.Height, pTheme.ColorShadow());
    m_pSurface->FillRectangle(nX, nY, szTitle.Width + 8, szTitle.Height, pTheme.ColorBackground());
    m_pSurface->WriteText(nX + 4, nY - 1, nFontTitle, pTheme.ColorTitle(), sTitle);

    // subtitle
    if (!sSubTitle.empty())
    {
        nY += 32 + 2;
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szSubTitle.Width + 8, szSubTitle.Height, pTheme.ColorShadow());
        m_pSurface->FillRectangle(nX, nY, szSubTitle.Width + 8, szSubTitle.Height, pTheme.ColorBackground());
        m_pSurface->WriteText(nX + 4, nY - 1, nFontSubtitle, pTheme.ColorDescription(), sSubTitle);
    }

    m_pSurface->SetOpacity(0.85);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
