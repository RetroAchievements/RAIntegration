#include "LoginViewModel.hh"

#include "Exports.hh"
#include "RA_Interface.h"
#include "RA_StringUtils.h"

#include "api\Login.hh"

#include "data\context\EmulatorContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\IConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

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

    ra::api::Login::Request request;
    request.Username = ra::Narrow(GetUsername());
    request.Password = ra::Narrow(GetPassword());

    const auto response = request.Call();

    if (!response.Succeeded())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to login",
            ra::Widen(response.ErrorMessage));
        return false;
    }

    const bool bRememberLogin = IsPasswordRemembered();
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetUsername(response.Username);
    pConfiguration.SetApiToken(bRememberLogin ? response.ApiToken : "");
    pConfiguration.Save();

    // initialize the user context
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
    pUserContext.Initialize(response.Username, response.DisplayName, response.ApiToken);
    pUserContext.SetScore(response.Score);

    // load the session information
    auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>();
    pSessionTracker.Initialize(response.Username);

    ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(
        std::wstring(L"Successfully logged in as ") + ra::Widen(response.Username));

    ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();
#ifndef RA_UTEST
    _RA_UpdateAppTitle();
#endif

    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
