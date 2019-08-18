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

    GSL_SUPPRESS_CON3 void OnLvnItemChanged(const LPNMLISTVIEW pnmListView);
    void OnNmClick(const NMITEMACTIVATE* pnmItemActivate);

    void OnSizeChanged(const ra::ui::Size&) override { UpdateLayout(); }

protected:
    void UpdateLayout();
    void UpdateAllItems();
    void UpdateItems(gsl::index nColumn);

    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) noexcept override;

private:
    size_t m_nColumnsCreated = 0;
    std::vector<std::unique_ptr<GridColumnBinding>> m_vColumns;
    ViewModelCollectionBase* m_vmItems = nullptr;
    HWND m_hInPlaceEditor = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDBINDING_H
