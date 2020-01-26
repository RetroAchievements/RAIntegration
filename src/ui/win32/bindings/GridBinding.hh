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

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        if (!m_vColumns.empty())
        {
            UpdateLayout();

            if (m_vmItems)
                UpdateAllItems();
        }
    }

    void BindColumn(gsl::index nColumn, std::unique_ptr<GridColumnBinding> pColumnBinding);

    void BindItems(ViewModelCollectionBase& vmItems);
    ViewModelCollectionBase& GetItems() noexcept { return *m_vmItems; }

    void Virtualize(const IntModelProperty& pScrollOffsetProperty, const IntModelProperty& pScrollMaximumProperty,
        std::function<void(gsl::index, gsl::index, bool)> pUpdateSelectedItems);

    void BindIsSelected(const BoolModelProperty& pIsSelectedProperty) noexcept;
    void BindRowColor(const IntModelProperty& pRowColorProperty) noexcept;
   
    GSL_SUPPRESS_CON3 LRESULT OnLvnItemChanging(const LPNMLISTVIEW pnmListView);
    GSL_SUPPRESS_CON3 void OnLvnItemChanged(const LPNMLISTVIEW pnmListView);
    GSL_SUPPRESS_CON3 void OnLvnOwnerDrawStateChanged(const LPNMLVODSTATECHANGE pnmStateChanged);
    GSL_SUPPRESS_CON3 void OnLvnColumnClick(const LPNMLISTVIEW pnmListView);
    void OnLvnGetDispInfo(NMLVDISPINFO& pnmDispInfo);
    void OnNmClick(const NMITEMACTIVATE* pnmItemActivate);
    void OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate);
    LRESULT OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw) override;

    void OnGotFocus() override;
    void OnLostFocus() noexcept override;

    void OnShown() override { CheckForScrollBar();  UpdateLayout(); }
    void OnSizeChanged(const ra::ui::Size&) override { UpdateLayout(); }

    bool GetShowGridLines() const noexcept { return m_bShowGridLines; }
    void SetShowGridLines(bool bValue) noexcept { m_bShowGridLines = bValue; }

    static LRESULT CloseIPE(HWND hwnd, GridColumnBinding::InPlaceEditorInfo* pInfo);
    static GridColumnBinding::InPlaceEditorInfo* GetIPEInfo(HWND hwnd) noexcept;

    int UpdateSelected(const IntModelProperty& pProperty, int nNewValue);

protected:
    void UpdateLayout();
    void UpdateAllItems();
    void UpdateItems(gsl::index nColumn);
    void CheckForScrollBar();
    void UpdateScroll();

    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) override;
    void OnViewModelChanged(gsl::index nIndex) override;
    void OnBeginViewModelCollectionUpdate() noexcept override;
    void OnEndViewModelCollectionUpdate() override;

private:
    void UpdateRow(gsl::index nIndex, bool bExisting);

    bool m_bShowGridLines = false;
    bool m_bHasScrollbar = false;

    size_t m_nColumnsCreated = 0;
    std::vector<std::unique_ptr<GridColumnBinding>> m_vColumns;
    bool m_bHasColoredColumns = false;

    ViewModelCollectionBase* m_vmItems = nullptr;

    const BoolModelProperty* m_pIsSelectedProperty = nullptr;
    const IntModelProperty* m_pRowColorProperty = nullptr;

    const IntModelProperty* m_pScrollOffsetProperty = nullptr;
    const IntModelProperty* m_pScrollMaximumProperty = nullptr;
    int m_nScrollOffset = 0;
    std::string m_sDispInfo;
    std::function<void(gsl::index, gsl::index, bool)> m_pUpdateSelectedItems = nullptr;

    HWND m_hInPlaceEditor = nullptr;

    gsl::index m_nSortIndex = -1;

    std::chrono::steady_clock::time_point m_tFocusTime{};
    std::chrono::steady_clock::time_point m_tIPECloseTime{};
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDBINDING_H
