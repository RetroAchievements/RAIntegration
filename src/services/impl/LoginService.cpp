#include "LoginService.hh"

#include "Exports.hh"
#include "util\Strings.hh"

#include "context\IRcClient.hh"
#include "context\UserContext.hh"

#include "api\impl\DisconnectedServer.hh"

#include "data\context\EmulatorContext.hh"
#include "data\context\SessionTracker.hh"

#include "services\AchievementRuntime.hh"
#include "services\AchievementRuntimeExports.hh"
#include "services\IConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace services {
namespace impl {

bool LoginService::IsLoggedIn() const
{
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
    return !pUserContext.GetApiToken().empty();
}

bool LoginService::Login(const std::string& sUsername, const std::string& sPassword)
{
    ra::services::AchievementRuntime::Synchronizer pSynchronizer;

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    pRuntime.BeginLoginWithPassword(sUsername, sPassword,
        [](int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata) {
            auto* pSynchronizer = static_cast<ra::services::AchievementRuntime::Synchronizer*>(pUserdata);
            Expects(pSynchronizer != nullptr);

            pSynchronizer->CaptureResult(nResult, sErrorMessage);
            pSynchronizer->Notify();
        },
        &pSynchronizer);

    pSynchronizer.Wait();

    if (pSynchronizer.GetResult() != RC_OK)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to login",
            ra::Widen(pSynchronizer.GetErrorMessage()));
        return false;
    }

    return true;
}

void LoginService::Logout()
{
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::context::UserContext>();

    if (pUserContext.GetApiToken().empty())
        return;

    _RA_ActivateGame(0U);

    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    rc_client_logout(pClient);

    pUserContext.Initialize("", "", "");

    ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator.UpdateWindowTitle();
    ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();
    RaiseClientExternalMenuChanged();

    ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"You are now logged out.");

    // update the global IServer instance to the disconnected API
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    auto serverApi = std::make_unique<ra::api::impl::DisconnectedServer>(pConfiguration.GetHostUrl());
    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(serverApi));
}

} // namespace impl
} // namespace services
} // namespace ra
