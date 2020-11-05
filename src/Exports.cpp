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
#include "services\FrameEventQueue.hh"
#include "services\GameIdentifier.hh"
#include "services\Http.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\Initialization.hh"
#include "services\PerformanceCounter.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\Desktop.hh"
#include "ui\win32\OverlayWindow.hh"

#include "RAInterface\RA_Emulators.h"

#ifndef RA_UTEST
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameLibrary.h"
#endif

API const char* CCONV _RA_IntegrationVersion() { return RA_INTEGRATION_VERSION; }

API const char* CCONV _RA_HostName()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.GetHostName().c_str();
}

API const char* CCONV _RA_HostUrl()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.GetHostUrl().c_str();
}

API int CCONV _RA_HardcoreModeIsActive()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
}

#ifndef RA_UTEST
API void CCONV _RA_UpdateHWnd(HWND hMainHWND)
{
    auto& pDesktop = dynamic_cast<ra::ui::win32::Desktop&>(ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>());
    if (hMainHWND != pDesktop.GetMainHWnd())
    {
        pDesktop.SetMainHWnd(hMainHWND);

        auto& pOverlayWindow = ra::services::ServiceLocator::GetMutable<ra::ui::win32::OverlayWindow>();
        pOverlayWindow.CreateOverlayWindow(hMainHWND);
    }
}
#endif

static BOOL InitCommon([[maybe_unused]] HWND hMainHWND, [[maybe_unused]] int nEmulatorID,
    [[maybe_unused]] const char* sClientName, const char* sClientVer, bool bOffline)
{
#ifndef RA_UTEST
    ra::services::Initialization::RegisterServices(ra::itoe<EmulatorID>(nEmulatorID), sClientName);

    _RA_UpdateHWnd(hMainHWND);
#endif

    if (bOffline)
    {
        ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().DisableLogin();
    }
    else
    {
        // Set the client version and User-Agent string
        ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().SetClientVersion(sClientVer);

        // validate version (async call)
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([]
        {
            if (!ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().ValidateClientVersion())
                ra::services::ServiceLocator::GetMutable<ra::data::UserContext>().Logout();
        });
    }

    if (_RA_HardcoreModeIsActive())
    {
        const auto& pDesktop = dynamic_cast<ra::ui::win32::Desktop&>(ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>());
        if (pDesktop.IsDebuggerPresent())
        {
            if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Disable Hardcore mode?",
                L"A debugger or similar tool has been detected. If you do not disable hardcore mode, RetroAchievements functionality will be disabled.",
                ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
            {
                return FALSE;
            }

            RA_LOG_INFO("Hardcore disabled by external tool");
            ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().DisableHardcoreMode();
        }
    }

    return TRUE;
}

API BOOL CCONV _RA_InitOffline(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    return InitCommon(hMainHWND, nEmulatorID, nullptr, sClientVer, true);
}

API BOOL CCONV _RA_InitClientOffline(HWND hMainHWND, const char* sClientName, const char* sClientVer)
{
    return InitCommon(hMainHWND, EmulatorID::UnknownEmulator, sClientName, sClientVer, true);
}

API BOOL CCONV _RA_InitI(HWND hMainHWND, /*enum EmulatorID*/int nEmulatorID, const char* sClientVer)
{
    return InitCommon(hMainHWND, nEmulatorID, nullptr, sClientVer, false);
}

API BOOL CCONV _RA_InitClient(HWND hMainHWND, const char* sClientName, const char* sClientVer)
{
    return InitCommon(hMainHWND, EmulatorID::UnknownEmulator, sClientName, sClientVer, false);
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

        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmMessage(new ra::ui::viewmodels::PopupMessageViewModel);
        vmMessage->SetTitle(ra::StringPrintf(L"Welcome %s%s", pSessionTracker.HasSessionData() ? L"back " : L"",
            response.Username.c_str()));
        vmMessage->SetDescription((response.NumUnreadMessages == 1)
            ? L"You have 1 new message"
            : ra::StringPrintf(L"You have %u new messages", response.NumUnreadMessages));
        vmMessage->SetDetail(ra::StringPrintf(L"%u points", response.Score));
        vmMessage->SetImage(ra::ui::ImageType::UserPic, response.Username);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmMessage);

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

API const char* CCONV _RA_UserName()
{
    auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    return pUserContext.GetUsername().c_str();
}

API void CCONV _RA_SetConsoleID(unsigned int nConsoleId)
{
    auto pContext = std::make_unique<ra::data::ConsoleContext>(ra::itoe<ConsoleID>(nConsoleId));
    RA_LOG_INFO("Console set to %u (%s)", pContext->Id(), pContext->Name());
    ra::services::ServiceLocator::Provide<ra::data::ConsoleContext>(std::move(pContext));
}

API void CCONV _RA_SetUserAgentDetail(const char* sDetail)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
    pEmulatorContext.SetClientUserAgentDetail(sDetail);
}

API void CCONV _RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize)
{
    ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().AddMemoryBlock(nBankID, nBankSize,
        static_cast<ra::data::EmulatorContext::MemoryReadFunction*>(pReader),
        static_cast<ra::data::EmulatorContext::MemoryWriteFunction*>(pWriter));
}

API void CCONV _RA_ClearMemoryBanks()
{
    ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().ClearMemoryBlocks();
}

API unsigned int CCONV _RA_IdentifyRom(const BYTE* pROM, unsigned int nROMSize)
{
    return ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyGame(pROM, nROMSize);
}

API unsigned int CCONV _RA_IdentifyHash(const char* sHash)
{
    return ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyHash(sHash);
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
    auto& vmAssetList = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().AssetList;

    TALLY_PERFORMANCE(PerformanceCheckpoint::RuntimeProcess);
    std::vector<ra::services::AchievementRuntime::Change> vChanges;
    pRuntime.Process(vChanges);

    TALLY_PERFORMANCE(PerformanceCheckpoint::RuntimeEvents);
    for (const auto& pChange : vChanges)
    {
        switch (pChange.nType)
        {
            case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            {
                const auto* pAchievement = pGameContext.FindAchievement(pChange.nId);
                if (pAchievement && pAchievement->GetPauseOnReset())
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(ra::Widen(pAchievement->Title()));
                }

                const auto* vmAchievement = vmAssetList.FindAchievement(pChange.nId);
                if (vmAchievement && vmAchievement->IsPauseOnReset())
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(vmAchievement->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            {
                // AwardAchievement may detach the trigger, which would result in the state changing to Inactive.
                // explicitly call DoFrame now to set the state to Triggered before the trigger gets detached.
                auto* vmAchievement = vmAssetList.FindAchievement(pChange.nId);
                if (vmAchievement)
                    vmAchievement->DoFrame();

                pGameContext.AwardAchievement(pChange.nId);

#pragma warning(push)
#pragma warning(disable : 26462)
                auto* pAchievement = pGameContext.FindAchievement(pChange.nId);
#pragma warning(pop)
                if (pAchievement)
                {
                    if (pGameContext.HasRichPresence() && !pGameContext.IsRichPresenceFromFile())
                        pAchievement->SetUnlockRichPresence(pGameContext.GetRichPresenceDisplayString());

#ifndef RA_UTEST
                    g_AchievementsDialog.ReloadLBXData(pAchievement->ID());

                    if (g_AchievementEditorDialog.ActiveAchievement() == pAchievement)
                        g_AchievementEditorDialog.LoadAchievement(pAchievement, TRUE);
#endif

                    if (pAchievement->GetPauseOnTrigger())
                    {
                        auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                        pFrameEventQueue.QueuePauseOnTrigger(ra::Widen(pAchievement->Title()));
                    }
                }

                if (vmAchievement && vmAchievement->IsPauseOnTrigger())
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnTrigger(vmAchievement->GetName());
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

API void CCONV _RA_SetPaused(bool bIsPaused)
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    if (bIsPaused)
        pOverlayManager.ShowOverlay();
    else
        pOverlayManager.HideOverlay();
}

#ifndef RA_UTEST

static void UpdateUIForFrameChange()
{
    TALLY_PERFORMANCE(PerformanceCheckpoint::OverlayManagerAdvanceFrame);
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.AdvanceFrame();

    TALLY_PERFORMANCE(PerformanceCheckpoint::MemoryBookmarksDoFrame);
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.MemoryBookmarks.DoFrame();

    TALLY_PERFORMANCE(PerformanceCheckpoint::MemoryInspectorDoFrame);
    pWindowManager.MemoryInspector.DoFrame();

    TALLY_PERFORMANCE(PerformanceCheckpoint::AssetListDoFrame);
    pWindowManager.AssetList.DoFrame();

    TALLY_PERFORMANCE(PerformanceCheckpoint::AssetEditorDoFrame);
    pWindowManager.AssetEditor.DoFrame();

    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
    pFrameEventQueue.DoFrame();
}

#endif

API void CCONV _RA_DoAchievementsFrame()
{
    // make sure we process the achievements _before_ the frozen bookmarks modify the memory
    ProcessAchievements();

#ifndef RA_UTEST
    UpdateUIForFrameChange();
#endif

    CHECK_PERFORMANCE();
}

API void CCONV _RA_OnSaveState(const char* sFilename)
{
    ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().SaveProgressToFile(sFilename);
}

API int CCONV _RA_CaptureState(char* pBuffer, int nBufferSize)
{
    return ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().SaveProgressToBuffer(pBuffer, nBufferSize);
}

static bool CanRestoreState()
{
    if (!ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
        return false;

    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        // save state is being allowed by app (user should have been warned!)
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Disabling Hardcore mode.", L"Loading save states is not allowed in Hardcore mode.");

        RA_LOG_INFO("Hardcore disabled by loading state");
        ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>().DisableHardcoreMode();
    }

    return true;
}

static void OnStateRestored()
{
    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();

#ifndef RA_UTEST
    UpdateUIForFrameChange();

    if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
        g_AchievementEditorDialog.ActiveAchievement()->SetDirtyFlag(Achievement::DirtyFlags::Conditions);
#endif
}

API void CCONV _RA_OnLoadState(const char* sFilename)
{
    if (CanRestoreState())
    {
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().LoadProgressFromFile(sFilename);
        OnStateRestored();
    }
}

API void CCONV _RA_RestoreState(const char* pBuffer)
{
    if (CanRestoreState())
    {
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().LoadProgressFromBuffer(pBuffer);
        OnStateRestored();
    }
}
