#ifndef RA_UI_WIN32_TEXTBOXBINDING_H
#define RA_UI_WIN32_TEXTBOXBINDING_H
#pragma once

#include "ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class TextBoxBinding : public ControlBinding
{
public:
    explicit TextBoxBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        UpdateBoundText();

        if (!m_mKeyHandlers.empty())
            SubclassWndProc();
    }

    enum class UpdateMode
    {
        None = 0,  // one way from source
        LostFocus, // update source when control loses focus
        KeyPress,  // update source after each key press
    };

    void BindText(const StringModelProperty& pSourceProperty, UpdateMode nUpdateMode = UpdateMode::LostFocus)
    {
        m_pTextBoundProperty = &pSourceProperty;
        m_pTextUpdateMode = nUpdateMode;

        UpdateBoundText();
    }

    void OnLostFocus() override
    {
        if (m_pTextUpdateMode == UpdateMode::LostFocus)
            UpdateSource();
    }

    void OnValueChanged() override
    {
        if (m_pTextUpdateMode == UpdateMode::KeyPress)
            UpdateSource();
    }

    void BindKey(unsigned int nKey, std::function<bool()> pHandler)
    {
        if (m_mKeyHandlers.empty() && m_hWnd)
            SubclassWndProc();

        m_mKeyHandlers.insert_or_assign(nKey, pHandler);
    }

    void UpdateSource()
    {
        const int nLength = GetWindowTextLengthW(m_hWnd);

        std::wstring sBuffer;
        sBuffer.resize(gsl::narrow_cast<size_t>(nLength) + 1);
        GetWindowTextW(m_hWnd, sBuffer.data(), gsl::narrow_cast<int>(sBuffer.capacity()));
        sBuffer.resize(nLength);

        UpdateSourceFromText(sBuffer);
    }

protected:
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
    {
        if (m_pTextBoundProperty && *m_pTextBoundProperty == args.Property)
            UpdateBoundText();
    }

    INT_PTR CALLBACK WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            case WM_KEYDOWN:
            {
                const auto iter = m_mKeyHandlers.find(static_cast<unsigned int>(wParam));
                if (iter != m_mKeyHandlers.end())
                {
                    if (iter->second())
                        return 0;
                }
                break;
            }
        }

        return ControlBinding::WndProc(hControl, uMsg, wParam, lParam);
    }

    virtual void UpdateBoundText()
    {
        if (m_hWnd && m_pTextBoundProperty)
            UpdateTextFromSource(GetValue(*m_pTextBoundProperty));
    }

    virtual void UpdateTextFromSource(const std::wstring& sText)
    {
        SetWindowTextW(m_hWnd, sText.c_str());
    }

    virtual void UpdateSourceFromText(const std::wstring& sText)
    {
        SetValue(*m_pTextBoundProperty, sText);
    }

    UpdateMode m_pTextUpdateMode = UpdateMode::None;

private:
    const StringModelProperty* m_pTextBoundProperty = nullptr;
    std::map<unsigned int, std::function<bool()>> m_mKeyHandlers;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_TEXTBOXBINDING_H
