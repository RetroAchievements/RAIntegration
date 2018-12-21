#ifndef RA_UI_WIN32_DLG_LOGIN_H
#define RA_UI_WIN32_DLG_LOGIN_H
#pragma once

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\win32\DialogBase.hh"
#include "ui\win32\IDialogPresenter.hh"
#include "ui\win32\bindings\CheckBoxBinding.hh"
#include "ui\win32\bindings\TextBoxBinding.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class LoginDialog : public DialogBase
{
public:
    explicit LoginDialog(ra::ui::viewmodels::LoginViewModel& vmLogin);
    virtual ~LoginDialog() noexcept = default;
    LoginDialog(const LoginDialog&) noexcept = delete;
    LoginDialog& operator=(const LoginDialog&) noexcept = delete;
    LoginDialog(LoginDialog&&) noexcept = delete;
    LoginDialog& operator=(LoginDialog&&) noexcept = delete;

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
    ra::ui::win32::bindings::TextBoxBinding m_bindUsername;
    ra::ui::win32::bindings::TextBoxBinding m_bindPassword;
    ra::ui::win32::bindings::CheckBoxBinding m_bindRememberMe;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_LOGIN_H
