#include "LoginViewModel.hh"

#include "Exports.hh"
#include "RA_Interface.h"
#include "RA_StringUtils.h"

#include "api\Login.hh"

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

    std::mutex wait_mutex;
    m_bWaiting = true;

    auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::RcheevosClient>();
    pClient.BeginLoginWithPassword(ra::Narrow(GetUsername()), ra::Narrow(GetPassword()),
        [](int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
        {
            const LoginViewModel* vmLogin = reinterpret_cast<const LoginViewModel*>(pUserdata);

            if (nResult != RC_OK)
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to login",
                                                                          ra::Widen(sErrorMessage));
            }
            else
            {
                auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();

                const bool bRememberLogin = vmLogin->IsPasswordRemembered();
                auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
                pConfiguration.SetUsername(pUserContext.GetUsername());
                pConfiguration.SetApiToken(bRememberLogin ? pUserContext.GetApiToken() : "");
                pConfiguration.Save();

                // load the session information
                auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>();
                pSessionTracker.Initialize(pUserContext.GetUsername());

                ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(std::wstring(L"Successfully logged in as ") +
                                                                         ra::Widen(pUserContext.GetDisplayName()));

                ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();
                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator.UpdateWindowTitle();
            }

            vmLogin->m_pCondVar.notify_all();
            vmLogin->m_bWaiting = false;
        },
        reinterpret_cast<void*>(const_cast<LoginViewModel*>(this)));

    if (m_bWaiting)
    {
        std::unique_lock<std::mutex> lock(wait_mutex);
        m_pCondVar.wait(lock);
    }

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    return pUserContext.IsLoggedIn();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
