#ifndef RA_UI_WIN32_MESSAGEBOXDIALOG_H
#define RA_UI_WIN32_MESSAGEBOXDIALOG_H
#pragma once

#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class MessageBoxDialog
{
public:
    class Presenter : public IDialogPresenter
    {
    public:
        Presenter() noexcept;

        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel) override;
    };
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_MESSAGEBOXDIALOG_H
