#include "OverlayListPageViewModel.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayListPageViewModel::ListTitleProperty("OverlayListPageViewModel", "ListTitle", L"");
const StringModelProperty OverlayListPageViewModel::SummaryProperty("OverlayListPageViewModel", "Summary", L"");
const IntModelProperty OverlayListPageViewModel::SelectedItemIndexProperty("OverlayListPageViewModel", "SelectedItemIndex", 0);

const StringModelProperty OverlayListPageViewModel::ItemViewModel::DetailProperty("ItemViewModel", "Detail", L"");
const BoolModelProperty OverlayListPageViewModel::ItemViewModel::IsDisabledProperty("ItemViewModel", "IsDisabled", false);

void OverlayListPageViewModel::Refresh()
{
    m_bDetail = false;
    SetTitle(m_sTitle);
    m_nImagesPending = 99;
}

bool OverlayListPageViewModel::Update(double fElapsed)
{
    m_fElapsed += fElapsed;

    bool bUpdated = m_bRedraw;
    m_bRedraw = false;

    if (m_nImagesPending)
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();

        unsigned int nImagesPending = 0;
        for (gsl::index nIndex = m_vItems.Count() - 1; nIndex >= 0; --nIndex)
        {
            const auto* pItem = m_vItems.GetItemAt(nIndex);
            if (pItem && !pImageRepository.IsImageAvailable(pItem->Image.Type(), pItem->Image.Name()))
                ++nImagesPending;
        }

        if (m_nImagesPending != nImagesPending)
        {
            m_nImagesPending = nImagesPending;
            bUpdated = true;
        }
    }

    return bUpdated;
}

void OverlayListPageViewModel::Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    if (m_bDetail)
        RenderDetail(pSurface, nX, nY, nWidth, nHeight);
    else
        RenderList(pSurface, nX, nY, nWidth, nHeight);
}

void OverlayListPageViewModel::RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();

    // title
    const auto& sGameTitle = GetListTitle();
    if (!sGameTitle.empty())
    {
        const auto nTitleFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
        pSurface.WriteText(nX, nY, nTitleFont, pTheme.ColorOverlayText(), sGameTitle);
        nY += 34;
        nHeight -= 34;
    }

    // subtitle
    const auto nFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
    const auto& sSummary = GetSummary();
    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sSummary);
    nY += 30;
    nHeight -= 30;

    // scrollbar
    const auto nSelectedIndex = ra::to_unsigned(GetSelectedItemIndex());
    gsl::index nIndex = m_nScrollOffset;
    const auto nItemSize = 64;
    const auto nItemSpacing = 8;
    m_nVisibleItems = (nHeight + nItemSpacing) / (nItemSize + nItemSpacing);

    if (m_nVisibleItems < m_vItems.Count())
    {
        RenderScrollBar(pSurface, nX, nY, nHeight, m_vItems.Count(), m_nVisibleItems, nIndex);
        nX += 20;
    }

    // achievements list
    while (nHeight > nItemSize + nItemSpacing)
    {
        const auto* pItem = m_vItems.GetItemAt(nIndex);
        if (!pItem)
            break;

        auto nTextX = nX;
        if (pItem->Image.Type() != ra::ui::ImageType::None)
        {
            pSurface.DrawImage(nX, nY, nItemSize, nItemSize, pItem->Image);
            nTextX += nItemSize;
        }

        const ra::ui::Color nSubTextColor = pItem->IsDisabled() ? pTheme.ColorOverlayDisabledSubText() : pTheme.ColorOverlaySubText();
        ra::ui::Color nTextColor = pTheme.ColorOverlayText();

        const bool bSelected = (nIndex == ra::to_signed(nSelectedIndex));
        if (bSelected)
        {
            pSurface.FillRectangle(nTextX, nY, nWidth - nTextX, nItemSize, pTheme.ColorOverlaySelectionBackground());
            nTextColor = pItem->IsDisabled() ?
                pTheme.ColorOverlaySelectionDisabledText() : pTheme.ColorOverlaySelectionText();
        }
        else if (pItem->IsDisabled())
        {
            nTextColor = pTheme.ColorOverlayDisabledText();
        }

        pSurface.WriteText(nTextX + 12, nY + 4, nFont, nTextColor, pItem->GetLabel());
        pSurface.WriteText(nTextX + 12, nY + 4 + 26, nSubFont, nSubTextColor, pItem->GetDetail());

        nIndex++;
        nY += nItemSize + nItemSpacing;
        nHeight -= nItemSize + nItemSpacing;
    }
}

bool OverlayListPageViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (pInput.m_bDownPressed)
    {
        const auto nSelectedItemIndex = GetSelectedItemIndex();
        if (nSelectedItemIndex < ra::to_signed(m_vItems.Count()) - 1)
        {
            SetSelectedItemIndex(nSelectedItemIndex + 1);

            if (nSelectedItemIndex + 1 - m_nScrollOffset == ra::to_signed(m_nVisibleItems))
                ++m_nScrollOffset;

            if (m_bDetail)
            {
                auto* vmItem = m_vItems.GetItemAt(nSelectedItemIndex + 1);
                if (vmItem != nullptr)
                    FetchItemDetail(*vmItem);
            }

            return true;
        }
    }
    else if (pInput.m_bUpPressed)
    {
        const auto nSelectedItemIndex = GetSelectedItemIndex();
        if (nSelectedItemIndex > 0)
        {
            SetSelectedItemIndex(nSelectedItemIndex - 1);

            if (nSelectedItemIndex == m_nScrollOffset)
                --m_nScrollOffset;

            if (m_bDetail)
            {
                auto* vmItem = m_vItems.GetItemAt(nSelectedItemIndex - 1);
                if (vmItem != nullptr)
                    FetchItemDetail(*vmItem);
            }

            return true;
        }
    }
    else if (pInput.m_bConfirmPressed)
    {
        return SetDetail(true);
    }
    else if (pInput.m_bCancelPressed)
    {
        return SetDetail(false);
    }

    return false;
}

bool OverlayListPageViewModel::SetDetail(bool bDetail)
{
    if (bDetail)
    {
        if (!m_bDetail && !m_sDetailTitle.empty())
        {
            m_bDetail = true;
            SetTitle(m_sDetailTitle);

            auto* vmItem = m_vItems.GetItemAt(GetSelectedItemIndex());
            if (vmItem != nullptr)
                FetchItemDetail(*vmItem);
            return true;
        }
    }
    else
    {
        if (m_bDetail)
        {
            m_bDetail = false;
            SetTitle(m_sTitle);
            return true;
        }
    }

    return false;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
