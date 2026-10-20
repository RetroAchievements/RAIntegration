#ifndef RA_UI_WIN32_DLG_STRUCTUREDDATABROWSER_H
#define RA_UI_WIN32_DLG_STRUCTUREDDATABROWSER_H
#pragma once

#include "ui/viewmodels/StructuredDataBrowserViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/ComboBoxBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class StructuredDataBrowserDialog : public DialogBase
{
public:
    explicit StructuredDataBrowserDialog(viewmodels::StructuredDataBrowserViewModel& vmStructuredDataBrowser);
    virtual ~StructuredDataBrowserDialog() noexcept = default;
    StructuredDataBrowserDialog(const StructuredDataBrowserDialog&) noexcept = delete;
    StructuredDataBrowserDialog& operator=(const StructuredDataBrowserDialog&) noexcept = delete;
    StructuredDataBrowserDialog(StructuredDataBrowserDialog&&) noexcept = delete;
    StructuredDataBrowserDialog& operator=(StructuredDataBrowserDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<StructuredDataBrowserDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    bindings::ComboBoxBinding m_bindAddress;
    bindings::ComboBoxBinding m_bindNodes;
    bindings::TextBoxBinding m_bindDescription;
    bindings::GridBinding m_bindPointerChain;
    bindings::GridBinding m_bindFields;
    bindings::TextBoxBinding m_bindFieldNote;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_STRUCTUREDDATABROWSER_H
