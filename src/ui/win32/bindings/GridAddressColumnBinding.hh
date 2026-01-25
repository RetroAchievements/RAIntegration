#ifndef RA_UI_WIN32_GRIDADDRESSCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDADDRESSCOLUMNBINDING_H
#pragma once

#include "GridColumnBinding.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridAddressColumnBinding : public GridColumnBinding
{
public:
    GridAddressColumnBinding(const IntModelProperty& pBoundProperty)
        : m_pBoundProperty(&pBoundProperty)
    {
        UpdateWidth();
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        return pMemoryContext.FormatAddress(nValue);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        return pProperty == *m_pBoundProperty;
    }

    bool HandleDoubleClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) override;

    void UpdateWidth();
    static unsigned CalculateWidth();

protected:
    const IntModelProperty* m_pBoundProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDADDRESSCOLUMNBINDING_H
