#include "LoginViewModel.hh"

#include "Exports.hh"
#include "RAInterface\RA_Interface.h"
#include "util\Strings.hh"

#include "data\context\EmulatorContext.hh"
#include "data\context\UserContext.hh"

#include "services\IConfiguration.hh"
#include "services\ILoginService.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty LoginViewModel::UsernameProperty("LoginViewModel", "Username", L"");
const StringModelProperty LoginViewModel::PasswordProperty("LoginViewModel", "Password", L"");
const BoolModelProperty LoginViewModel::IsPasswordRememberedProperty("LoginViewModel", "IsPasswordRemembered", false);

LoginViewModel::LoginViewModel()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    SetUsername(ra::Widen(pConfiguration.GetUsername()));
}

LoginViewModel::LoginViewModel(const std::wstring&& sUsername)
{
    SetUsername(sUsername);
}

bool LoginViewModel::Login() const
{
    if (GetUsername().empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Username is required.");
        return false;
    }

    if (GetPassword().empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Password is required.");
        return false;
    }

    auto& pLoginService = ra::services::ServiceLocator::GetMutable<ra::services::ILoginService>();
    if (!pLoginService.Login(ra::Narrow(GetUsername()), ra::Narrow(GetPassword())))
        return false;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    const bool bRememberLogin = IsPasswordRemembered();
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetUsername(pUserContext.GetDisplayName());
    pConfiguration.SetApiToken(bRememberLogin ? pUserContext.GetApiToken() : "");
    pConfiguration.Save();

    ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(std::wstring(L"Successfully logged in as ") +
                                                             ra::Widen(pUserContext.GetDisplayName()));

    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
