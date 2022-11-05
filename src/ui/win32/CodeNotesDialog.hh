#ifndef RA_UI_WIN32_DLG_CODENOTES_H
#define RA_UI_WIN32_DLG_CODENOTES_H
#pragma once

#include "data/context/EmulatorContext.hh"

#include "ui/viewmodels/CodeNotesViewModel.hh"

#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/MultiLineGridBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

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
    class CodeNotesGridBinding : public ra::ui::win32::bindings::MultiLineGridBinding,
        protected ra::data::context::EmulatorContext::NotifyTarget
    {
    public:
        explicit CodeNotesGridBinding(ViewModelBase& vmViewModel);
        GSL_SUPPRESS_F6 ~CodeNotesGridBinding();
        CodeNotesGridBinding(const CodeNotesGridBinding&) noexcept = delete;
        CodeNotesGridBinding& operator=(const CodeNotesGridBinding&) noexcept = delete;
        CodeNotesGridBinding(CodeNotesGridBinding&&) noexcept = delete;
        CodeNotesGridBinding& operator=(CodeNotesGridBinding&&) noexcept = delete;

    protected:
        void OnTotalMemorySizeChanged() override;
    };

    CodeNotesGridBinding m_bindNotes;
    ra::ui::win32::bindings::TextBoxBinding m_bindFilterValue;
    ra::ui::win32::bindings::CheckBoxBinding m_bindUnpublished;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_CODENOTES_H
