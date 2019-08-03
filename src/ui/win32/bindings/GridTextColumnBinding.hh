#ifndef RA_UI_WIN32_GRIDTEXTCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDTEXTCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"
#include "ui\ModelProperty.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridTextColumnBinding : public GridColumnBinding
{
public:
    GridTextColumnBinding(const StringModelProperty& pBoundProperty) noexcept
        : m_pBoundProperty(&pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        return vmItems.GetItemValue(nIndex, *m_pBoundProperty);
    }

    bool DependsOn(const ra::ui::StringModelProperty& pProperty) const override
    {
        return pProperty == *m_pBoundProperty;
    }

protected:
    const StringModelProperty* m_pBoundProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDCOLUMNBINDING_H
