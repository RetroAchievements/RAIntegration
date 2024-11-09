#include "UserContext.hh"

#include "Exports.hh"
#include "RA_StringUtils.h"

#include "api\impl\DisconnectedServer.hh"

#include "data\context\EmulatorContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\AchievementRuntimeExports.hh"
#include "services\IConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace data {
namespace context {

void UserContext::Initialize(const std::string& sUsername, const std::string& sDisplayName, const std::string& sApiToken)
{
    m_sUsername = sUsername;
    m_sDisplayName = sDisplayName;
    m_sApiToken = sApiToken;
    m_nScore = 0U;

    RaiseClientExternalMenuChanged();
}

void UserContext::Logout()
{
    if (!IsLoggedIn())
        return;

    _RA_ActivateGame(0U);

    auto* pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();
    rc_client_logout(pClient);

    m_sUsername.clear();
    m_sDisplayName.clear();
    m_sApiToken.clear();

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

} // namespace context
} // namespace data
} // namespace ra
