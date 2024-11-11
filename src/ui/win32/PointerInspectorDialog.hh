#ifndef RA_UI_WIN32_DLG_POINTERINSPECTOR_H
#define RA_UI_WIN32_DLG_POINTERINSPECTOR_H
#pragma once

#include "ui/viewmodels/PointerInspectorViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/ComboBoxBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class PointerInspectorDialog : public DialogBase
{
public:
    explicit PointerInspectorDialog(viewmodels::PointerInspectorViewModel& vmPointerInspector);
    virtual ~PointerInspectorDialog() noexcept = default;
    PointerInspectorDialog(const PointerInspectorDialog&) noexcept = delete;
    PointerInspectorDialog& operator=(const PointerInspectorDialog&) noexcept = delete;
    PointerInspectorDialog(PointerInspectorDialog&&) noexcept = delete;
    PointerInspectorDialog& operator=(PointerInspectorDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<PointerInspectorDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    bindings::TextBoxBinding m_bindAddress;
    bindings::ComboBoxBinding m_bindNodes;
    bindings::TextBoxBinding m_bindDescription;
    bindings::GridBinding m_bindFields;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_POINTERINSPECTOR_H
