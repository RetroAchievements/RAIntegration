#ifndef RA_UI_WIN32_NUMERICUPDOWNBINDING_H
#define RA_UI_WIN32_NUMERICUPDOWNBINDING_H
#pragma once

#include "NumericTextBoxBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class NumericUpDownBinding : public NumericTextBoxBinding
{
public:
    explicit NumericUpDownBinding(ViewModelBase& vmViewModel) noexcept 
        : NumericTextBoxBinding(vmViewModel) 
    {
        BindKey(VK_UP, [this]() 
        { 
            Adjust(1); return true; 
        });

        BindKey(VK_DOWN, [this]() 
        { 
            Adjust(-1); return true; 
        });
    }

    void SetWrapAround(bool bValue) { m_bWrapAround = bValue; }

    void SetSpinnerControl(DialogBase& pDialog, int nControlResourceId)
    {
        SetSpinnerHWND(GetDlgItem(pDialog.GetHWND(), nControlResourceId));
    }

    void SetSpinnerHWND(HWND hControl)
    {
        m_hWndSpinner = hControl;
        ControlBinding::AddSecondaryControlBinding(hControl);
    }

    void OnUdnDeltaPos(const NMUPDOWN& lpUpDown)
    {
        Adjust(-lpUpDown.iDelta); // up returns negative data
    }

protected:
    void Adjust(int nAmount)
    {
        std::wstring sBuffer;
        GetText(sBuffer);

        wchar_t* pEnd;
        int nVal = std::wcstol(sBuffer.c_str(), &pEnd, 10);
        nVal += nAmount;

        if (nVal < m_nMinimum)
            nVal = m_bWrapAround ? m_nMaximum : m_nMinimum;
        else if (nVal > m_nMaximum)
            nVal = m_bWrapAround ? m_nMinimum : m_nMaximum;

        if (m_pValueBoundProperty)
        {
            SetValue(*m_pValueBoundProperty, nVal);
        }
        else
        {
            std::wstring sValue = std::to_wstring(nVal);
            SetWindowTextW(m_hWnd, sValue.c_str());
            UpdateSourceFromText(sValue);
        }
    }

    HWND m_hWndSpinner = nullptr;
    bool m_bWrapAround = false;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_NUMERICUPDOWNBINDING_H
