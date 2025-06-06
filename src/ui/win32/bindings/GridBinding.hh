#ifndef RA_UI_WIN32_GRIDBINDING_H
#define RA_UI_WIN32_GRIDBINDING_H
#pragma once

#include "ControlBinding.hh"
#include "GridColumnBinding.hh"

#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridBinding : public ControlBinding, protected ViewModelCollectionBase::NotifyTarget
{
public:
    explicit GridBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}
    GSL_SUPPRESS_F6 ~GridBinding() noexcept;

    GridBinding(const GridBinding&) noexcept = delete;
    GridBinding& operator=(const GridBinding&) noexcept = delete;
    GridBinding(GridBinding&&) noexcept = delete;
    GridBinding& operator=(GridBinding&&) noexcept = delete;

    void SetHWND(DialogBase& pDialog, HWND hControl) override;

    void BindColumn(gsl::index nColumn, std::unique_ptr<GridColumnBinding> pColumnBinding);
    void RefreshColumn(gsl::index nColumn);

    void BindItems(ViewModelCollectionBase& vmItems);
    ViewModelCollectionBase& GetItems() noexcept { return *m_vmItems; }

    void Virtualize(const IntModelProperty& pScrollOffsetProperty, const IntModelProperty& pScrollMaximumProperty,
        std::function<void(gsl::index, gsl::index, bool)> pUpdateSelectedItems);

    void BindIsSelected(const BoolModelProperty& pIsSelectedProperty);
    void BindEnsureVisible(const IntModelProperty& pEnsureVisibleProperty) noexcept;
    void BindRowColor(const IntModelProperty& pRowColorProperty) noexcept;

    void SetDoubleClickHandler(std::function<void(gsl::index)> pHandler);
    void SetCopyHandler(std::function<void()> pHandler);
    void SetPasteHandler(std::function<void()> pHandler);

    void InitializeTooltips(std::chrono::milliseconds nDisplayTime);

    GSL_SUPPRESS_CON3 virtual LRESULT OnLvnItemChanging(const LPNMLISTVIEW pnmListView);
    GSL_SUPPRESS_CON3 virtual void OnLvnItemChanged(const LPNMLISTVIEW pnmListView);
    GSL_SUPPRESS_CON3 void OnLvnOwnerDrawStateChanged(const LPNMLVODSTATECHANGE pnmStateChanged);
    GSL_SUPPRESS_CON3 void OnLvnColumnClick(const LPNMLISTVIEW pnmListView);
    GSL_SUPPRESS_CON3 void OnLvnKeyDown(const LPNMLVKEYDOWN pnmKeyDown);
    void OnLvnGetDispInfo(NMLVDISPINFO& pnmDispInfo);
    void OnTooltipGetDispInfo(NMTTDISPINFO& pnmTtDispInfo);
    virtual void OnNmClick(const NMITEMACTIVATE* pnmItemActivate);
    void OpenIPE(
        std::unique_ptr<ra::ui::win32::bindings::GridColumnBinding::InPlaceEditorInfo,
                        std::default_delete<ra::ui::win32::bindings::GridColumnBinding::InPlaceEditorInfo>>& pInfo,
        ra::ui::win32::bindings::GridColumnBinding& pColumn);
    virtual void OnNmRClick(const NMITEMACTIVATE* pnmItemActivate);
    virtual void OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate);
    LRESULT OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw) override;

    void OnGotFocus() override;
    void OnLostFocus() noexcept override;

    void OnShown() override { CheckForScrollBar();  UpdateLayout(); }
    void OnSizeChanged(const ra::ui::Size&) override;

    bool GetShowGridLines() const noexcept { return m_bShowGridLines; }
    void SetShowGridLines(bool bValue) noexcept { m_bShowGridLines = bValue; }

    static LRESULT CloseIPE(HWND hwnd, GridColumnBinding::InPlaceEditorInfo* pInfo);
    static GridColumnBinding::InPlaceEditorInfo* GetIPEInfo(HWND hwnd) noexcept;

    void DeselectAll() noexcept;
    int UpdateSelected(const IntModelProperty& pProperty, int nNewValue);

    virtual void EnsureVisible(gsl::index nIndex) noexcept(false);

protected:
    void SuspendRedraw();
    void UpdateLayout();
    virtual void UpdateAllItems();
    virtual void UpdateItems(gsl::index nColumn);
    virtual void UpdateCell(gsl::index nIndex, gsl::index nColumnIndex);
    void CheckForScrollBar();
    int GetVisibleItemIndex(int iItem);
    gsl::index GetRealItemIndex(gsl::index iItem) const;

    INT_PTR CALLBACK WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;
    using ViewModelBase::NotifyTarget::OnViewModelBoolValueChanged;
    using ViewModelBase::NotifyTarget::OnViewModelStringValueChanged;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) override;
    void OnViewModelChanged(gsl::index nIndex) override;
    void OnBeginViewModelCollectionUpdate() noexcept override;
    void OnEndViewModelCollectionUpdate() override;

    std::vector<std::unique_ptr<GridColumnBinding>> m_vColumns;
    std::vector<int> m_vColumnWidths;
    ViewModelCollectionBase* m_vmItems = nullptr;
    const BoolModelProperty* m_pIsSelectedProperty = nullptr;
    const IntModelProperty* m_pEnsureVisibleProperty = nullptr;

    int m_nScrollOffset = 0;

private:
    void UpdateRow(gsl::index nIndex, bool bExisting);
    void UpdateSelectedItemStates();

    bool m_bShowGridLines = false;
    bool m_bHasScrollbar = false;
    bool m_bForceRepaint = false;
    bool m_bForceRepaintItems = false;
    bool m_bRedrawSuspended = false;
    bool m_bUpdateSelectedItemStates = false;
    int m_nPendingUpdates = 0;
    int m_nAdjustingScrollOffset = 0;

    size_t m_nColumnsCreated = 0;
    bool m_bHasColoredColumns = false;

    HWND m_hTooltip = nullptr;
    int m_nTooltipLocation = 0;
    ra::tstring m_sTooltip;

    const IntModelProperty* m_pRowColorProperty = nullptr;

    const IntModelProperty* m_pScrollOffsetProperty = nullptr;
    const IntModelProperty* m_pScrollMaximumProperty = nullptr;
    ra::tstring m_sDispInfo;
    std::function<void(gsl::index, gsl::index, bool)> m_pUpdateSelectedItems = nullptr;

    HWND m_hInPlaceEditor = nullptr;

    std::function<void(gsl::index)> m_pDoubleClickHandler = nullptr;
    std::function<void()> m_pCopyHandler = nullptr;
    std::function<void()> m_pPasteHandler = nullptr;

    gsl::index m_nSortIndex = -1;

    std::chrono::steady_clock::time_point m_tFocusTime{};
    std::chrono::steady_clock::time_point m_tIPECloseTime{};
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDBINDING_H
