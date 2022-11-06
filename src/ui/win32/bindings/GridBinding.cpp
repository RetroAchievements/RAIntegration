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
    {
        if (ra::services::ServiceLocator::IsInitialized())
            m_vmItems->RemoveNotifyTarget(*this);
    }

    if (m_hTooltip)
    {
        RemoveSecondaryControlBinding(m_hTooltip);
        DestroyWindow(m_hTooltip);
    }
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

    if (pColumnBinding->GetTextColorProperty() != nullptr)
        m_bHasColoredColumns = true;

    m_vColumns.at(nColumn) = std::move(pColumnBinding);

    if (m_hWnd)
    {
        UpdateLayout();

        if (m_vmItems)
            RefreshColumn(nColumn);
    }
}

void GridBinding::RefreshColumn(gsl::index nColumn)
{
    DisableBinding();
    UpdateItems(nColumn);
    EnableBinding();
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

    constexpr int nDefaultDPI = 96;
    int nDPI = 0;

    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        switch (pColumn.GetWidthType())
        {
        case GridColumnBinding::WidthType::Pixels:
            if (nDPI == 0)
            {
                const HDC hScreen = GetDC(nullptr);
                nDPI = GetDeviceCaps(hScreen, LOGPIXELSX);
                ReleaseDC(nullptr, hScreen);

                if (nDPI == 0)
                    nDPI = nDefaultDPI;
            }

            vWidths.at(i) = pColumn.GetWidth() * nDPI / nDefaultDPI;
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

        LV_COLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_FIXED_WIDTH;
        col.cx = vWidths.at(i);
        GSL_SUPPRESS_TYPE3 col.pszText = const_cast<LPWSTR>(pColumn.GetHeader().data());
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

        GSL_SUPPRESS_TYPE1
        SNDMSG(m_hWnd, (i < ra::to_signed(nColumns)) ? LVM_SETCOLUMNW : LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&col));
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
    m_vColumnWidths.swap(vWidths);
}

void GridBinding::UpdateAllItems()
{
    if (!m_hWnd)
        return;

    RECT rcClientBefore{};
    ::GetClientRect(m_hWnd, &rcClientBefore);

    DisableBinding();

    for (gsl::index nColumn = 0; ra::to_unsigned(nColumn) < m_vColumns.size(); ++nColumn)
        UpdateItems(nColumn);

    UpdateSelectedItemStates();

    EnableBinding();

    CheckForScrollBar();

    // if a scrollbar is added or removed, the client size will change
    RECT rcClientAfter;
    ::GetClientRect(m_hWnd, &rcClientAfter);
    if (rcClientAfter.right != rcClientBefore.right)
        UpdateLayout();
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
        GSL_SUPPRESS_TYPE3 item.pszText = const_cast<LPTSTR>(TEXT(""));

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
        std::wstring sText;

        LV_ITEMW item{};
        item.mask = LVIF_TEXT;
        item.iSubItem = gsl::narrow_cast<int>(nColumn);

        for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
        {
            sText = pColumn.GetText(*m_vmItems, i);
            item.pszText = sText.data();
            item.iItem = gsl::narrow_cast<int>(i);

            GSL_SUPPRESS_TYPE1
            SNDMSG(m_hWnd, (i < nItems) ? LVM_SETITEMW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
        }
    }

    if (ra::to_unsigned(nItems) > m_vmItems->Count())
    {
        for (gsl::index i = gsl::narrow_cast<gsl::index>(nItems) - 1; ra::to_unsigned(i) >= m_vmItems->Count(); --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

void GridBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_pScrollOffsetProperty)
    {
        if (*m_pScrollOffsetProperty == args.Property)
        {
            m_nScrollOffset = args.tNewValue;

            const auto nTopIndex = ListView_GetTopIndex(m_hWnd);
            const auto nSpacing = HIWORD(ListView_GetItemSpacing(m_hWnd, TRUE));
            const int nDeltaY = (m_nScrollOffset - nTopIndex) * gsl::narrow_cast<int>(nSpacing);
            ListView_Scroll(m_hWnd, 0, nDeltaY);
            return;
        }

        if (*m_pScrollMaximumProperty == args.Property)
        {
            if (m_hWnd)
            {
                if (args.tNewValue < 1)
                    ListView_SetItemCountEx(m_hWnd, 0, 0);
                else
                    ListView_SetItemCountEx(m_hWnd, gsl::narrow_cast<size_t>(args.tNewValue), LVSICF_NOSCROLL);

                CheckForScrollBar();
            }
            return;
        }
    }
    else if (m_pEnsureVisibleProperty && *m_pEnsureVisibleProperty == args.Property)
    {
        if (args.tNewValue >= 0)
        {
            EnsureVisible(gsl::narrow_cast<gsl::index>(args.tNewValue));
            SetValue(*m_pEnsureVisibleProperty, -1);
        }
    }
}

void GridBinding::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    // when virtualizing, only the visible items have view models. adjust the index accordingly.
    nIndex = GetRealItemIndex(nIndex);

    if (m_pRowColorProperty && *m_pRowColorProperty == args.Property)
    {
        if (m_nAdjustingScrollOffset == 0)
        {
            ListView_RedrawItems(m_hWnd, nIndex, nIndex);
            m_bForceRepaintItems = true;
        }
        return;
    }

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            SuspendRedraw();

            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            UpdateCell(nIndex, nColumnIndex);

            ListView_RedrawItems(m_hWnd, nIndex, nIndex);
            m_bForceRepaintItems = true;
        }
    }

    if (!m_vmItems->IsUpdating())
        OnEndViewModelCollectionUpdate();
}

void GridBinding::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    // when virtualizing, only the visible items have view models. adjust the index accordingly.
    nIndex = GetRealItemIndex(nIndex);

    if (m_pIsSelectedProperty && *m_pIsSelectedProperty == args.Property)
        ListView_SetItemState(m_hWnd, nIndex, args.tNewValue ? LVIS_SELECTED : 0, LVIS_SELECTED);

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            SuspendRedraw();

            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            UpdateCell(nIndex, nColumnIndex);

            ListView_RedrawItems(m_hWnd, nIndex, nIndex);
            m_bForceRepaintItems = true;
        }
    }
}

void GridBinding::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    // when virtualizing, only the visible items have view models. adjust the index accordingly.
    nIndex = GetRealItemIndex(nIndex);

    for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
    {
        const auto& pColumn = *m_vColumns.at(nColumnIndex);
        if (pColumn.DependsOn(args.Property))
        {
            SuspendRedraw();

            // if the affected data is in the sort column, it's no longer sorted
            if (m_nSortIndex == ra::to_signed(nColumnIndex))
                m_nSortIndex = -1;

            UpdateCell(nIndex, nColumnIndex);

            ListView_RedrawItems(m_hWnd, nIndex, nIndex);
            m_bForceRepaintItems = true;
        }
    }
}

void GridBinding::UpdateCell(gsl::index nIndex, gsl::index nColumnIndex)
{
    std::wstring sText;
    LV_ITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);
    item.iSubItem = gsl::narrow_cast<int>(nColumnIndex);

    const auto& pColumn = *m_vColumns.at(nColumnIndex);
    sText = pColumn.GetText(*m_vmItems, nIndex);
    item.pszText = sText.data();

    GSL_SUPPRESS_TYPE1
    SNDMSG(m_hWnd, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&item));
}

void GridBinding::EnsureVisible(gsl::index nIndex)
{
    if (nIndex >= 0 && nIndex < gsl::narrow_cast<gsl::index>(m_vmItems->Count()))
        ListView_EnsureVisible(m_hWnd, gsl::narrow_cast<int>(nIndex), FALSE);
}

void GridBinding::DeselectAll() noexcept
{
    int nScan = - 1;
    while ((nScan = ListView_GetNextItem(m_hWnd, nScan, LVNI_SELECTED)) != -1)
        ListView_SetItemState(m_hWnd, nScan, 0, LVIS_SELECTED);
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
    // virtualized listview does not support LVM_INSERTITEM or LVM_SETITEM
    if (m_pUpdateSelectedItems)
        return;

    std::wstring sText;

    LV_ITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nIndex);
    item.iSubItem = 0;

    const auto& pColumn = *m_vColumns.at(0);
    nIndex -= m_nScrollOffset;

    sText = pColumn.GetText(*m_vmItems, nIndex);
    item.pszText = sText.data();

    GSL_SUPPRESS_TYPE1
    SNDMSG(m_hWnd, bExisting ? LVM_SETITEMW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

    const auto* pCheckBoxColumn = dynamic_cast<const GridCheckBoxColumnBinding*>(&pColumn);
    if (pCheckBoxColumn != nullptr)
    {
        const auto& pBoundProperty = pCheckBoxColumn->GetBoundProperty();
        ListView_SetCheckState(m_hWnd, item.iItem, m_vmItems->GetItemValue(nIndex, pBoundProperty));
    }

    for (gsl::index i = 1; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        sText = m_vColumns.at(i)->GetText(*m_vmItems, nIndex);

        item.pszText = sText.data();
        item.iSubItem = gsl::narrow_cast<int>(i);

        GSL_SUPPRESS_TYPE1
        SNDMSG(m_hWnd, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&item));
    }

    if (m_pIsSelectedProperty)
    {
        const bool bSelected = m_vmItems->GetItemValue(nIndex, *m_pIsSelectedProperty);
        if (bSelected || bExisting)
            ListView_SetItemState(m_hWnd, item.iItem, bSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
    }
}

void GridBinding::OnViewModelAdded(gsl::index nIndex)
{
    if (!m_hWnd || m_vColumns.empty())
        return;

    // when an item is added, we can't assume the list is still sorted
    m_nSortIndex = -1;

    if (m_pUpdateSelectedItems)
    {
        // don't actually add/update items in virtual listview
        m_bForceRepaint = true;
    }
    else
    {
        SuspendRedraw();
        UpdateRow(nIndex, false);
    }

    if (!m_vmItems->IsUpdating())
        CheckForScrollBar();
}

void GridBinding::OnViewModelRemoved(gsl::index nIndex)
{
    if (m_hWnd)
    {
        if (m_pUpdateSelectedItems)
        {
            // don't actually delete items from virtual listview
            m_bForceRepaint = true;
        }
        else
        {
            SuspendRedraw();
            ListView_DeleteItem(m_hWnd, nIndex);
        }

        if (!m_vmItems->IsUpdating())
            CheckForScrollBar();
    }
}

void GridBinding::OnViewModelChanged(gsl::index nIndex)
{
    if (m_hWnd)
    {
        SuspendRedraw();
        UpdateRow(nIndex, true);
    }
}

void GridBinding::CheckForScrollBar()
{
    int nItems = gsl::narrow_cast<int>(m_vmItems->Count());
    if (m_pScrollMaximumProperty)
        nItems = GetValue(*m_pScrollMaximumProperty);

    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_PAGE;
    const bool bHasScrollbar = GetScrollInfo(m_hWnd, SBS_VERT, &info) &&
        (info.nPage > 0) && (info.nPage < ra::to_unsigned(nItems));

    if (bHasScrollbar != m_bHasScrollbar)
    {
        // we think the scrollbar has appeared or disappeared - update the column widths
        m_bHasScrollbar = bHasScrollbar;
        UpdateLayout();
    }
}

void GridBinding::OnBeginViewModelCollectionUpdate() noexcept
{
    m_bForceRepaint = false;
    m_bForceRepaintItems = false;
}

void GridBinding::OnEndViewModelCollectionUpdate()
{
    if (m_hWnd)
    {
        if (m_bRedrawSuspended)
        {
            // enable redraw before calling CheckForScrollBar to ensure metrics are updated
            SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
            m_bRedrawSuspended = false;
        }

        CheckForScrollBar();

        if (m_bUpdateSelectedItemStates)
        {
            m_bUpdateSelectedItemStates = false;
            UpdateSelectedItemStates();
        }

        if (m_nAdjustingScrollOffset == 0)
        {
            if (m_bForceRepaint)
            {
                m_bForceRepaint = false;
                m_bForceRepaintItems = false;

                ControlBinding::ForceRepaint(m_hWnd);
            }
            else if (m_bForceRepaintItems)
            {
                m_bForceRepaintItems = false;
                ControlBinding::RedrawWindow(m_hWnd);
            }
        }
    }
}

void GridBinding::SuspendRedraw() noexcept
{
    if (!m_bRedrawSuspended && m_vmItems->IsUpdating())
    {
        // if we're in a BeginUpdate/EndUpdate block, stop redrawing until the EndUpdate
        m_bRedrawSuspended = true;
        SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
    }
}

void GridBinding::UpdateSelectedItemStates()
{
    if (m_pIsSelectedProperty && !m_pUpdateSelectedItems)
    {
        for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
        {
            const bool bSelected = m_vmItems->GetItemValue(nIndex, *m_pIsSelectedProperty);
            ListView_SetItemState(m_hWnd, nIndex, bSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
        }
    }
}

void GridBinding::BindIsSelected(const BoolModelProperty& pIsSelectedProperty)
{
    m_pIsSelectedProperty = &pIsSelectedProperty;

    if (m_hWnd)
        UpdateSelectedItemStates();
}

void GridBinding::BindEnsureVisible(const IntModelProperty& pEnsureVisibleProperty) noexcept
{
    m_pEnsureVisibleProperty = &pEnsureVisibleProperty;
}

void GridBinding::BindRowColor(const IntModelProperty& pRowColorProperty) noexcept
{
    m_pRowColorProperty = &pRowColorProperty;
}

void GridBinding::SetDoubleClickHandler(std::function<void(gsl::index)> pHandler)
{
    m_pDoubleClickHandler = pHandler;
}

void GridBinding::SetCopyHandler(std::function<void()> pHandler)
{
    m_pCopyHandler = pHandler;
}

void GridBinding::SetPasteHandler(std::function<void()> pHandler)
{
    m_pPasteHandler = pHandler;
}

void GridBinding::Virtualize(const IntModelProperty& pScrollOffsetProperty, const IntModelProperty& pScrollMaximumProperty,
    std::function<void(gsl::index, gsl::index, bool)> pUpdateSelectedItems)
{
    if (m_hWnd)
    {
        Expects((GetWindowStyle(m_hWnd) & LVS_OWNERDATA) != 0);
    }

    m_pScrollOffsetProperty = &pScrollOffsetProperty;
    m_pScrollMaximumProperty = &pScrollMaximumProperty;
    m_pUpdateSelectedItems = pUpdateSelectedItems;

    m_nScrollOffset = GetValue(pScrollOffsetProperty);
}

void GridBinding::SetHWND(DialogBase& pDialog, HWND hControl)
{
    ControlBinding::SetHWND(pDialog, hControl);

    if (m_pScrollOffsetProperty)
    {
        Expects((GetWindowStyle(m_hWnd) & LVS_OWNERDATA) != 0);
    }

    if (!m_vColumns.empty())
    {
        UpdateLayout();

        if (m_vmItems)
            UpdateAllItems();
    }

#ifdef UNICODE
    // ListView control still sends LVN_GETDISPINFOA messages for LVS_OWNERDATA unless we explicitly tell it to use unicode
    if ((GetWindowStyle(m_hWnd) & LVS_OWNERDATA) != 0)
        ListView_SetUnicodeFormat(hControl, TRUE);
#endif
}

INT_PTR CALLBACK GridBinding::WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_MOUSEMOVE:
            if (m_hTooltip)
            {
                LVHITTESTINFO lvHitTestInfo;
                GetCursorPos(&lvHitTestInfo.pt);
                ScreenToClient(m_hWnd, &lvHitTestInfo.pt);

                int nTooltipLocation = -1;
                if (ListView_SubItemHitTest(m_hWnd, &lvHitTestInfo) != -1)
                    nTooltipLocation = lvHitTestInfo.iItem * 256 + lvHitTestInfo.iSubItem;

                // if the mouse has moved to a new grid cell, hide the toolip
                if (nTooltipLocation != m_nTooltipLocation)
                {
                    m_nTooltipLocation = nTooltipLocation;
                    m_sTooltip.clear();

                    if (IsWindowVisible(m_hTooltip))
                    {
                        // tooltip is visible, just hide it
                        SendMessage(m_hTooltip, TTM_POP, 0, 0);
                    }
                    else
                    {
                        // tooltip is not visible. if we told the tooltip to not display in the TTM_GETDISPINFO handler by
                        // setting lpszText to empty string, it won't try to display again unless we tell it to, so do so now
                        SendMessage(m_hTooltip, TTM_POPUP, 0, 0);

                        // but we don't want the tooltip to immediately display when entering a new cell, so immediately
                        // hide it. the normal hover behavior will make it display as expected after a normal hover wait time.
                        SendMessage(m_hTooltip, TTM_POP, 0, 0);
                    }
                }
            }
            break;
    }

    return ControlBinding::WndProc(hControl, uMsg, wParam, lParam);
}

void GridBinding::InitializeTooltips(std::chrono::milliseconds nDisplayTime)
{
    const HWND hDialog = GetDialogHwnd();
    Expects(hDialog != nullptr);

    m_hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hDialog, nullptr, GetModuleHandle(nullptr), nullptr);

    if (m_hTooltip)
    {
        TOOLINFO toolInfo;
        memset(&toolInfo, 0, sizeof(toolInfo));
        GSL_SUPPRESS_ES47 toolInfo.cbSize = TTTOOLINFO_V1_SIZE;
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.hwnd = hDialog;
        GSL_SUPPRESS_TYPE1 toolInfo.uId = reinterpret_cast<UINT_PTR>(m_hWnd);
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        GSL_SUPPRESS_TYPE1 SendMessage(m_hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
        SendMessage(m_hTooltip, TTM_ACTIVATE, TRUE, 0);
        SendMessage(m_hTooltip, TTM_SETMAXTIPWIDTH, 0, 320);
        SendMessage(m_hTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, gsl::narrow_cast<LPARAM>(nDisplayTime.count()));

        // install a mouse hook so we can show different tooltips per cell within the list view
        SubclassWndProc();

        // TTN_GETDISPINFO is raised against m_hTooltip, need to register this GridBinding against m_hTooltip
        AddSecondaryControlBinding(m_hTooltip);
    }
}

LRESULT GridBinding::OnLvnItemChanging(const LPNMLISTVIEW pnmListView)
{
    // if an item is being unselected
    if (pnmListView->uChanged & LVIF_STATE && !(pnmListView->uNewState & LVIS_SELECTED) && pnmListView->uOldState & LVIS_SELECTED)
    {
        // and the in-place editor is not open
        if (m_hInPlaceEditor == nullptr)
        {
            // when CTRL+clicking to open the in-place editor, the CTRL+click will try to deselect the item.
            // if CTRL is pressed and the in-place editor will open, prevent the deselect.
            if (GetAsyncKeyState(VK_LCONTROL) < 0 || GetAsyncKeyState(VK_RCONTROL) < 0)
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
    }

    return FALSE;
}

void GridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    if (pnmListView->iItem == -1)
    {
        if (m_pUpdateSelectedItems)
        {
            NMLVODSTATECHANGE pnmStateChange{};
            pnmStateChange.iFrom = 0;
            pnmStateChange.iTo = GetValue(*m_pScrollMaximumProperty) - 1;
            pnmStateChange.uNewState = pnmListView->uNewState;
            pnmStateChange.uOldState = pnmListView->uOldState;
            OnLvnOwnerDrawStateChanged(&pnmStateChange);
        }

        return;
    }

    // when virtualizing, only the visible items have view models. adjust the index accordingly.
    const auto nIndex = GetVisibleItemIndex(pnmListView->iItem);

    if (m_pIsSelectedProperty)
    {
        // don't update the property on the view model while the list is being updated. assume items are
        // being added/removed/moved and we'll reset the state correctly in OnEndViewModelCollectionUpdate.
        if (!m_vmItems->IsUpdating())
        {
            if (pnmListView->uNewState & LVIS_SELECTED)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, true);
            else if (pnmListView->uOldState & LVIS_SELECTED)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, false);
        }
        else
        {
            m_bUpdateSelectedItemStates = true;
        }
    }

    switch (pnmListView->uNewState & 0x3000)
    {
        case 0x2000: // LVIS_CHECKED
        {
            const auto* pCheckBoxBinding = dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get());
            if (pCheckBoxBinding != nullptr)
            {
                m_vmItems->RemoveNotifyTarget(*this);
                m_vmItems->SetItemValue(nIndex, pCheckBoxBinding->GetBoundProperty(), true);
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
                m_vmItems->SetItemValue(nIndex, pCheckBoxBinding->GetBoundProperty(), false);
                m_vmItems->AddNotifyTarget(*this);
            }
        }
        break;
    }

    if (!m_hInPlaceEditor && pnmListView->uNewState & LVIS_SELECTED)
        m_tFocusTime = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
}

void GridBinding::OnLvnOwnerDrawStateChanged(const LPNMLVODSTATECHANGE pnmStateChanged)
{
    // when virtualizing, notify the view model of the entire change, then update the
    // selected property on the visible items
    if (m_pUpdateSelectedItems &&
        (pnmStateChanged->uNewState ^ pnmStateChanged->uOldState) & LVIS_SELECTED)
    {
        const bool bSelected = pnmStateChanged->uNewState & LVIS_SELECTED;
        gsl::index nFrom = pnmStateChanged->iFrom;
        gsl::index nTo = pnmStateChanged->iTo;

        // notify the view model of all the changed items
        m_pUpdateSelectedItems(nFrom, nTo, bSelected);

        // explicitly update the items in the virtualization window
        if (m_pIsSelectedProperty)
        {
            if (nFrom < m_nScrollOffset)
                nFrom = 0;
            else
                nFrom -= m_nScrollOffset;

            if (nTo >= gsl::narrow_cast<gsl::index>(m_nScrollOffset) + ra::to_signed(m_vmItems->Count()))
                nTo = m_vmItems->Count() - 1;
            else
                nTo -= m_nScrollOffset;

            m_vmItems->RemoveNotifyTarget(*this);
            for (gsl::index nIndex = nFrom; nIndex <= nTo; ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, bSelected);
            m_vmItems->AddNotifyTarget(*this);
        }
    }
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

gsl::index GridBinding::GetRealItemIndex(gsl::index iItem) const
{
    // if not virtualizing, the index doesn't need to be modified
    if (!m_pScrollOffsetProperty)
        return iItem;

    // adjust for scroll offset
    const auto iAdjustedItem = iItem + m_nScrollOffset;

    // quick check to make sure the item is valid
    const auto nScrollMax = GetValue(*m_pScrollMaximumProperty);
    if (iAdjustedItem >= nScrollMax)
        return -1;

    return iAdjustedItem;
}

int GridBinding::GetVisibleItemIndex(int iItem)
{
    // if not virtualizing, the index doesn't need to be modified
    if (!m_pScrollOffsetProperty)
        return iItem;

    // make sure there's backing data
    const auto nItems = gsl::narrow_cast<int>(m_vmItems->Count());
    if (nItems == 0)
        return -1;

    // adjust for scroll offset
    const auto iAdjustedItem = iItem - m_nScrollOffset;
    if (iAdjustedItem >= 0 && iAdjustedItem < nItems)
        return iAdjustedItem;

    // quick check to make sure the item is valid
    const auto nScrollMax = GetValue(*m_pScrollMaximumProperty);
    if (iItem >= nScrollMax)
        return -1;

    // calculate what the scroll offset should be
    int nOffset = 0;
    if (iAdjustedItem < 0)
    {
        nOffset = m_nScrollOffset + iAdjustedItem;
        if (nOffset < 0)
            nOffset = 0;
    }
    else
    {
        const auto nPerPage = ListView_GetCountPerPage(m_hWnd);
        const auto nMaxOffset = nScrollMax - nPerPage;
        nOffset = m_nScrollOffset + iAdjustedItem - nPerPage;
        if (nOffset > nMaxOffset)
            nOffset = nMaxOffset;
    }

    // if it's correct, the requested item is not available
    if (m_nScrollOffset == nOffset)
        return -1;

    // update the scroll offset
    {
        // changing the scroll offset can cause the list to repaint, which may try to
        // adjust the scroll offset again. make sure it doesn't.
        ++m_nAdjustingScrollOffset;

        // SetValue detaches the change notification event, so we won't be notified.
        // Update the value manually. also, it's important to make sure that it's set
        // before notifying other targets in case they call back into us.
        m_nScrollOffset = nOffset;

        SetValue(*m_pScrollOffsetProperty, nOffset);

        --m_nAdjustingScrollOffset;

        ControlBinding::InvokeOnUIThread([this]()
        {
            if (m_bForceRepaint)
            {
                m_bForceRepaint = false;
                ControlBinding::ForceRepaint(m_hWnd);
            }
        });

        // attempt to scroll failed
        if (m_nScrollOffset != nOffset)
            return -1;
    }

    // readjust with new scroll offset
    iItem -= m_nScrollOffset;
    if (iItem >= 0 && iItem < nItems)
        return iItem;

    return -1;
}

static void CacheText(ra::tstring& sCacheString, const std::wstring& sText)
{
    // weird bug - assigning the NativeStr to the cache string seems to do some weird stuff with the
    // backing pointer, sometimes causing invalid memory to be displayed in the control. This forces
    // the string into the same buffer, instead of doing a swap() or whatever is happening.
#ifdef NEVER
    sCacheString = NativeStr(sText);
#else
    ra::tstring sNativeText = NativeStr(sText);
    sCacheString.resize(sNativeText.length());
    memcpy(sCacheString.data(), sNativeText.data(), (sNativeText.length() + 1) * 2);
#endif
}

void GridBinding::OnLvnGetDispInfo(NMLVDISPINFO& pnmDispInfo)
{
    // when virtualizing, only the visible items have view models. adjust the index accordingly.
    const auto nIndex = GetVisibleItemIndex(pnmDispInfo.item.iItem);

    // get the requested data
    CacheText(m_sDispInfo, m_vColumns.at(pnmDispInfo.item.iSubItem)->GetText(*m_vmItems, nIndex));
    GSL_SUPPRESS_TYPE3 pnmDispInfo.item.pszText = const_cast<LPTSTR>(m_sDispInfo.c_str());
}

void GridBinding::OnTooltipGetDispInfo(NMTTDISPINFO& pnmTtDispInfo)
{
    const auto nIndex = GetVisibleItemIndex(m_nTooltipLocation / 256);
    if (nIndex < 0 || nIndex >= gsl::narrow_cast<int>(m_vmItems->Count()))
        return;

    const auto iSubItem = m_nTooltipLocation % 256;
    if (iSubItem < 0 || iSubItem >= gsl::narrow_cast<int>(m_vColumns.size()))
        return;

    CacheText(m_sTooltip, m_vColumns.at(iSubItem)->GetTooltip(*m_vmItems, nIndex));
    GSL_SUPPRESS_TYPE3 pnmTtDispInfo.lpszText = const_cast<LPTSTR>(m_sTooltip.c_str());
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

void GridBinding::OnSizeChanged(const ra::ui::Size&)
{
    UpdateLayout();

    ControlBinding::ForceRepaint(m_hWnd);
}

LRESULT GridBinding::OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw)
{
    LRESULT nResult = CDRF_DODEFAULT;

    if (m_pRowColorProperty)
    {
        switch (pCustomDraw->nmcd.dwDrawStage)
        {
            case CDDS_PREPAINT:
                nResult |= CDRF_NOTIFYITEMDRAW;
                break;

            case CDDS_ITEMPREPAINT:
            {
                if (ListView_GetItemState(m_hWnd, pCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED) == 0)
                {
                    const auto nIndex = gsl::narrow_cast<gsl::index>(pCustomDraw->nmcd.dwItemSpec) - m_nScrollOffset;
                    const Color pColor(ra::to_unsigned(m_vmItems->GetItemValue(nIndex, *m_pRowColorProperty)));
                    if (pColor.Channel.A != 0)
                        pCustomDraw->clrTextBk = RGB(pColor.Channel.R, pColor.Channel.G, pColor.Channel.B);
                }
                break;
            }
        }
    }

    if (m_bHasColoredColumns)
    {
        switch (pCustomDraw->nmcd.dwDrawStage)
        {
            case CDDS_PREPAINT:
                nResult |= CDRF_NOTIFYITEMDRAW;
                break;

            case CDDS_ITEMPREPAINT:
                nResult |= CDRF_NOTIFYSUBITEMDRAW;
                break;

            case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                if (pCustomDraw->iSubItem >= 0 && pCustomDraw->iSubItem < gsl::narrow_cast<int>(m_vColumns.size()))
                {
                    const auto& pColumn = *m_vColumns.at(pCustomDraw->iSubItem);
                    if (pColumn.GetTextColorProperty() != nullptr)
                    {
                        const auto nIndex = gsl::narrow_cast<gsl::index>(pCustomDraw->nmcd.dwItemSpec) - m_nScrollOffset;
                        const Color pColor(ra::to_unsigned(m_vmItems->GetItemValue(nIndex, *pColumn.GetTextColorProperty())));
                        if (pColor.Channel.A != 0)
                            pCustomDraw->clrText = RGB(pColor.Channel.R, pColor.Channel.G, pColor.Channel.B);
                    }
                }
                break;
        }
    }

    return nResult;
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

                pInfo->nItemIndex -= m_nScrollOffset;

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

void GridBinding::OnNmRClick(const NMITEMACTIVATE* pnmItemActivate)
{
    // if an in-place editor is open, close it.
    if (m_hInPlaceEditor)
        SendMessage(m_hInPlaceEditor, WM_KILLFOCUS, 0, 0);

    if (ra::to_unsigned(pnmItemActivate->iSubItem) < m_vColumns.size())
    {
        const auto& pColumn = m_vColumns.at(pnmItemActivate->iSubItem);
        if (pColumn->HandleRightClick(GetItems(), gsl::narrow_cast<gsl::index>(pnmItemActivate->iItem) - m_nScrollOffset))
            return;
    }
}

void GridBinding::OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate)
{
    if (ra::to_unsigned(pnmItemActivate->iSubItem) < m_vColumns.size())
    {
        const auto& pColumn = m_vColumns.at(pnmItemActivate->iSubItem);
        if (pColumn->HandleDoubleClick(GetItems(), gsl::narrow_cast<gsl::index>(pnmItemActivate->iItem) - m_nScrollOffset))
            return;
    }

    if (m_pDoubleClickHandler)
        m_pDoubleClickHandler(gsl::narrow_cast<gsl::index>(pnmItemActivate->iItem));
}

void GridBinding::OnLvnKeyDown(const LPNMLVKEYDOWN pnmKeyDown)
{
    // only interested in Ctrl key events
    const bool bControlHeld = (GetKeyState(VK_CONTROL) < 0);
    if (!bControlHeld)
    {
        // if Enter is pushed, treat it as a double-click on the first selected item
        if (pnmKeyDown->wVKey == VK_RETURN && m_pDoubleClickHandler)
        {
            // ignore Alt && Shift
            const bool bAltHeld = (GetKeyState(VK_MENU) < 0);
            const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
            if (bAltHeld || bShiftHeld)
                return;

            const int nFirstSelectedItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
            if (nFirstSelectedItem != -1)
                m_pDoubleClickHandler(gsl::narrow_cast<gsl::index>(nFirstSelectedItem));
        }

        return;
    }

    // ignore Ctrl+Alt
    const bool bAltHeld = (GetKeyState(VK_MENU) < 0);
    if (bAltHeld)
        return;

    // ignore Ctrl+Shift
    const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
    if (bShiftHeld)
        return;

    switch (pnmKeyDown->wVKey)
    {
        case 'A': // Ctrl+A = select all
            if ((GetWindowLong(m_hWnd, GWL_STYLE) & LVS_SINGLESEL) == 0)
                ListView_SetItemState(m_hWnd, -1, LVIS_SELECTED, LVIS_SELECTED);
            break;

        case 'D': // Ctrl+D = deselect all
            ListView_SetItemState(m_hWnd, -1, 0, LVIS_SELECTED);
            break;

        case 'C': // Ctrl+C = copy
            if (m_pCopyHandler)
                m_pCopyHandler();
            break;

        case 'V': // Ctrl+V = paste
            if (m_pPasteHandler)
                m_pPasteHandler();
            break;
    }

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
