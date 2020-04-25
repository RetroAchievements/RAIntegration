#ifndef RA_UI_WIN32_DLG_MEMORYBOOKMARKS_H
#define RA_UI_WIN32_DLG_MEMORYBOOKMARKS_H
#pragma once

#include "ui/viewmodels/MemoryBookmarksViewModel.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class MemoryBookmarksDialog : public DialogBase
{
public:
    explicit MemoryBookmarksDialog(ra::ui::viewmodels::MemoryBookmarksViewModel& vmMemoryBookmarks);
    virtual ~MemoryBookmarksDialog() noexcept = default;
    MemoryBookmarksDialog(const MemoryBookmarksDialog&) noexcept = delete;
    MemoryBookmarksDialog& operator=(const MemoryBookmarksDialog&) noexcept = delete;
    MemoryBookmarksDialog(MemoryBookmarksDialog&&) noexcept = delete;
    MemoryBookmarksDialog& operator=(MemoryBookmarksDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<MemoryBookmarksDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::GridBinding m_bindBookmarks;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_MEMORYBOOKMARKS_H
