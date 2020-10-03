#ifndef RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
#define RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
#pragma once

#include "TextBoxBinding.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class NumericTextBoxBinding : public TextBoxBinding
{
public:
    explicit NumericTextBoxBinding(ViewModelBase& vmViewModel) noexcept : TextBoxBinding(vmViewModel) {}

    void BindValue(const IntModelProperty& pSourceProperty, UpdateMode nUpdateMode = UpdateMode::LostFocus)
    {
        m_pValueBoundProperty = &pSourceProperty;
        m_pTextUpdateMode = nUpdateMode;

        UpdateBoundText();
    }

protected:
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
    {
        if (m_pValueBoundProperty && *m_pValueBoundProperty == args.Property)
            UpdateBoundText();
    }

    void UpdateBoundText() override
    {
        if (m_hWnd && m_pValueBoundProperty)
        {
            std::wstring sValue = std::to_wstring(GetValue(*m_pValueBoundProperty));
            SetWindowTextW(m_hWnd, sValue.c_str());
        }
    }

    void UpdateSourceFromText(const std::wstring& sValue) override
    {
        std::wstring sError;

        try
        {
            wchar_t* pEnd = nullptr;
            const auto nVal = std::wcstoll(sValue.c_str(), &pEnd, 10);
            if (pEnd && *pEnd != '\0')
            {
                sError = L"Only values that can be represented as decimal are allowed";
            }
            else if (nVal < m_nMinimum)
            {
                if (m_nMinimum == 0)
                    sError = L"Negative values are not allowed";
                else
                    sError = ra::StringPrintf(L"Value must be at least %d", m_nMinimum);
            }
            else if (nVal > m_nMaximum)
            {
                sError = ra::StringPrintf(L"Value cannot exceed %d", m_nMaximum);
            }
            else
            {
                const int nValue = gsl::narrow_cast<unsigned int>(ra::to_unsigned(nVal));
                SetValue(*m_pValueBoundProperty, nValue);
            }
        }
        catch (const std::invalid_argument&)
        {
            sError = L"Only values that can be represented as decimal are allowed";
        }
        catch (const std::out_of_range&)
        {
            sError = ra::StringPrintf(L"Value cannot exceed %d", m_nMaximum);
        }

        if (!sError.empty())
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(sError);
    }

private:
    const IntModelProperty* m_pValueBoundProperty = nullptr;
    int m_nMinimum = 0;
    int m_nMaximum = 0x7FFFFFFF;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
