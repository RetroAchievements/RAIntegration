#ifndef RA_UI_WIN32_DLG_CODENOTES_H
#define RA_UI_WIN32_DLG_CODENOTES_H
#pragma once

#include "ui/viewmodels/CodeNotesViewModel.hh"
#include "ui/win32/bindings/MultiLineGridBinding.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class CodeNotesDialog : public DialogBase
{
public:
    explicit CodeNotesDialog(ra::ui::viewmodels::CodeNotesViewModel& vmMemoryBookmarks);
    virtual ~CodeNotesDialog() noexcept = default;
    CodeNotesDialog(const CodeNotesDialog&) noexcept = delete;
    CodeNotesDialog& operator=(const CodeNotesDialog&) noexcept = delete;
    CodeNotesDialog(CodeNotesDialog&&) noexcept = delete;
    CodeNotesDialog& operator=(CodeNotesDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<CodeNotesDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;
    
private:
    ra::ui::win32::bindings::MultiLineGridBinding m_bindNotes;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_CODENOTES_H
