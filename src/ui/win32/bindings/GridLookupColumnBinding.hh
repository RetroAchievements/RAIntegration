#ifndef RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridLookupColumnBinding : public GridColumnBinding
{
public:
    GridLookupColumnBinding(const IntModelProperty& pBoundProperty, const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept
        : m_pBoundProperty(&pBoundProperty), m_vmItems(vmItems)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        return m_vmItems.GetLabelForId(nValue);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        return pProperty == *m_pBoundProperty;
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override;
    int GetValueFromIndex(gsl::index nIndex) const;
    const IntModelProperty& GetBoundProperty() const noexcept { return *m_pBoundProperty; }

    typedef bool (*IsVisibleFunction)(const ViewModelBase& vmItem, int nValue);
    void SetVisibilityFilter(IsVisibleFunction fIsVisible) noexcept { m_fIsVisible = fIsVisible; }

    void SetDropDownWidth(int nPixels) noexcept { m_nDropDownWidth = nPixels; }

protected:
    const IntModelProperty* m_pBoundProperty = nullptr;
    const ra::ui::viewmodels::LookupItemViewModelCollection& m_vmItems;

    IsVisibleFunction m_fIsVisible = nullptr;
    std::vector<int> m_vVisibleItems;
    int m_nDropDownWidth = -1;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
