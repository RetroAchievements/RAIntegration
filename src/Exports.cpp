#include "Exports.hh"

#include "RA_BuildVer.h"
#include "RA_PopupWindows.h"
#include "RA_Resource.h"

#include "api\Login.hh"

#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\Http.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

API const char* CCONV _RA_IntegrationVersion() { return RA_INTEGRATION_VERSION; }

API const char* CCONV _RA_HostName()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.GetHostName().c_str();
}

API int CCONV _RA_HardcoreModeIsActive()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
}

static void HandleLoginResponse(const ra::api::Login::Response& response)
{
    if (response.Succeeded())
    {
        // initialize the user context
        auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
        pUserContext.Initialize(response.Username, response.ApiToken);
        pUserContext.SetScore(response.Score);

        // start a new session
        auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>();
        pSessionTracker.Initialize(response.Username);

#ifndef RA_UTEST
        // show the welcome message
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup(ra::StringPrintf("Welcome %s%s (%u)", pSessionTracker.HasSessionData() ? "back " : "",
                                          response.Username.c_str(), response.Score),
                         (response.NumUnreadMessages == 1)
                             ? "You have 1 new message"
                             : ra::StringPrintf("You have %u new messages", response.NumUnreadMessages),
                         PopupMessageType::PopupLogin, ra::ui::ImageType::UserPic, response.Username));

        // notify the client to update the RetroAchievements menu
        RA_RebuildMenu();

        // update the client titlebar to include the user name
        _RA_UpdateAppTitle();

        // notify the overlay of the new user image
        g_AchievementOverlay.UpdateImages();
#endif
    }
    else if (!response.ErrorMessage.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Login Failed", ra::Widen(response.ErrorMessage));
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Login Failed", L"Please login again.");
    }
}

API void CCONV _RA_AttemptLogin(bool bBlocking)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetApiToken().empty() || pConfiguration.GetUsername().empty())
    {
#ifndef RA_UTEST
        // show the login dialog box
        ra::ui::viewmodels::LoginViewModel vmLogin;
        vmLogin.ShowModal();
#endif
    }
    else
    {
        ra::api::Login::Request request;
        request.Username = pConfiguration.GetUsername();
        request.ApiToken = pConfiguration.GetApiToken();

        if (bBlocking)
        {
            ra::api::Login::Response response = request.Call();
            HandleLoginResponse(response);
        }
        else
        {
            request.CallAsync(HandleLoginResponse);
        }
    }
}
