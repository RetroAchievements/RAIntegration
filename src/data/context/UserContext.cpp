#include "UserContext.hh"

#include "Exports.hh"
#include "RA_StringUtils.h"

#include "api\impl\DisconnectedServer.hh"

#include "data\context\EmulatorContext.hh"

#include "services\IConfiguration.hh"
#include "services\RcheevosClient.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace data {
namespace context {

void UserContext::Logout()
{
    if (!IsLoggedIn())
        return;

    auto* pClient = ra::services::ServiceLocator::Get<ra::services::RcheevosClient>().GetClient();
    rc_client_logout(pClient);

    m_sUsername.clear();
    m_sDisplayName.clear();
    m_sApiToken.clear();

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.ClearPopups();
    pOverlayManager.HideOverlay();

    ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator.UpdateWindowTitle();
    ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();

    ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"You are now logged out.");

    // update the global IServer instance to the connected API
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    auto serverApi = std::make_unique<ra::api::impl::DisconnectedServer>(pConfiguration.GetHostUrl());
    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(serverApi));
}

} // namespace context
} // namespace data
} // namespace ra
