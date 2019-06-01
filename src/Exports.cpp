#include "Exports.hh"

#include "RA_BuildVer.h"
#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "api\Login.hh"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\GameIdentifier.hh"
#include "services\Http.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

#ifndef RA_UTEST
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameLibrary.h"
#include "RA_Dlg_Memory.h"
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

        // load the session information
        auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>();
        pSessionTracker.Initialize(response.Username);

        // show the welcome message
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\login.wav");

        ra::ui::viewmodels::PopupMessageViewModel message;
        message.SetTitle(ra::StringPrintf(L"Welcome %s%s", pSessionTracker.HasSessionData() ? L"back " : L"",
            response.Username.c_str()));
        message.SetDescription((response.NumUnreadMessages == 1)
            ? L"You have 1 new message"
            : ra::StringPrintf(L"You have %u new messages", response.NumUnreadMessages));
        message.SetDetail(ra::StringPrintf(L"%u points", response.Score));
        message.SetImage(ra::ui::ImageType::UserPic, response.Username);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(std::move(message));

        // notify the client to update the RetroAchievements menu
        ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().RebuildMenu();

        // update the client title-bar to include the user name
        _RA_UpdateAppTitle();
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

API unsigned int CCONV _RA_IdentifyRom(const BYTE* pROM, unsigned int nROMSize)
{
    return ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyGame(pROM, nROMSize);
}

API void CCONV _RA_ActivateGame(unsigned int nGameId)
{
    ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().ActivateGame(nGameId);
}

API int CCONV _RA_OnLoadNewRom(const BYTE* pROM, unsigned int nROMSize)
{
    ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyAndActivateGame(pROM, nROMSize);
    return 0;
}

API void CCONV _RA_UpdateAppTitle(const char* sMessage)
{
    auto sTitle = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetAppTitle(sMessage ? sMessage : "");
    auto& vmEmulator = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator;
    vmEmulator.SetWindowTitle(sTitle);
}

API bool _RA_IsOverlayFullyVisible()
{
    return ra::services::ServiceLocator::Get<ra::ui::viewmodels::OverlayManager>().IsOverlayFullyVisible();
}

_Use_decl_annotations_
API void _RA_NavigateOverlay(const ControllerInput* pInput)
{
    static const ControllerInput pNoInput{};
    if (pInput == nullptr)
        pInput = &pNoInput;

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().Update(*pInput);
}

_Use_decl_annotations_
API int _RA_UpdateOverlay(const ControllerInput* pInput, float, bool, bool)
{
    _RA_NavigateOverlay(pInput);
    return true; // was return state = closing - does anything check this?
}

#ifndef RA_UTEST
_Use_decl_annotations_
API void _RA_RenderOverlay(HDC hDC, const RECT* rcSize)
{
    switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
    {
        case EmulatorID::RA_Gens:
            ra::ui::drawing::gdi::GDISurface pSurface(hDC, *rcSize);
            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().Render(pSurface, true);
            break;
    }
}
#endif

static void ProcessAchievements()
{
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

                if (pGameContext.HasRichPresence())
                    pAchievement->SetUnlockRichPresence(pGameContext.GetRichPresenceDisplayString());

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
                const auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lb.wav");
                        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                            L"Leaderboard Attempt Started", ra::Widen(pLeaderboard->Title()), ra::Widen(pLeaderboard->Description()));
                    }

                    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                    auto& pScoreTracker = pOverlayManager.AddScoreTracker(pLeaderboard->ID());
                    const auto sDisplayText = pLeaderboard->FormatScore(pChange.nValue);
                    pScoreTracker.SetDisplayText(ra::Widen(sDisplayText));
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardUpdated:
            {
                const auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                    auto* pScoreTracker = pOverlayManager.GetScoreTracker(pChange.nId);
                    if (pScoreTracker != nullptr)
                    {
                        const auto sDisplayText = pLeaderboard->FormatScore(pChange.nValue);
                        pScoreTracker->SetDisplayText(ra::Widen(sDisplayText));
                    }
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardCanceled:
            {
                const auto* pLeaderboard = pGameContext.FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCancelNotifications))
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");
                        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                            L"Leaderboard Attempt Canceled", ra::Widen(pLeaderboard->Title()), ra::Widen(pLeaderboard->Description()));
                    }

                    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                    pOverlayManager.RemoveScoreTracker(pLeaderboard->ID());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardTriggered:
            {
                pGameContext.SubmitLeaderboardEntry(pChange.nId, pChange.nValue); // will show the scoreboard when submission completes

                auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                pOverlayManager.RemoveScoreTracker(pChange.nId);
                break;
            }
        }
    }
}

API void CCONV _RA_DoAchievementsFrame()
{
    ProcessAchievements();

#ifndef RA_UTEST
    // make sure we process the achievements _before_ the frozen bookmarks modify the memory
    g_MemoryDialog.Invalidate();
#endif
}
