#include "Exports.hh"

#include "RA_AchievementOverlay.h"
#include "RA_BuildVer.h"
#include "RA_Resource.h"

#include "api\Login.hh"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\Http.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

#ifndef RA_UTEST
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameLibrary.h"
#include "RA_Dlg_Memory.h"

#include "RA_LeaderboardPopup.h"
extern LeaderboardPopup g_LeaderboardPopups;
#endif

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


API void CCONV _RA_InstallSharedFunctions(bool(*)(void), void(*fpCauseUnpause)(void), void(*fpRebuildMenu)(void),
                                          void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), void(*fpLoadROM)(const char*))
{
    _RA_InstallSharedFunctionsExt(nullptr, fpCauseUnpause, nullptr, fpRebuildMenu, fpEstimateTitle, fpResetEmulation, fpLoadROM);
}

API void CCONV _RA_InstallSharedFunctionsExt(bool(*)(void), void(*fpCauseUnpause)(void), void(*fpCausePause)(void), void(*fpRebuildMenu)(void),
                                             void(*fpEstimateTitle)(char*), void(*fpResetEmulation)(void), [[maybe_unused]] void(*fpLoadROM)(const char*))
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
    pEmulatorContext.SetResetFunction(fpResetEmulation);
    pEmulatorContext.SetPauseFunction(fpCausePause);
    pEmulatorContext.SetUnpauseFunction(fpCauseUnpause);
    pEmulatorContext.SetGetGameTitleFunction(fpEstimateTitle);
    pEmulatorContext.SetRebuildMenuFunction(fpRebuildMenu);

#ifndef RA_UTEST
    g_GameLibrary.m_fpLoadROM = fpLoadROM;
#endif
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

        // notify the client to update the RetroAchievements menu
        ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().RebuildMenu();

        // update the client title-bar to include the user name
#ifndef RA_UTEST
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

API void CCONV _RA_SetConsoleID(unsigned int nConsoleId)
{
    auto pContext = ra::data::ConsoleContext::GetContext(ra::itoe<ConsoleID>(nConsoleId));
    RA_LOG("Console set to %u (%s)", pContext->Id(), pContext->Name());
    ra::services::ServiceLocator::Provide<ra::data::ConsoleContext>(std::move(pContext));

#ifndef RA_UTEST
    g_MemoryDialog.UpdateMemoryRegions();
#endif
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

API void CCONV _RA_DoAchievementsFrame()
{
#ifndef RA_UTEST
    g_MemoryDialog.Invalidate();
#endif

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    if (pRuntime.IsPaused())
        return;

#ifndef RA_UTEST
    {
        auto* pEditingAchievement = g_AchievementEditorDialog.ActiveAchievement();
        if (pEditingAchievement && pEditingAchievement->Active())
            pEditingAchievement->SetDirtyFlag(Achievement::DirtyFlags::Conditions);
    }
#endif

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    std::vector<ra::services::AchievementRuntime::Change> vChanges;
    pRuntime.Process(vChanges);

    for (const auto& pChange : vChanges)
    {
        switch (pChange.nType)
        {
            case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            {
                // we only watch for AchievementReset if PauseOnReset is set, so handle that now.
                ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().Pause();
                const auto* pAchievement = pGameContext.FindAchievement(pChange.nId);
                if (pAchievement)
                {
                    std::wstring sMessage = ra::StringPrintf(L"Pause on Reset: %s", pAchievement->Title());
                    ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                }
                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            {
                pGameContext.AwardAchievement(pChange.nId);
#pragma warning(push)
#pragma warning(disable : 26462)
                auto* pAchievement = pGameContext.FindAchievement(pChange.nId);
#pragma warning(pop)
                if (!pAchievement)
                    break;

#ifndef RA_UTEST
                //	Reverse find where I am in the list:
                unsigned int nOffset = 0;
                for (nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
                {
                    if (pAchievement == &g_pActiveAchievements->GetAchievement(nOffset))
                        break;
                }

                ASSERT(nOffset < g_pActiveAchievements->NumAchievements());
                if (nOffset < g_pActiveAchievements->NumAchievements())
                {
                    g_AchievementsDialog.ReloadLBXData(nOffset);

                    if (g_AchievementEditorDialog.ActiveAchievement() == pAchievement)
                        g_AchievementEditorDialog.LoadAchievement(pAchievement, TRUE);
                }
#endif

                if (pAchievement->GetPauseOnTrigger())
                {
                    ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().Pause();
                    std::wstring sMessage = ra::StringPrintf(L"Pause on Trigger: %s", pAchievement->Title());
                    ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardStarted:
            {
                auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    pLeaderboard->SetCurrentValue(pChange.nValue);

                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lb.wav");
                        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                            ra::StringPrintf(L"Challenge Available: %s", pLeaderboard->Title()),
                            ra::Widen(pLeaderboard->Description()));
                    }

#ifndef RA_UTEST
                    g_LeaderboardPopups.Activate(pLeaderboard->ID());
#endif
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardUpdated:
            {
                auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                    pLeaderboard->SetCurrentValue(pChange.nValue);

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardCanceled:
            {
                const auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");
                        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                            L"Leaderboard attempt canceled!", ra::Widen(pLeaderboard->Title()));
                    }

#ifndef RA_UTEST
                    g_LeaderboardPopups.Deactivate(pLeaderboard->ID());
#endif
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardTriggered:
            {
                pGameContext.SubmitLeaderboardEntry(pChange.nId, pChange.nValue);

#ifndef RA_UTEST
                g_LeaderboardPopups.Deactivate(pChange.nId);
#endif

                break;
            }
        }
    }
}

