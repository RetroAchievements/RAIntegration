#ifndef RA_UI_WIN32_DLG_PROGRESS_H
#define RA_UI_WIN32_DLG_PROGRESS_H
#pragma once

#include "ui/viewmodels/ProgressViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include "ui/win32/bindings/ProgressBarBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class ProgressDialog : public DialogBase
{
public:
    explicit ProgressDialog(_Inout_ ra::ui::viewmodels::ProgressViewModel& vmProgress);
    ~ProgressDialog() noexcept = default;

    ProgressDialog(const ProgressDialog&) noexcept = delete;
    ProgressDialog& operator=(const ProgressDialog&) noexcept = delete;
    ProgressDialog(ProgressDialog&&) noexcept = delete;
    ProgressDialog& operator=(ProgressDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<ProgressDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::ProgressBarBinding m_bindProgress;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_PROGRESS_H
