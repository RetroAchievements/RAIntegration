#include "GridBinding.hh"

#include "RA_StringUtils.h"

#include "GridCheckBoxColumnBinding.hh"

#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

GridBinding::~GridBinding()
{
    if (m_vmItems != nullptr)
        m_vmItems->RemoveNotifyTarget(*this);
}

void GridBinding::BindColumn(gsl::index nColumn, std::unique_ptr<GridColumnBinding> pColumnBinding)
{
    if (dynamic_cast<GridCheckBoxColumnBinding*>(pColumnBinding.get()) != nullptr)
    {
        if (nColumn != 0)
            return;
    }

    if (m_vColumns.size() <= ra::to_unsigned(nColumn))
    {
        if (m_vColumns.capacity() == 0)
            m_vColumns.reserve(4U);

        m_vColumns.resize(nColumn + 1);
    }

    m_vColumns.at(nColumn) = std::move(pColumnBinding);

    if (m_hWnd)
    {
        UpdateLayout();

        if (m_vmItems)
        {
            DisableBinding();
            UpdateItems(nColumn);
            EnableBinding();
        }
    }
}

void GridBinding::BindItems(ViewModelCollectionBase& vmItems)
{
    if (m_vmItems != nullptr)
        m_vmItems->RemoveNotifyTarget(*this);

    m_vmItems = &vmItems;
    m_vmItems->AddNotifyTarget(*this);

    if (m_hWnd && !m_vColumns.empty())
        UpdateAllItems();
}

void GridBinding::UpdateLayout()
{
    if (m_vColumns.empty())
        return;

    DWORD exStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
    if (m_bShowGridLines)
        exStyle |= LVS_EX_GRIDLINES;
    if (dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get()) != nullptr)
        exStyle |= LVS_EX_CHECKBOXES;
    ListView_SetExtendedListViewStyle(m_hWnd, exStyle);

    // calculate the column widths
    RECT rcList;
    GetClientRect(m_hWnd, &rcList);

    const int nWidth = rcList.right - rcList.left;
    int nRemaining = nWidth;
    int nFillParts = 0;
    std::vector<int> vWidths;
    vWidths.resize(m_vColumns.size());

    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        switch (pColumn.GetWidthType())
        {
        case GridColumnBinding::WidthType::Pixels:
            vWidths.at(i) = pColumn.GetWidth();
            break;

        case GridColumnBinding::WidthType::Percentage:
            vWidths.at(i) = nWidth * pColumn.GetWidth() / 100;
            break;

        default:
            nFillParts += pColumn.GetWidth();
            vWidths.at(i) = 0;
            break;
        }

        nRemaining -= vWidths.at(i);
    }

    if (nFillParts > 0)
    {
        for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
        {
            const auto& pColumn = *m_vColumns.at(i);
            if (pColumn.GetWidthType() == GridColumnBinding::WidthType::Fill)
                vWidths.at(i) = nRemaining * pColumn.GetWidth() / nFillParts;
        }
    }

    // update or insert the columns
    const auto nColumns = m_nColumnsCreated;
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        const auto sHeader = NativeStr(pColumn.GetHeader());

        LV_COLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_FIXED_WIDTH;
        col.cx = vWidths.at(i);
        GSL_SUPPRESS_TYPE3 col.pszText = const_cast<LPSTR>(sHeader.data());
        col.iSubItem = gsl::narrow_cast<int>(i);

        switch (pColumn.GetAlignment())
        {
            default:
                col.fmt |= LVCFMT_LEFT;
                break;

            case ra::ui::RelativePosition::Center:
                col.fmt |= LVCFMT_CENTER;
                break;

            case ra::ui::RelativePosition::Far:
                col.fmt |= LVCFMT_RIGHT;
                break;
        }

        if (i < ra::to_signed(nColumns))
            ListView_SetColumn(m_hWnd, i, &col);
        else
            ListView_InsertColumn(m_hWnd, i, &col);
    }

    // remove any extra columns
    if (nColumns > m_vColumns.size())
    {
        for (gsl::index i = nColumns - 1; ra::to_unsigned(i) >= m_vColumns.size(); --i)
            ListView_DeleteColumn(m_hWnd, i);
    }

    // store the number of columns we know to be present since the suggested way to
    // retrieve that number doesn't always work.
    m_nColumnsCreated = m_vColumns.size();
}

void GridBinding::UpdateAllItems()
{
    RECT rcClientBefore{};
    if (m_hWnd)
        ::GetClientRect(m_hWnd, &rcClientBefore);

    DisableBinding();

    for (gsl::index nColumn = 0; ra::to_unsigned(nColumn) < m_vColumns.size(); ++nColumn)
        UpdateItems(nColumn);

    EnableBinding();

    if (m_hWnd)
    {
        CheckForScrollBar();

        // if a scrollbar is added or removed, the client size will change
        RECT rcClientAfter;
        ::GetClientRect(m_hWnd, &rcClientAfter);
        if (rcClientAfter.right != rcClientBefore.right)
            UpdateLayout();
    }
}

void GridBinding::UpdateItems(gsl::index nColumn)
{
    const auto& pColumn = *m_vColumns.at(nColumn);

    const auto nItems = ListView_GetItemCount(m_hWnd);

    const auto* pCheckBoxColumn = dynamic_cast<const GridCheckBoxColumnBinding*>(&pColumn);
    if (pCheckBoxColumn != nullptr)
    {
        LV_ITEM item{};
        item.mask = LVIF_TEXT;
        item.iSubItem = gsl::narrow_cast<int>(nColumn);
        GSL_SUPPRESS_TYPE3 item.pszText = const_cast<LPSTR>("");

        const auto& pBoundProperty = pCheckBoxColumn->GetBoundProperty();

        for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
        {
            if (i >= nItems)
            {
                item.iItem = gsl::narrow_cast<int>(i);
                ListView_InsertItem(m_hWnd, &item);
            }

            ListView_SetCheckState(m_hWnd, i, m_vmItems->GetItemValue(i, pBoundProperty));
        }
    }
    else
    {
        std::string sText;

        LV_ITEM item{};
        item.mask = LVIF_TEXT;
        item.iSubItem = gsl::narrow_cast<int>(nColumn);

        for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
        {
            sText = NativeStr(pColumn.GetText(*m_vmItems, i));
            item.pszText = sText.data();
            item.iItem = gsl::narrow_cast<int>(i);

            if (i < nItems)
                ListView_SetItem(m_hWnd, &item);
            else
                ListView_InsertItem(m_hWnd, &item);
        }
    }

    if (ra::to_unsigned(nItems) > m_vmItems->Count())
    {
        for (gsl::index i = nItems - 1; ra::to_unsigned(i) >= m_vmItems->Count(); --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

void GridBinding::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (m_pRowColorProperty && *m_pRowColorProperty == args.Property)
    {
        ListView_RedrawItems(m_hWnd, nIndex, nIndex);
        return;
    }

    std::string sText;
    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            item.iSubItem = gsl::narrow_cast<int>(nColumnIndex);
            sText = NativeStr(pColumn.GetText(*m_vmItems, nIndex));
            item.pszText = sText.data();
            ListView_SetItem(m_hWnd, &item);
        }
    }
}

void GridBinding::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    std::string sText;
    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            item.iSubItem = gsl::narrow_cast<int>(nColumnIndex);
            sText = NativeStr(pColumn.GetText(*m_vmItems, nIndex));
            item.pszText = sText.data();
            ListView_SetItem(m_hWnd, &item);
        }
    }
}

void GridBinding::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    std::string sText;
    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            item.iSubItem = gsl::narrow_cast<int>(nColumnIndex);
            sText = NativeStr(pColumn.GetText(*m_vmItems, nIndex));
            item.pszText = sText.data();
            ListView_SetItem(m_hWnd, &item);
        }
    }
}

int GridBinding::UpdateSelected(const IntModelProperty& pProperty, int nNewValue)
{
    if (m_pIsSelectedProperty == nullptr)
        return 0;

    int nUpdated = 0;

    const auto nCount = ra::to_signed(m_vmItems->Count());
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        if (m_vmItems->GetItemValue(nIndex, *m_pIsSelectedProperty))
        {
            m_vmItems->SetItemValue(nIndex, pProperty, nNewValue);
            ++nUpdated;
        }
    }

    return nUpdated;
}

void GridBinding::UpdateRow(gsl::index nIndex, bool bExisting)
{
    std::string sText;

    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);
    item.iSubItem = 0;

    const auto& pColumn = *m_vColumns.at(0);

    sText = NativeStr(pColumn.GetText(*m_vmItems, nIndex));
    item.pszText = sText.data();

    if (bExisting)
        ListView_SetItem(m_hWnd, &item);
    else
        ListView_InsertItem(m_hWnd, &item);

    const auto* pCheckBoxColumn = dynamic_cast<const GridCheckBoxColumnBinding*>(&pColumn);
    if (pCheckBoxColumn != nullptr)
    {
        const auto& pBoundProperty = pCheckBoxColumn->GetBoundProperty();
        ListView_SetCheckState(m_hWnd, nIndex, m_vmItems->GetItemValue(nIndex, pBoundProperty));
    }

    for (gsl::index i = 1; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        sText = NativeStr(m_vColumns.at(i)->GetText(*m_vmItems, nIndex));
        if (!sText.empty())
        {
            item.pszText = sText.data();
            item.iSubItem = gsl::narrow_cast<int>(i);
            ListView_SetItem(m_hWnd, &item);
        }
    }

    if (m_pIsSelectedProperty)
    {
        const bool bSelected = m_vmItems->GetItemValue(nIndex, *m_pIsSelectedProperty);
        if (bSelected || bExisting)
            ListView_SetItemState(m_hWnd, nIndex, bSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
    }
}

void GridBinding::OnViewModelAdded(gsl::index nIndex)
{
    if (!m_hWnd || m_vColumns.empty())
        return;

    // when an item is added, we can't assume the list is still sorted
    m_nSortIndex = -1;

    UpdateRow(nIndex, false);

    if (!m_vmItems->IsUpdating())
        CheckForScrollBar();
}

void GridBinding::OnViewModelRemoved(gsl::index nIndex)
{
    if (m_hWnd)
    {
        ListView_DeleteItem(m_hWnd, nIndex);

        if (!m_vmItems->IsUpdating())
            CheckForScrollBar();
    }
}

void GridBinding::OnViewModelChanged(gsl::index nIndex)
{
    if (m_hWnd)
        UpdateRow(nIndex, true);
}

void GridBinding::CheckForScrollBar()
{
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_PAGE;
    const bool bHasScrollbar = GetScrollInfo(m_hWnd, SBS_VERT, &info) &&
        (info.nPage > 0) && (info.nPage < m_vmItems->Count());

    if (bHasScrollbar != m_bHasScrollbar)
    {
        m_bHasScrollbar = bHasScrollbar;
        UpdateLayout();
    }
}

void GridBinding::OnBeginViewModelCollectionUpdate() noexcept
{
    if (m_hWnd)
        SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
}

void GridBinding::OnEndViewModelCollectionUpdate()
{
    if (m_hWnd)
    {
        CheckForScrollBar();

        SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void GridBinding::BindIsSelected(const BoolModelProperty& pIsSelectedProperty) noexcept
{
    m_pIsSelectedProperty = &pIsSelectedProperty;
}

void GridBinding::BindRowColor(const IntModelProperty& pRowColorProperty) noexcept
{
    m_pRowColorProperty = &pRowColorProperty;
}

LRESULT GridBinding::OnLvnItemChanging(const LPNMLISTVIEW pnmListView)
{
    // if an item is being unselected
    if (pnmListView->uChanged & LVIF_STATE && !(pnmListView->uNewState & LVIS_SELECTED) && pnmListView->uOldState & LVIS_SELECTED)
    {
        // and the in-place editor is not open
        if (m_hInPlaceEditor == nullptr)
        {
            // determine which row/column the user actually clicked on
            LVHITTESTINFO lvHitTestInfo{};
            GetCursorPos(&lvHitTestInfo.pt);
            ScreenToClient(m_hWnd, &lvHitTestInfo.pt);
            ListView_SubItemHitTest(m_hWnd, &lvHitTestInfo);

            // if the click was for the currently focused item, the in-place editor might open. check to see if the column supports editing
            if (lvHitTestInfo.iItem == pnmListView->iItem && ListView_GetItemState(m_hWnd, lvHitTestInfo.iItem, LVIS_FOCUSED))
            {
                if (ra::to_unsigned(lvHitTestInfo.iSubItem) < m_vColumns.size())
                {
                    // the in-place editor will open. prevent the item from being unselected.
                    const auto& pColumn = m_vColumns.at(lvHitTestInfo.iSubItem);
                    if (!pColumn->IsReadOnly())
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void GridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    if (m_pIsSelectedProperty)
    {
        if (pnmListView->uNewState & LVIS_SELECTED)
            m_vmItems->SetItemValue(pnmListView->iItem, *m_pIsSelectedProperty, true);
        else if (pnmListView->uOldState & LVIS_SELECTED)
            m_vmItems->SetItemValue(pnmListView->iItem, *m_pIsSelectedProperty, false);
    }

    switch (pnmListView->uNewState & 0x3000)
    {
        case 0x2000: // LVIS_CHECKED
        {
            const auto* pCheckBoxBinding = dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get());
            if (pCheckBoxBinding != nullptr)
            {
                m_vmItems->RemoveNotifyTarget(*this);
                m_vmItems->SetItemValue(pnmListView->iItem, pCheckBoxBinding->GetBoundProperty(), true);
                m_vmItems->AddNotifyTarget(*this);
            }
        }
        break;

        case 0x1000: // LVIS_UNCHECKED
        {
            const auto* pCheckBoxBinding = dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get());
            if (pCheckBoxBinding != nullptr)
            {
                m_vmItems->RemoveNotifyTarget(*this);
                m_vmItems->SetItemValue(pnmListView->iItem, pCheckBoxBinding->GetBoundProperty(), false);
                m_vmItems->AddNotifyTarget(*this);
            }
        }
        break;
    }

    if (!m_hInPlaceEditor && pnmListView->uNewState & LVIS_SELECTED)
        m_tFocusTime = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
}

void GridBinding::OnLvnColumnClick(const LPNMLISTVIEW pnmListView)
{
    const auto nCount = ra::to_signed(m_vmItems->Count());

    // if the data is already sorted by the selected column, just reverse the items
    if (m_nSortIndex == pnmListView->iSubItem)
    {
        m_vmItems->Reverse();

        // update the focus rectangle
        for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
        {
            if (ListView_GetItemState(m_hWnd, nIndex, LVIS_FOCUSED))
            {
                ListView_SetItemState(m_hWnd, nCount - 1 - nIndex, LVIS_FOCUSED, LVIS_FOCUSED);
                break;
            }
        }

        return;
    }

    gsl::index nFocusedIndex = -1;

    m_nSortIndex = pnmListView->iSubItem;
    const auto& pColumnBinding = *m_vColumns.at(m_nSortIndex);

    // sort the items into buckets associated with their key.
    // in many cases, the key will be unique per item
    std::map<std::wstring, std::vector<gsl::index>> mSortMap;
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        std::wstring sKey = pColumnBinding.GetSortKey(*m_vmItems, nIndex);
        mSortMap[sKey].push_back(nIndex);

        if (nFocusedIndex == -1)
        {
            if (ListView_GetItemState(m_hWnd, nIndex, LVIS_FOCUSED))
                nFocusedIndex = nIndex;
        }
    }

    // flatten the map out into a sorted list. relies on std::map being sorted by key.
    std::vector<gsl::index> vSorted;
    vSorted.reserve(nCount);
    for (const auto& pPair : mSortMap)
    {
        for (const auto nIndex : pPair.second)
            vSorted.push_back(nIndex);
    }

    // update the focus rectangle
    if (nFocusedIndex >= 0 && vSorted.at(nFocusedIndex) != nFocusedIndex)
    {
        for (gsl::index nIndex = nCount - 1; nIndex >= 0; --nIndex)
        {
            if (vSorted.at(nIndex) == nFocusedIndex)
            {
                ListView_SetItemState(m_hWnd, nIndex, LVIS_FOCUSED, LVIS_FOCUSED);
                break;
            }
        }
    }

    // move the items around to match the new order
    m_vmItems->BeginUpdate();
    for (gsl::index nIndex = nCount - 1; nIndex >= 0; --nIndex)
    {
        const auto nOldIndex = vSorted.at(nIndex);
        if (nOldIndex != nIndex)
        {
            m_vmItems->MoveItem(nOldIndex, nIndex);

            for (auto pIter = vSorted.begin(); pIter < vSorted.begin() + nIndex; ++pIter)
            {
                if (*pIter > nOldIndex)
                    *pIter = (*pIter) - 1;
            }
        }
    }
    m_vmItems->EndUpdate();
}

void GridBinding::OnGotFocus()
{
    if (!m_hInPlaceEditor)
    {
        const auto tNow = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
        const auto tElapsed = tNow - m_tIPECloseTime;
        if (tElapsed > std::chrono::milliseconds(200))
            m_tFocusTime = tNow;
    }
}

void GridBinding::OnLostFocus() noexcept
{
    if (!m_hInPlaceEditor)
        m_tFocusTime = {};
}

LRESULT GridBinding::OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw)
{
    if (m_pRowColorProperty)
    {
        switch (pCustomDraw->nmcd.dwDrawStage)
        {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT:
            {
                const auto nIndex = gsl::narrow_cast<gsl::index>(pCustomDraw->nmcd.dwItemSpec);
                const Color pColor(ra::to_unsigned(m_vmItems->GetItemValue(nIndex, *m_pRowColorProperty)));
                if (pColor.Channel.A != 0)
                    pCustomDraw->clrTextBk = RGB(pColor.Channel.R, pColor.Channel.G, pColor.Channel.B);
                return CDRF_DODEFAULT;
            }
        }
    }

    return CDRF_DODEFAULT;
}

void GridBinding::OnNmClick(const NMITEMACTIVATE* pnmItemActivate)
{
    // if an in-place editor is open, close it.
    if (m_hInPlaceEditor)
        SendMessage(m_hInPlaceEditor, WM_KILLFOCUS, 0, 0);

    if (ra::to_unsigned(pnmItemActivate->iSubItem) < m_vColumns.size())
    {
        // if the click also caused a focus event, ignore it
        const auto nElapsed = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime() - m_tFocusTime;
        if (nElapsed < std::chrono::milliseconds(200))
            return;

        const auto& pColumn = m_vColumns.at(pnmItemActivate->iSubItem);
        if (!pColumn->IsReadOnly())
        {
            auto pInfo = std::make_unique<GridColumnBinding::InPlaceEditorInfo>();
            pInfo->bIgnoreLostFocus = false;

            if (pnmItemActivate->iItem >= 0)
            {
                pInfo->nItemIndex = pnmItemActivate->iItem;
            }
            else
            {
                LVHITTESTINFO lvHitTestInfo{};
                lvHitTestInfo.pt = pnmItemActivate->ptAction;
                ListView_SubItemHitTest(m_hWnd, &lvHitTestInfo);
                pInfo->nItemIndex = lvHitTestInfo.iItem;
            }

            if (pInfo->nItemIndex >= 0)
            {
                pInfo->nColumnIndex = pnmItemActivate->iSubItem;
                pInfo->pGridBinding = this;
                pInfo->pColumnBinding = pColumn.get();

                RECT rcOffset{};
                GetWindowRect(m_hWnd, &rcOffset);

                GSL_SUPPRESS_ES47 ListView_GetSubItemRect(m_hWnd, pInfo->nItemIndex,
                    gsl::narrow_cast<int>(pInfo->nColumnIndex), LVIR_BOUNDS, &pInfo->rcSubItem);
                pInfo->rcSubItem.left += rcOffset.left + 1;
                pInfo->rcSubItem.right += rcOffset.left + 1;
                pInfo->rcSubItem.top += rcOffset.top + 1;
                pInfo->rcSubItem.bottom += rcOffset.top + 1;

                // the SubItemRect for the first column contains the entire row, adjust to just the first column
                if (pInfo->nColumnIndex == 0 && m_vColumns.size() > 1)
                {
                    RECT rcSecondColumn{};
                    GSL_SUPPRESS_ES47 ListView_GetSubItemRect(m_hWnd, pInfo->nItemIndex, 1, LVIR_BOUNDS, &rcSecondColumn);
                    pInfo->rcSubItem.right = rcSecondColumn.left + rcOffset.left + 1;
                }

                const HWND hParent = GetAncestor(m_hWnd, GA_ROOT);
                m_hInPlaceEditor = pColumn->CreateInPlaceEditor(hParent, *pInfo);
                if (m_hInPlaceEditor)
                {
                    GSL_SUPPRESS_TYPE1 SetWindowLongPtr(m_hInPlaceEditor, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pInfo.release()));
                    SetFocus(m_hInPlaceEditor);
                    return;
                }
            }
        }
    }
}

void GridBinding::OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate)
{
    if (ra::to_unsigned(pnmItemActivate->iSubItem) < m_vColumns.size())
    {
        const auto& pColumn = m_vColumns.at(pnmItemActivate->iSubItem);
        if (pColumn->HandleDoubleClick(GetItems(), pnmItemActivate->iItem))
            return;
    }

    // TODO: row-level double-click handler
}

GridColumnBinding::InPlaceEditorInfo* GridBinding::GetIPEInfo(HWND hwnd) noexcept
{
    GridColumnBinding::InPlaceEditorInfo* pInfo;
    GSL_SUPPRESS_TYPE1 pInfo = reinterpret_cast<GridColumnBinding::InPlaceEditorInfo*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    return pInfo;
}

LRESULT GridBinding::CloseIPE(HWND hwnd, GridColumnBinding::InPlaceEditorInfo* pInfo)
{
    GridBinding* gridBinding = static_cast<GridBinding*>(pInfo->pGridBinding);
    Expects(gridBinding != nullptr);

    pInfo->bIgnoreLostFocus = true;

    // DestroyWindow will free pInfo - cannot use after this point
    DestroyWindow(hwnd);

    if (gridBinding->m_hInPlaceEditor == hwnd)
    {
        gridBinding->m_tIPECloseTime = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
        gridBinding->m_hInPlaceEditor = nullptr;
    }

    return 0;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
