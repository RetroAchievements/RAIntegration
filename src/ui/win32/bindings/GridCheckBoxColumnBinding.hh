#ifndef RA_UI_WIN32_GRIDCHECKBOXCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDCHECKBOXCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"
#include "ui\ModelProperty.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridCheckBoxColumnBinding : public GridColumnBinding
{
public:
    GridCheckBoxColumnBinding(const BoolModelProperty& pBoundProperty) noexcept
        : m_pBoundProperty(&pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase&, gsl::index) const override
    {
        return L"";
    }

    std::wstring GetSortKey(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        return vmItems.GetItemValue(nIndex, *m_pBoundProperty) ? L"Checked" : L"Unchecked";
    }

    bool DependsOn(const ra::ui::BoolModelProperty& pProperty) const noexcept override
    {
        return pProperty == *m_pBoundProperty;
    }

    const BoolModelProperty& GetBoundProperty() const noexcept { return *m_pBoundProperty; }

protected:
    const BoolModelProperty* m_pBoundProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDCHECKBOXCOLUMNBINDING_H
