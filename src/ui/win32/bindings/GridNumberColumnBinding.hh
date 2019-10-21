#ifndef RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
#pragma once

#include "GridTextColumnBinding.hh"
#include "ui\ModelProperty.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridNumberColumnBinding : public GridTextColumnBindingBase
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


    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override;

    std::wstring GetSortKey(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        std::wstring sText = GetText(vmItems, nIndex);
        sText.insert(0, 10 - sText.length(), '0');
        return sText;
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        return pProperty == *m_pBoundProperty;
    }

    unsigned int Maximum() const noexcept { return m_nMaximum; }
    void SetMaximum(unsigned int nValue) noexcept { m_nMaximum = nValue; }

    const IntModelProperty& GetBoundProperty() const noexcept { return *m_pBoundProperty; }

protected:
    bool ParseUnsignedInt(const std::wstring& sValue, _Out_ unsigned int& nValue, _Out_ std::wstring& sError);
    bool ParseHex(const std::wstring& sValue, _Out_ unsigned int& nValue, _Out_ std::wstring& sError);

    const IntModelProperty* m_pBoundProperty = nullptr;
    unsigned int m_nMaximum = 0xFFFFFFFF;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDNUMBERCOLUMNBINDING_H
