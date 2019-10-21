#ifndef RA_UI_WIN32_FILEDIALOG_H
#define RA_UI_WIN32_FILEDIALOG_H
#pragma once

#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class FileDialog
{
public:
    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;

        static void DoShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd);
    };
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_FILEDIALOG_H
