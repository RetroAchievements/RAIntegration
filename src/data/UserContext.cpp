#include "UserContext.hh"

#include "RA_Core.h"

#include "api\Logout.hh"

#include "services\IConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace data {

void UserContext::Logout()
{
    if (!IsLoggedIn())
        return;

    const ra::api::Logout::Request request;
    const auto response = request.Call();
    if (response.Succeeded())
    {
        m_sUsername.clear();
        m_sApiToken.clear();
        m_nScore = 0U;

        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();
        ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();
#ifndef RA_UTEST
        _RA_UpdateAppTitle();
        RA_RebuildMenu();
#endif

        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"You are now logged out.");
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Logout failed", ra::Widen(response.ErrorMessage));
    }
}

} // namespace data
} // namespace ra
