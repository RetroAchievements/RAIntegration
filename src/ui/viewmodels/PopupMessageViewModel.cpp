#include "PopupMessageViewModel.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static _CONSTANT_VAR FONT_TO_USE = "Tahoma";
static _CONSTANT_VAR FONT_SIZE_TITLE = 24;
static _CONSTANT_VAR FONT_SIZE_SUBTITLE = 20;
static _CONSTANT_VAR FONT_SIZE_DETAIL = 18;

const StringModelProperty PopupMessageViewModel::TitleProperty("PopupMessageViewModel", "Title", L"");
const StringModelProperty PopupMessageViewModel::DescriptionProperty("PopupMessageViewModel", "Description", L"");
const StringModelProperty PopupMessageViewModel::DetailProperty("PopupMessageViewModel", "Detail", L"");
const BoolModelProperty PopupMessageViewModel::IsDetailErrorProperty("PopupMessageViewModel", "IsDetailError", false);

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

    auto nFontDetail = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_DETAIL, ra::ui::FontStyles::Normal);
    const auto sDetail = GetDetail();
    const auto szDetail = pSurface->MeasureText(nFontDetail, sDetail);

    const auto& pOverlayTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nShadowOffset = pOverlayTheme.ShadowOffset();

    // create the actual surface
    constexpr int nImageSize = 64;
    constexpr int nSubTitleIndent = 4;
    const int nWidth = 4 + ((m_hImage.Type() != ra::ui::ImageType::None) ? nImageSize : 0) + 6 +
        std::max(szTitle.Width, std::max(szSubTitle.Width, szDetail.Width) + nSubTitleIndent) + 6 + 4 + nShadowOffset;
    const int nHeight = 4 + nImageSize + 4 + nShadowOffset;

    pSurface = pSurfaceFactory.CreateTransparentSurface(nWidth, nHeight);
    m_pSurface = std::move(pSurface);

    int nX = 4;
    int nY = 4;

    // background
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), ra::ui::Color::Transparent);
    m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, m_pSurface->GetWidth() - nShadowOffset, m_pSurface->GetHeight() - nShadowOffset, pOverlayTheme.ColorShadow());

    // frame
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth() - nShadowOffset, m_pSurface->GetHeight() - nShadowOffset, pOverlayTheme.ColorBackground());
    m_pSurface->FillRectangle(1, 1, m_pSurface->GetWidth() - nShadowOffset - 2, m_pSurface->GetHeight() - nShadowOffset - 2, pOverlayTheme.ColorBorder());
    m_pSurface->FillRectangle(2, 2, m_pSurface->GetWidth() - nShadowOffset - 4, m_pSurface->GetHeight() - nShadowOffset - 4, pOverlayTheme.ColorBackground());

    // image
    if (m_hImage.Type() != ra::ui::ImageType::None)
    {
        m_pSurface->DrawImage(nX, nY, nImageSize, nImageSize, m_hImage);
        nX += nImageSize;
    }
    nX += 6;

    // title
    nY -= 1;
    m_pSurface->WriteText(nX + 1, nY + 1, nFontTitle, pOverlayTheme.ColorTextShadow(), sTitle);
    m_pSurface->WriteText(nX, nY, nFontTitle, pOverlayTheme.ColorTitle(), sTitle);
    nX += nSubTitleIndent;

    // subtitle
    nY += FONT_SIZE_TITLE - 1;
    if (!sSubTitle.empty())
    {
        m_pSurface->WriteText(nX + 1, nY + 1, nFontSubtitle, pOverlayTheme.ColorTextShadow(), sSubTitle);
        m_pSurface->WriteText(nX, nY, nFontSubtitle, pOverlayTheme.ColorDescription(), sSubTitle);
    }

    // detail
    nY += FONT_SIZE_SUBTITLE;
    if (!sDetail.empty())
    {
        m_pSurface->WriteText(nX + 1, nY + 1, nFontDetail, pOverlayTheme.ColorTextShadow(), sDetail);
        m_pSurface->WriteText(nX, nY, nFontDetail, IsDetailError() ? pOverlayTheme.ColorError() : pOverlayTheme.ColorDetail(), sDetail);
    }

    m_pSurface->SetOpacity(0.85);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
