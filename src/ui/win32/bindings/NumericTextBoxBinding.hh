#ifndef RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
#define RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
#pragma once

#include "TextBoxBinding.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "util\Strings.hh"

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

    void SetRange(int nMinimum, int nMaximum) noexcept
    {
        m_nMinimum = nMinimum;
        m_nMaximum = nMaximum;
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
            InvokeOnUIThread([this, sValue = std::to_wstring(GetValue(*m_pValueBoundProperty))]() noexcept {
                SetWindowTextW(m_hWnd, sValue.c_str());
            });
        }
        else
        {
            TextBoxBinding::UpdateBoundText();
        }
    }

    void UpdateSourceFromText(const std::wstring& sValue) override
    {
        std::wstring sError;
        const int nValue = ParseValue(sValue, sError);
        if (!sError.empty())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(sError);
        }
        else
        {
            SetValue(*m_pValueBoundProperty, nValue);
        }
    }

    int ParseValue(const std::wstring& sValue, std::wstring& sError)
    {
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
                    sError = ra::util::String::Printf(L"Value must be at least %d", m_nMinimum);
            }
            else if (nVal > m_nMaximum)
            {
                sError = ra::util::String::Printf(L"Value cannot exceed %d", m_nMaximum);
            }
            else
            {
                sError.clear();
                return gsl::narrow_cast<int>(nVal);
            }
        }
        catch (const std::invalid_argument&)
        {
            sError = L"Only values that can be represented as decimal are allowed";
        }
        catch (const std::out_of_range&)
        {
            sError = ra::util::String::Printf(L"Value cannot exceed %d", m_nMaximum);
        }

        return 0;
    }

protected:
    const IntModelProperty* m_pValueBoundProperty = nullptr;
    int m_nMinimum = 0;
    int m_nMaximum = 0x7FFFFFFF;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_NUMERICTEXTBOXBINDING_H
