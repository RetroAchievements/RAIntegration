#ifndef RA_UI_WIN32_DLG_MEMORYNOTES_H
#define RA_UI_WIN32_DLG_MEMORYNOTES_H
#pragma once

#include "context/IEmulatorMemoryContext.hh"

#include "ui/viewmodels/MemoryNotesViewModel.hh"

#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/MultiLineGridBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class MemoryNotesDialog : public DialogBase
{
public:
    explicit MemoryNotesDialog(ra::ui::viewmodels::MemoryNotesViewModel& vmMemoryBookmarks);
    virtual ~MemoryNotesDialog() noexcept = default;
    MemoryNotesDialog(const MemoryNotesDialog&) noexcept = delete;
    MemoryNotesDialog& operator=(const MemoryNotesDialog&) noexcept = delete;
    MemoryNotesDialog(MemoryNotesDialog&&) noexcept = delete;
    MemoryNotesDialog& operator=(MemoryNotesDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<MemoryNotesDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    class MemoryNotesGridBinding : public ra::ui::win32::bindings::MultiLineGridBinding,
        protected ra::context::IEmulatorMemoryContext::NotifyTarget
    {
    public:
        explicit MemoryNotesGridBinding(ViewModelBase& vmViewModel);
        GSL_SUPPRESS_F6 ~MemoryNotesGridBinding();
        MemoryNotesGridBinding(const MemoryNotesGridBinding&) noexcept = delete;
        MemoryNotesGridBinding& operator=(const MemoryNotesGridBinding&) noexcept = delete;
        MemoryNotesGridBinding(MemoryNotesGridBinding&&) noexcept = delete;
        MemoryNotesGridBinding& operator=(MemoryNotesGridBinding&&) noexcept = delete;

    protected:
        void OnTotalMemorySizeChanged() override;
    };

    MemoryNotesGridBinding m_bindNotes;
    ra::ui::win32::bindings::TextBoxBinding m_bindFilterValue;
    ra::ui::win32::bindings::CheckBoxBinding m_bindUnpublished;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_MEMORYNOTES_H
