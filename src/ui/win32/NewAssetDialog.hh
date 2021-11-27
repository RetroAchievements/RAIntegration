#ifndef RA_UI_WIN32_DLG_NEWASSET_H
#define RA_UI_WIN32_DLG_NEWASSET_H
#pragma once

#include "ui/viewmodels/NewAssetViewModel.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class NewAssetDialog : public DialogBase
{
public:
    explicit NewAssetDialog(ra::ui::viewmodels::NewAssetViewModel& vmNewAsset) noexcept;
    virtual ~NewAssetDialog() noexcept = default;
    NewAssetDialog(const NewAssetDialog&) noexcept = delete;
    NewAssetDialog& operator=(const NewAssetDialog&) noexcept = delete;
    NewAssetDialog(NewAssetDialog&&) noexcept = delete;
    NewAssetDialog& operator=(NewAssetDialog&&) noexcept = delete;

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
    };

protected:
    BOOL OnCommand(WORD nCommand) override;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_NEWASSET_H
