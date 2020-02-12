#include "MultiLineGridBinding.hh"

#include "GridTextColumnBinding.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

gsl::index MultiLineGridBinding::GetIndexForLine(gsl::index nLine) const
{
    // TODO: update m_nFirstVisibleItem in UpdateScroll
    gsl::index nIndex = m_nFirstVisibleItem;
    while (nIndex < ra::to_signed(m_vItemMetrics.size()))
    {
        const auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        if (pItemMetrics.nFirstLine > nLine)
        {
            if (nIndex == 0)
                break;
            if (--nIndex == 0)
                break;
        }
        else if (pItemMetrics.nFirstLine + pItemMetrics.nNumLines <= nLine)
        {
            ++nIndex;
        }
        else
        {
            break;
        }
    }
    return nIndex;
}

void MultiLineGridBinding::UpdateAllItems()
{
    m_vItemMetrics.clear();
    m_vItemMetrics.resize(m_vmItems->Count());

    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (reinterpret_cast<const GridTextColumnBinding*>(pColumn) != nullptr)
        {
            LV_COLUMN col{};
            col.mask = LVCF_WIDTH | LVCF_SUBITEM;
            col.iSubItem = gsl::narrow_cast<int>(nColumn);
            ListView_GetColumn(m_hWnd, nColumn, &col);
            const auto nChars = std::max(col.cx / 6, 32);

            for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
            {
                const auto sText = pColumn->GetText(*m_vmItems, nIndex);
                const auto nNewLine = sText.find('\n');
                if (nNewLine == std::string::npos && sText.length() < nChars)
                    continue;

                auto& pItemMetrics = m_vItemMetrics.at(nIndex);
                auto& pColumnLineOffset = pItemMetrics.mColumnLineOffsets[gsl::narrow_cast<int>(nColumn)];
                size_t nStart = 0;
                size_t nLastWhitespace = 0;
                for (size_t nOffset = 0; nOffset < sText.length(); ++nOffset)
                {
                    const auto c = sText.at(nOffset);
                    if (c == '\n')
                    {
                        nStart = nOffset + 1;
                        pColumnLineOffset.push_back(gsl::narrow_cast<unsigned int>(nStart));
                    }
                    else if (nOffset - nStart > nChars)
                    {
                        if (nLastWhitespace > nStart)
                        {
                            nStart = nLastWhitespace + 1;
                            pColumnLineOffset.push_back(gsl::narrow_cast<unsigned int>(nStart));
                        }
                        else if (c == ' ' || c == '\t')
                        {
                            nStart = nOffset + 1;
                            pColumnLineOffset.push_back(gsl::narrow_cast<unsigned int>(nStart));
                        }
                    }
                    else if (c == ' ' || c == '\t')
                    {
                        nLastWhitespace = nOffset;
                    }
                }

                if (pColumnLineOffset.size() + 1 > pItemMetrics.nNumLines)
                    pItemMetrics.nNumLines = gsl::narrow_cast<unsigned int>(pColumnLineOffset.size()) + 1;
            }
        }
    }

    gsl::index nLine = 0;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
    {
        auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        pItemMetrics.nFirstLine = gsl::narrow_cast<unsigned int>(nLine);
        nLine += pItemMetrics.nNumLines;
    }

    GridBinding::UpdateAllItems();
}

void MultiLineGridBinding::UpdateItems(gsl::index nColumn)
{
    const auto& pColumn = *m_vColumns.at(nColumn);
    const auto nItems = ListView_GetItemCount(m_hWnd);
    std::string sText;

    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iSubItem = gsl::narrow_cast<int>(nColumn);

    gsl::index nLine = 0;
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
    {
        sText = NativeStr(pColumn.GetText(*m_vmItems, i));
        item.pszText = sText.data();
        item.iItem = gsl::narrow_cast<int>(nLine);

        const auto& pItemMetrics = m_vItemMetrics.at(i);
        if (pItemMetrics.nNumLines > 1)
        {
            const auto pIter = pItemMetrics.mColumnLineOffsets.find(gsl::narrow_cast<int>(nColumn));
            if (pIter != pItemMetrics.mColumnLineOffsets.end())
            {
                for (auto nIndex : pIter->second)
                {
                    auto nIndex2 = nIndex - 1;
                    while (isspace(sText.at(nIndex2 - 1)))
                        --nIndex2;
                    sText.at(nIndex2) = '\0';

                    if (nLine < nItems)
                        ListView_SetItem(m_hWnd, &item);
                    else
                        ListView_InsertItem(m_hWnd, &item);

                    item.pszText = &sText.at(nIndex);
                    item.iItem = gsl::narrow_cast<int>(++nLine);
                }
            }
            else
            {
                for (auto nIndex = 1; nIndex < pItemMetrics.nNumLines; ++nIndex)
                {
                    if (nLine < nItems)
                        ListView_SetItem(m_hWnd, &item);
                    else
                        ListView_InsertItem(m_hWnd, &item);

                    sText.at(0) = '\0';
                    item.iItem = gsl::narrow_cast<int>(++nLine);
                }
            }
        }

        if (nLine < nItems)
            ListView_SetItem(m_hWnd, &item);
        else
            ListView_InsertItem(m_hWnd, &item);

        ++nLine;
    }

    if (ra::to_unsigned(nItems) > nLine)
    {
        for (gsl::index i = gsl::narrow_cast<gsl::index>(nItems) - 1; ra::to_unsigned(i) >= nLine; --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

LRESULT MultiLineGridBinding::OnLvnItemChanging(const LPNMLISTVIEW pnmListView)
{
    // prevent built-in selection logic - we'll do manual selection in OnNmClick
    if (pnmListView->uChanged & LVIF_STATE && pnmListView->uNewState & LVIS_SELECTED && !(pnmListView->uOldState & LVIS_SELECTED))
        return TRUE;

    return GridBinding::OnLvnItemChanging(pnmListView);
}

void MultiLineGridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    NMLISTVIEW nmListView;
    memcpy(&nmListView, pnmListView, sizeof(nmListView));
    nmListView.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmListView->iItem));
    GridBinding::OnLvnItemChanged(&nmListView);
}

LRESULT MultiLineGridBinding::OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw)
{
    NMLVCUSTOMDRAW lvCustomDraw;
    memcpy(&lvCustomDraw, pCustomDraw, sizeof(NMLVCUSTOMDRAW));
    lvCustomDraw.nmcd.dwItemSpec = GetIndexForLine(pCustomDraw->nmcd.dwItemSpec);

    switch (pCustomDraw->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            if (m_pIsSelectedProperty && m_vmItems->GetItemValue(lvCustomDraw.nmcd.dwItemSpec, *m_pIsSelectedProperty))
            {
                pCustomDraw->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
                return CDRF_NOTIFYSUBITEMDRAW;
            }
            break;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (m_pIsSelectedProperty && m_vmItems->GetItemValue(lvCustomDraw.nmcd.dwItemSpec, *m_pIsSelectedProperty))
            {
                pCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                return CDRF_DODEFAULT;
            }
            break;
    }

    const auto nResult = GridBinding::OnCustomDraw(&lvCustomDraw);
    pCustomDraw->clrText = lvCustomDraw.clrText;
    pCustomDraw->clrTextBk = lvCustomDraw.clrTextBk;
    return nResult;
}

void MultiLineGridBinding::OnNmClick(const NMITEMACTIVATE* pnmItemActivate)
{
    NMITEMACTIVATE nmItemActivate;
    memcpy(&nmItemActivate, pnmItemActivate, sizeof(nmItemActivate));
    nmItemActivate.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmItemActivate->iItem));

    if (m_pIsSelectedProperty)
    {
        m_vmItems->BeginUpdate();

        if (GetAsyncKeyState(VK_LCONTROL) < 0 || GetAsyncKeyState(VK_RCONTROL) < 0)
        {
            // when CTRL+clicking, toggle the clicked item
            m_vmItems->SetItemValue(nmItemActivate.iItem, *m_pIsSelectedProperty,
                !m_vmItems->GetItemValue(nmItemActivate.iItem, *m_pIsSelectedProperty));
        }
        else if (GetAsyncKeyState(VK_LSHIFT) < 0 || GetAsyncKeyState(VK_RSHIFT) < 0)
        {
            // when SHIFT+clicking, select a range
            gsl::index nFirst = nmItemActivate.iItem;
            gsl::index nLast = m_nLastClickedItem;

            if (nLast < nFirst)
            {
                nLast = nmItemActivate.iItem;
                nFirst = m_nLastClickedItem;
            }

            for (auto nIndex = nFirst; nIndex <= nLast; ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, true);
        }
        else
        {
            // when normal clicking, unselect everything but the clicked item
            for (gsl::index nIndex = 0; nIndex < m_vmItems->Count(); ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, nIndex == nmItemActivate.iItem);
        }

        m_vmItems->EndUpdate();

        m_nLastClickedItem = nmItemActivate.iItem;
    }

    GridBinding::OnNmClick(&nmItemActivate);
}

void MultiLineGridBinding::OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate)
{
    NMITEMACTIVATE nmItemActivate;
    memcpy(&nmItemActivate, pnmItemActivate, sizeof(nmItemActivate));
    nmItemActivate.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmItemActivate->iItem));
    GridBinding::OnNmDblClick(&nmItemActivate);
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
