#include "MultiLineGridBinding.hh"

#include "GridTextColumnBinding.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

static bool SupportsMultipleLines(const GridColumnBinding* pColumn) noexcept
{
    return (dynamic_cast<const GridTextColumnBinding*>(pColumn) != nullptr);
}

gsl::index MultiLineGridBinding::GetIndexForLine(gsl::index nLine) const
{
    // minor optimization: m_nScrollOffset is the number of lines that are not-visible
    // if they're all single-line, it'll also be the first visible item. if some are multi-line,
    // it allows us to start the scan closer to the data we actually care about.
    gsl::index nIndex = m_nScrollOffset;
    while (nIndex < ra::to_signed(m_vItemMetrics.size()))
    {
        const auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        if (pItemMetrics.nFirstLine > ra::to_unsigned(nLine))
        {
            if (nIndex == 0)
                break;
            if (--nIndex == 0)
                break;
        }
        else if (gsl::narrow_cast<gsl::index>(pItemMetrics.nFirstLine) + ra::to_signed(pItemMetrics.nNumLines) <= nLine)
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

void MultiLineGridBinding::OnViewModelAdded(gsl::index nIndex)
{
    m_vItemMetrics.emplace(m_vItemMetrics.begin() + nIndex);

    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (SupportsMultipleLines(pColumn))
        {
            const auto nChars = GetMaxCharsForColumn(nColumn);
            UpdateLineBreaks(nIndex, nColumn, pColumn, nChars);
        }
    }

    if (!m_vmItems->IsUpdating())
        OnEndViewModelCollectionUpdate();
}

void MultiLineGridBinding::OnViewModelRemoved(gsl::index nIndex)
{
    m_vItemMetrics.erase(m_vItemMetrics.begin() + nIndex);

    if (!m_vmItems->IsUpdating())
        OnEndViewModelCollectionUpdate();
}

void MultiLineGridBinding::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (pColumn && SupportsMultipleLines(pColumn) && pColumn->DependsOn(args.Property))
        {
            auto& pItemMetrics = m_vItemMetrics.at(nIndex);
            const auto nChars = GetMaxCharsForColumn(nColumn);
            std::vector<unsigned int> vLineBreaks;
            const auto sText = pColumn->GetText(*m_vmItems, nIndex);

            GetLineBreaks(sText, nChars, vLineBreaks);

            if (vLineBreaks.empty())
            {
                pItemMetrics.mColumnLineOffsets.erase(gsl::narrow_cast<int>(nColumn));
            }
            else
            {
                auto& pColumnLineBreaks = pItemMetrics.mColumnLineOffsets[gsl::narrow_cast<int>(nColumn)];
                pColumnLineBreaks.swap(vLineBreaks);
            }

            pItemMetrics.nNumLines = 0;
            for (const auto& pPair : pItemMetrics.mColumnLineOffsets)
            {
                if (pPair.second.size() + 1 > pItemMetrics.nNumLines)
                    pItemMetrics.nNumLines = gsl::narrow_cast<unsigned int>(pPair.second.size()) + 1;
            }

            if (!m_vmItems->IsUpdating())
                OnEndViewModelCollectionUpdate();
        }
    }
}

void MultiLineGridBinding::OnEndViewModelCollectionUpdate()
{
    UpdateLineOffsets();

    InvokeOnUIThread([this]() {
        GridBinding::UpdateAllItems();
        GridBinding::OnEndViewModelCollectionUpdate();
    });
}

int MultiLineGridBinding::GetMaxCharsForColumn(gsl::index nColumn) const
{
    return std::max(m_vColumnWidths.at(nColumn) / 6, 32);
}

void MultiLineGridBinding::UpdateLineBreaks(gsl::index nIndex, gsl::index nColumn, const ra::ui::win32::bindings::GridColumnBinding* pColumn, size_t nChars)
{
    std::vector<unsigned int> vLineBreaks;
    Expects(pColumn != nullptr);

    const auto sText = pColumn->GetText(*m_vmItems, nIndex);
    GetLineBreaks(sText, nChars, vLineBreaks);

    if (!vLineBreaks.empty())
    {
        auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        auto& pColumnLineBreaks = pItemMetrics.mColumnLineOffsets[gsl::narrow_cast<int>(nColumn)];
        pColumnLineBreaks.swap(vLineBreaks);

        if (pColumnLineBreaks.size() + 1 > pItemMetrics.nNumLines)
            pItemMetrics.nNumLines = gsl::narrow_cast<unsigned int>(pColumnLineBreaks.size()) + 1;
    }
}

void MultiLineGridBinding::GetLineBreaks(const std::wstring& sText, size_t nChars, std::vector<unsigned int>& vLineBreaks)
{
    const auto nNewLine = sText.find('\n');
    if (nNewLine == std::string::npos && sText.length() < nChars)
        return;

    size_t nStart = 0;
    size_t nLastWhitespace = 0;
    for (size_t nOffset = 0; nOffset < sText.length(); ++nOffset)
    {
        const auto c = sText.at(nOffset);
        if (c == '\n')
        {
            nStart = nOffset + 1;
            vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
        }
        else if (nOffset - nStart > nChars)
        {
            if (nLastWhitespace > nStart)
            {
                nStart = nLastWhitespace + 1;
                vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
            }
            else if (c == ' ' || c == '\t')
            {
                nStart = nOffset + 1;
                vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
            }
        }
        else if (c == ' ' || c == '\t')
        {
            nLastWhitespace = nOffset;
        }
    }
}

void MultiLineGridBinding::UpdateLineOffsets()
{
    gsl::index nLine = 0;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
    {
        auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        pItemMetrics.nFirstLine = gsl::narrow_cast<unsigned int>(nLine);
        nLine += pItemMetrics.nNumLines;
    }
}

void MultiLineGridBinding::UpdateAllItems()
{
    m_vItemMetrics.clear();
    m_vItemMetrics.resize(m_vmItems->Count());

    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (SupportsMultipleLines(pColumn))
        {
            const auto nChars = GetMaxCharsForColumn(nColumn);
            for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
                UpdateLineBreaks(nIndex, nColumn, pColumn, nChars);
        }
    }

    UpdateLineOffsets();

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
                size_t nStop = 0;
                for (auto nIndex : pIter->second)
                {
                    auto nIndex2 = gsl::narrow_cast<size_t>(nIndex) - 1;
                    while (nIndex2 > nStop && isspace(sText.at(nIndex2 - 1)))
                        --nIndex2;
                    sText.at(nIndex2) = '\0';

                    if (nLine < nItems)
                        ListView_SetItem(m_hWnd, &item);
                    else
                        ListView_InsertItem(m_hWnd, &item);

                    item.pszText = nIndex < sText.length() ? &sText.at(nIndex) : &sText.at(nIndex2);
                    item.iItem = gsl::narrow_cast<int>(++nLine);

                    nStop = nIndex;
                }
            }
            else
            {
                for (unsigned int nIndex = 1; nIndex < pItemMetrics.nNumLines; ++nIndex)
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

    if (gsl::narrow_cast<gsl::index>(nItems) > nLine)
    {
        for (gsl::index i = gsl::narrow_cast<gsl::index>(nItems) - 1; i >= nLine; --i)
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
        m_vmItems->RemoveNotifyTarget(*this); // ignore IsSelected changes
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
            for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vmItems->Count()); ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, nIndex == nmItemActivate.iItem);
        }

        m_vmItems->EndUpdate();
        m_vmItems->AddNotifyTarget(*this);

        m_nLastClickedItem = nmItemActivate.iItem;

        SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hWnd, nullptr, FALSE);
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

void MultiLineGridBinding::EnsureVisible(gsl::index nIndex)
{
    if (nIndex < gsl::narrow_cast<gsl::index>(m_vItemMetrics.size()))
    {
        const auto nLine = m_vItemMetrics.at(nIndex).nFirstLine;
        ListView_EnsureVisible(m_hWnd, nLine, FALSE);
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
