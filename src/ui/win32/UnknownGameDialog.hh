#ifndef RA_UI_WIN32_DLG_UNKNOWN_GAME_H
#define RA_UI_WIN32_DLG_UNKNOWN_GAME_H
#pragma once

#include "ui\viewmodels\UnknownGameViewModel.hh"
#include "ui\win32\DialogBase.hh"
#include "ui\win32\IDialogPresenter.hh"
#include "ui\win32\bindings\ComboBoxBinding.hh"
#include "ui\win32\bindings\TextBoxBinding.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class UnknownGameDialog : public DialogBase
{
public:
    explicit UnknownGameDialog(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame);
    virtual ~UnknownGameDialog() noexcept = default;
    UnknownGameDialog(const UnknownGameDialog&) noexcept = delete;
    UnknownGameDialog& operator=(const UnknownGameDialog&) noexcept = delete;
    UnknownGameDialog(UnknownGameDialog&&) noexcept = delete;
    UnknownGameDialog& operator=(UnknownGameDialog&&) noexcept = delete;

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel) override;
    };

protected:
    BOOL OnInitDialog() override;

    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::ComboBoxBinding m_bindExistingTitle;
    ra::ui::win32::bindings::TextBoxBinding m_bindNewTitle;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_UNKNOWN_GAME_H
