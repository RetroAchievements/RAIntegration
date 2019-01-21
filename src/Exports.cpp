#include "Exports.hh"

#include "RA_AchievementOverlay.h"
#include "RA_BuildVer.h"
#include "RA_Resource.h"

#include "api\Login.hh"

#include "data\EmulatorContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\Http.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

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
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
    if (pUserContext.IsLoginDisabled())
        return;

    if (response.Succeeded())
    {
        // initialize the user context
        pUserContext.Initialize(response.Username, response.ApiToken);
        pUserContext.SetScore(response.Score);

        // start a new session
        auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>();
        pSessionTracker.Initialize(response.Username);

        // show the welcome message
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\login.wav");

        ra::ui::viewmodels::PopupMessageViewModel message;
        message.SetTitle(ra::StringPrintf(L"Welcome %s%s (%u)", pSessionTracker.HasSessionData() ? L"back " : L"",
            response.Username.c_str(), response.Score));
        message.SetDescription((response.NumUnreadMessages == 1)
            ? L"You have 1 new message"
            : ra::StringPrintf(L"You have %u new messages", response.NumUnreadMessages));
        message.SetImage(ra::ui::ImageType::UserPic, response.Username);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(message));

#ifndef RA_UTEST
        // notify the client to update the RetroAchievements menu
        RA_RebuildMenu();

        // update the client title-bar to include the user name
        _RA_UpdateAppTitle();

        // notify the overlay of the new user image
        g_AchievementOverlay.UpdateImages();

        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([]() { RAUsers::LocalUser().RequestFriendList(); });
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
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
    if (pUserContext.IsLoginDisabled())
        return;

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.GetApiToken().empty() || pConfiguration.GetUsername().empty())
    {
        // show the login dialog box
        ra::ui::viewmodels::LoginViewModel vmLogin;
        vmLogin.ShowModal();
    }
    else
    {
        ra::api::Login::Request request;
        request.Username = pConfiguration.GetUsername();
        request.ApiToken = pConfiguration.GetApiToken();

        if (bBlocking)
        {
            ra::api::Login::Response response = request.Call();

            // if we're blocking, we can't keep retrying on network failure, but we can retry at least once
            if (response.Result == ra::api::ApiResult::Incomplete)
                response = request.Call();

            HandleLoginResponse(response);
        }
        else
        {
            request.CallAsyncWithRetry(HandleLoginResponse);
        }
    }
}

#ifndef RA_UTEST
API void CCONV _RA_UpdateAppTitle(const char* sMessage)
{
    std::string sTitle = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetAppTitle(sMessage ? sMessage : "");
    SetWindowText(g_RAMainWnd, NativeStr(sTitle).c_str());
}
#endif

_Use_decl_annotations_
API int _RA_UpdatePopups(ControllerInput*, float fElapsedSeconds, bool, bool bPaused)
{
    if (bPaused)
        fElapsedSeconds = 0.0;

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().Update(fElapsedSeconds);
    return 0;
}

#ifndef RA_UTEST
_Use_decl_annotations_
API int _RA_RenderPopups(HDC hDC, const RECT* rcSize)
{
    if (!g_AchievementOverlay.IsFullyVisible())
    {
        ra::ui::drawing::gdi::GDISurface pSurface(hDC, *rcSize);
        ra::services::ServiceLocator::Get<ra::ui::viewmodels::OverlayManager>().Render(pSurface);
    }

    return 0;
}
#endif
