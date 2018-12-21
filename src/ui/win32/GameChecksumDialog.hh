#ifndef RA_UI_WIN32_DLG_GAMECHECKSUM_H
#define RA_UI_WIN32_DLG_GAMECHECKSUM_H
#pragma once

#include "ui/viewmodels/GameChecksumViewModel.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class GameChecksumDialog : public DialogBase
{
public:
    explicit GameChecksumDialog(ra::ui::viewmodels::GameChecksumViewModel& vmGameChecksum);
    virtual ~GameChecksumDialog() noexcept = default;
    GameChecksumDialog(const GameChecksumDialog&) noexcept = delete;
    GameChecksumDialog& operator=(const GameChecksumDialog&) noexcept = delete;
    GameChecksumDialog(GameChecksumDialog&&) noexcept = delete;
    GameChecksumDialog& operator=(GameChecksumDialog&&) noexcept = delete;

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel) override;
    };

protected:
    BOOL OnCommand(WORD nCommand) override;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_GAMECHECKSUM_H
