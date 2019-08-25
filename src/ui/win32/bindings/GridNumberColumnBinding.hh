#ifndef RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"
#include "ui\ModelProperty.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridNumberColumnBinding : public GridColumnBinding
{
public:
    GridNumberColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : m_pBoundProperty(&pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        return std::to_wstring(vmItems.GetItemValue(nIndex, *m_pBoundProperty));
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        return pProperty == *m_pBoundProperty;
    }

protected:
    const IntModelProperty* m_pBoundProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
