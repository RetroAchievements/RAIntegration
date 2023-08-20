#include "LoginViewModel.hh"

#include "Exports.hh"
#include "RA_Interface.h"
#include "RA_StringUtils.h"

#include "api\impl\ConnectedServer.hh"

#include "data\context\EmulatorContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\IConfiguration.hh"
#include "services\RcheevosClient.hh"

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

    ra::services::RcheevosClient::Synchronizer pSynchronizer;

    auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
    pClient.BeginLoginWithPassword(
        ra::Narrow(GetUsername()), ra::Narrow(GetPassword()),
        [](int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata) {
            auto* pSynchronizer = static_cast<ra::services::RcheevosClient::Synchronizer*>(pUserdata);
            Expects(pSynchronizer != nullptr);

            if (nResult != RC_OK)
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to login", ra::Widen(sErrorMessage));
            }

            pSynchronizer->Notify();
        },
        &pSynchronizer);

    pSynchronizer.Wait();

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return false;

    const bool bRememberLogin = IsPasswordRemembered();
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetUsername(pUserContext.GetUsername());
    pConfiguration.SetApiToken(bRememberLogin ? pUserContext.GetApiToken() : "");
    pConfiguration.Save();

    ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(std::wstring(L"Successfully logged in as ") +
                                                             ra::Widen(pUserContext.GetDisplayName()));

    PostLoginInitialization();

    return true;
}

void LoginViewModel::PostLoginInitialization()
{
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();

    // load the session information
    auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>();
    pSessionTracker.Initialize(pUserContext.GetUsername());

    // notify the client to update the RetroAchievements menu
    ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();

    // update the client title-bar to include the user name
    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator.UpdateWindowTitle();

    // update the global IServer instance to the connected API
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    auto serverApi = std::make_unique<ra::api::impl::ConnectedServer>(pConfiguration.GetHostUrl());
    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(serverApi));
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
