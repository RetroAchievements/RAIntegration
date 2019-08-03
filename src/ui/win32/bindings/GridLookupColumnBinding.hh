#ifndef RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"
#include "ui/ModelProperty.hh"

#include "ui/viewmodels/LookupItemViewModel.hh"

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

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const override
    {
        return pProperty == *m_pBoundProperty;
    }

    HWND CreateInPlaceEditor(HWND hParent, std::unique_ptr<InPlaceEditorInfo> pInfo) override;
    int GetValueFromIndex(gsl::index nIndex) const;
    const IntModelProperty& GetBoundProperty() const { return *m_pBoundProperty; }

protected:
    const IntModelProperty* m_pBoundProperty = nullptr;
    const ra::ui::viewmodels::LookupItemViewModelCollection& m_vmItems;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDLOOKUPCOLUMNBINDING_H
