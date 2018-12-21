#include "LoginDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

namespace ra {
namespace ui {
namespace win32 {

bool LoginDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::LoginViewModel*>(&vmViewModel) != nullptr);
}

void LoginDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto& vmLogin = reinterpret_cast<ra::ui::viewmodels::LoginViewModel&>(vmViewModel);

    LoginDialog oDialog(vmLogin);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_LOGIN), this);
}

void LoginDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel) { ShowModal(oViewModel); }

// ------------------------------------

LoginDialog::LoginDialog(ra::ui::viewmodels::LoginViewModel& vmLogin) :
    DialogBase(vmLogin),
    m_bindUsername(vmLogin),
    m_bindPassword(vmLogin),
    m_bindRememberMe(vmLogin)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);

    m_bindUsername.BindText(ra::ui::viewmodels::LoginViewModel::UsernameProperty);
    m_bindPassword.BindText(ra::ui::viewmodels::LoginViewModel::PasswordProperty);
    m_bindRememberMe.BindCheck(ra::ui::viewmodels::LoginViewModel::IsPasswordRememberedProperty);
}

BOOL LoginDialog::OnInitDialog()
{
    m_bindUsername.SetControl(*this, IDC_RA_USERNAME);
    m_bindPassword.SetControl(*this, IDC_RA_PASSWORD);
    m_bindRememberMe.SetControl(*this, IDC_RA_SAVEPASSWORD);

    return DialogBase::OnInitDialog();
}

BOOL LoginDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDOK)
    {
        const auto& vmLogin = reinterpret_cast<ra::ui::viewmodels::LoginViewModel&>(m_vmWindow);
        if (!vmLogin.Login())
            return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
