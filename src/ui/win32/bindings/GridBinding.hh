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

    GSL_SUPPRESS_CON3 void OnLvnItemChanged(const LPNMLISTVIEW pnmListView);

    void OnSizeChanged(const ra::ui::Size&) override { UpdateLayout(); }

protected:
    void UpdateLayout();
    void UpdateAllItems();
    void UpdateItems(gsl::index nColumn);

private:
    size_t m_nColumnsCreated = 0;
    std::vector<std::unique_ptr<GridColumnBinding>> m_vColumns;
    ViewModelCollectionBase* m_vmItems = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDBINDING_H
