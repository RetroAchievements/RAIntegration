#include "Exports.hh"

#include "RA_BuildVer.h"
#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "api\IServer.hh"
#include "api\Login.hh"
#include "api\impl\OfflineServer.hh"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

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
#include "ui\viewmodels\IntegrationMenuViewModel.hh"
#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\Desktop.hh"
#include "ui\win32\OverlayWindow.hh"
#include "ui\win32\bindings\ControlBinding.hh"

#include <RAInterface\RA_Emulators.h>

#include <rcheevos\include\rc_api_runtime.h>

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

API int CCONV _RA_WarnDisableHardcore(const char* sActivity)
{
    std::string sActivityString;
    if (sActivity)
        sActivityString = sActivity;

    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    return pEmulatorContext.WarnDisableHardcoreMode(sActivityString) ? 1 : 0;
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

static void InitializeOfflineMode()
{
    RA_LOG_INFO("Initializing offline mode");
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::make_unique<ra::api::impl::OfflineServer>());

    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
    const auto& sUsername = pConfiguration.GetUsername();
    if (sUsername.empty())
    {
        pUserContext.Initialize("Player", "Player", "");
    }
    else
    {
        pUserContext.Initialize(sUsername, sUsername, "");

        auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>();
        pSessionTracker.Initialize(sUsername);
    }

    pUserContext.DisableLogin();
}

static BOOL InitCommon([[maybe_unused]] HWND hMainHWND, [[maybe_unused]] int nEmulatorID,
    [[maybe_unused]] const char* sClientName, const char* sClientVer, bool bOffline)
{
#ifndef RA_UTEST
    ra::services::Initialization::RegisterServices(ra::itoe<EmulatorID>(nEmulatorID), sClientName);

    _RA_UpdateHWnd(hMainHWND);

    // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
    // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
    // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
    switch (ra::itoe<EmulatorID>(nEmulatorID))
    {
        case RA_Libretro:
        case RA_Oricutron:
            ra::ui::win32::bindings::ControlBinding::SetNeedsUpdateWindow(true);
            break;

        default:
            ra::ui::win32::bindings::ControlBinding::SetNeedsUpdateWindow(false);
            break;
    }
#endif

    // Set the client version and User-Agent string
    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().SetClientVersion(sClientVer);

    if (bOffline)
    {
        InitializeOfflineMode();
    }
    else
    {
        // validate version (async call)
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([]
        {
            if (!ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().ValidateClientVersion())
                ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>().Logout();
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
            ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().DisableHardcoreMode();
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
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.SetResetFunction(fpResetEmulation);
    pEmulatorContext.SetPauseFunction(fpCausePause);
    pEmulatorContext.SetUnpauseFunction(fpCauseUnpause);
    pEmulatorContext.SetGetGameTitleFunction(fpEstimateTitle);
    pEmulatorContext.SetRebuildMenuFunction(fpRebuildMenu);
}

API HMENU CCONV _RA_CreatePopupMenu()
{
    HMENU hMenu = CreatePopupMenu();

    ra::ui::viewmodels::LookupItemViewModelCollection vmMenuItems;
    ra::ui::viewmodels::IntegrationMenuViewModel::BuildMenu(vmMenuItems);

    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vmMenuItems.Count()); ++i)
    {
        const auto* pItem = vmMenuItems.GetItemAt(i);
        Expects(pItem != nullptr);

        const int nId = pItem->GetId();
        if (nId == 0)
            AppendMenu(hMenu, MF_SEPARATOR, 0U, nullptr);
        else if (pItem->IsSelected())
            AppendMenu(hMenu, MF_CHECKED, nId, pItem->GetLabel().c_str());
        else
            AppendMenu(hMenu, MF_STRING, nId, pItem->GetLabel().c_str());
    }

    return hMenu;
}

API int CCONV _RA_GetPopupMenuItems(RA_MenuItem *pItems)
{
    // have to keep a static variable here to hold label references
    static ra::ui::viewmodels::LookupItemViewModelCollection vmMenuItems;

    vmMenuItems.Clear();
    ra::ui::viewmodels::IntegrationMenuViewModel::BuildMenu(vmMenuItems);

    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vmMenuItems.Count()); ++i)
    {
        const auto* pItem = vmMenuItems.GetItemAt(i);
        Expects(pItem != nullptr);

        const int nId = pItem->GetId();
        pItems[i].nID = nId;

        if (nId == 0)
        {
            pItems[i].sLabel = nullptr;
            pItems[i].bChecked = 0;
        }
        else
        {
            pItems[i].sLabel = pItem->GetLabel().c_str();
            pItems[i].bChecked = pItem->IsSelected() ? 1 : 0;
        }
    }

    return gsl::narrow_cast<int>(vmMenuItems.Count());
}

API void CCONV _RA_InvokeDialog(LPARAM nID)
{
    ra::ui::viewmodels::IntegrationMenuViewModel::ActivateMenuItem(gsl::narrow_cast<int>(nID));
}

static void HandleLoginResponse(const ra::api::Login::Response& response)
{
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
    if (pUserContext.IsLoginDisabled())
        return;

    if (response.Succeeded())
    {
        // initialize the user context
        pUserContext.Initialize(response.Username, response.DisplayName, response.ApiToken);
        pUserContext.SetScore(response.Score);

        // load the session information
        auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>();
        pSessionTracker.Initialize(response.Username);

        // show the welcome message
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\login.wav");

        std::unique_ptr<ra::ui::viewmodels::PopupMessageViewModel> vmMessage(new ra::ui::viewmodels::PopupMessageViewModel);
        vmMessage->SetTitle(ra::StringPrintf(L"Welcome %s%s", pSessionTracker.HasSessionData() ? L"back " : L"",
            response.DisplayName.c_str()));
        vmMessage->SetDescription((response.NumUnreadMessages == 1)
            ? L"You have 1 new message"
            : ra::StringPrintf(L"You have %u new messages", response.NumUnreadMessages));
        vmMessage->SetDetail(ra::StringPrintf(L"%u points", response.Score));
        vmMessage->SetImage(ra::ui::ImageType::UserPic, response.Username);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(vmMessage);

        // notify the client to update the RetroAchievements menu
        ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().RebuildMenu();

        // update the client title-bar to include the user name
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator.UpdateWindowTitle();
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

API void CCONV _RA_AttemptLogin(int bBlocking)
{
    auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
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
    auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    return pUserContext.GetDisplayName().c_str();
}

API void CCONV _RA_SetConsoleID(unsigned int nConsoleId)
{
    auto pContext = std::make_unique<ra::data::context::ConsoleContext>(ra::itoe<ConsoleID>(nConsoleId));
    RA_LOG_INFO("Console set to %u (%s)", pContext->Id(), pContext->Name());
    ra::services::ServiceLocator::Provide<ra::data::context::ConsoleContext>(std::move(pContext));
}

API void CCONV _RA_SetUserAgentDetail(const char* sDetail)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.SetClientUserAgentDetail(sDetail);
}

API void CCONV _RA_InstallMemoryBank(int nBankID, void* pReader, void* pWriter, int nBankSize)
{
    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().AddMemoryBlock(nBankID, nBankSize,
        static_cast<ra::data::context::EmulatorContext::MemoryReadFunction*>(pReader),
        static_cast<ra::data::context::EmulatorContext::MemoryWriteFunction*>(pWriter));
}

API void CCONV _RA_InstallMemoryBankBlockReader(int nBankID, void* pReader)
{
    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().AddMemoryBlockReader(
        nBankID, static_cast<ra::data::context::EmulatorContext::MemoryReadBlockFunction*>(pReader));
}

API void CCONV _RA_ClearMemoryBanks()
{
    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().ClearMemoryBlocks();
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
    _RA_SuspendRepaint();
    ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().ActivateGame(nGameId);
    _RA_ResumeRepaint();
}

API int CCONV _RA_OnLoadNewRom(const BYTE* pROM, unsigned int nROMSize)
{
    ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyAndActivateGame(pROM, nROMSize);
    return 0;
}

API void CCONV _RA_UpdateAppTitle(const char* sMessage)
{
    auto& vmEmulator = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().Emulator;
    vmEmulator.SetAppTitleMessage(sMessage);
}

API int _RA_IsOverlayFullyVisible()
{
    return ra::services::ServiceLocator::Get<ra::ui::viewmodels::OverlayManager>().IsOverlayFullyVisible() ? 1 : 0;
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

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    
    TALLY_PERFORMANCE(PerformanceCheckpoint::RuntimeProcess);
    std::vector<ra::services::AchievementRuntime::Change> vChanges;
    pRuntime.Process(vChanges);

    const ra::data::models::AchievementModel* vmMeasuredAchievement = nullptr;
    const rc_trigger_t* pMeasuredAchievementTrigger = nullptr;
    float fMeasuredAchievementPercent = 0.0;

    TALLY_PERFORMANCE(PerformanceCheckpoint::RuntimeEvents);
    for (const auto& pChange : vChanges)
    {
        switch (pChange.nType)
        {
            case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            {
                const auto* pAchievement = pGameContext.Assets().FindAchievement(pChange.nId);
                if (pAchievement && pAchievement->IsPauseOnReset())
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(pAchievement->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            {
                // AwardAchievement may detach the trigger, which would result in the state changing to Inactive.
                // explicitly set the state to Triggered before the trigger gets detached.
                auto* vmAchievement = pGameContext.Assets().FindAchievement(pChange.nId);
                if (vmAchievement)
                    vmAchievement->SetState(ra::data::models::AssetState::Triggered);

                pGameContext.AwardAchievement(pChange.nId);

                if (vmAchievement && vmAchievement->IsPauseOnTrigger())
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnTrigger(vmAchievement->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementDisabled:
            {
                auto* vmAchievement = pGameContext.Assets().FindAchievement(pChange.nId);
                if (vmAchievement)
                {
                    RA_LOG_WARN("Achievement #%u disabled: address %s not supported by current core", pChange.nId, ra::ByteAddressToString(pChange.nValue));
                    vmAchievement->SetState(ra::data::models::AssetState::Disabled);
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementPrimed:
            {
                const auto* vmAchievement = pGameContext.Assets().FindAchievement(pChange.nId);
                if (vmAchievement)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Challenge) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                        pOverlayManager.AddChallengeIndicator(vmAchievement->GetID(), ra::ui::ImageType::Badge, ra::Narrow(vmAchievement->GetBadge()));
                    }
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementUnprimed:
            {
                auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                pOverlayManager.RemoveChallengeIndicator(pChange.nId);
                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardStarted:
            {
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lb.wav");
                        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                        pOverlayManager.QueueMessage(ra::ui::viewmodels::Popup::LeaderboardStarted,
                            L"Leaderboard attempt started", pLeaderboard->GetName(), pLeaderboard->GetDescription());
                    }

                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                        auto& pScoreTracker = pOverlayManager.AddScoreTracker(pLeaderboard->GetID());
                        const auto sDisplayText = pLeaderboard->FormatScore(pChange.nValue);
                        pScoreTracker.SetDisplayText(ra::Widen(sDisplayText));
                    }
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardUpdated:
            {
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
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
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");

                        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                        pOverlayManager.QueueMessage(ra::ui::viewmodels::Popup::LeaderboardCanceled,
                            L"Leaderboard attempt failed", pLeaderboard->GetName(), pLeaderboard->GetDescription());
                    }

                    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                    pOverlayManager.RemoveScoreTracker(pLeaderboard->GetID());
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

            case ra::services::AchievementRuntime::ChangeType::LeaderboardStartReset:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(L"Start: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardSubmitReset:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(L"Submit: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardCancelReset:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(L"Cancel: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardValueReset:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnReset(L"Value: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardStartTriggered:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnTrigger(L"Start: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardSubmitTriggered:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnTrigger(L"Submit: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::LeaderboardCancelTriggered:
            {
                /* ASSERT: this event is only raised if the leaderboard is watching for it */
                const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(pChange.nId);
                if (pLeaderboard)
                {
                    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
                    pFrameEventQueue.QueuePauseOnTrigger(L"Cancel: " + pLeaderboard->GetName());
                }

                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementProgressChanged:
            {
                const auto* pTrigger = pRuntime.GetAchievementTrigger(pChange.nId);
                if (pTrigger != nullptr && pTrigger->measured_target > 0)
                {
                    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Progress) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        const auto* vmAchievement = pGameContext.Assets().FindAchievement(pChange.nId);
                        if (vmAchievement != nullptr)
                        {
                            const float fProgress = gsl::narrow_cast<float>(pTrigger->measured_value) / gsl::narrow_cast<float>(pTrigger->measured_target);
                            if (fProgress > fMeasuredAchievementPercent)
                            {
                                fMeasuredAchievementPercent = fProgress;
                                vmMeasuredAchievement = vmAchievement;
                                pMeasuredAchievementTrigger = pTrigger;
                            }
                        }
                     }
                 }

                 break;
            }
        }
    }

    if (vmMeasuredAchievement && pMeasuredAchievementTrigger)
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.UpdateProgressTracker(ra::ui::ImageType::Badge,
            ra::StringPrintf("%s_lock", vmMeasuredAchievement->GetBadge()),
            pMeasuredAchievementTrigger->measured_value, pMeasuredAchievementTrigger->measured_target,
            pMeasuredAchievementTrigger->measured_as_percent);
    }
}

API void CCONV _RA_SetPaused(int bIsPaused)
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
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.DoFrame();

    TALLY_PERFORMANCE(PerformanceCheckpoint::AssetEditorDoFrame);
    pWindowManager.AssetEditor.DoFrame();

    pWindowManager.PointerFinder.DoFrame();

    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
    pFrameEventQueue.DoFrame();
}

#endif

API void CCONV _RA_DoAchievementsFrame()
{
#ifndef RA_UTEST
    ra::ui::win32::bindings::ControlBinding::RepaintGuard guard;
#endif

    // make sure we process the achievements _before_ the frozen bookmarks modify the memory
    ProcessAchievements();

#ifndef RA_UTEST
    UpdateUIForFrameChange();
#endif

    CHECK_PERFORMANCE();
}

API void CCONV _RA_SetForceRepaint([[maybe_unused]] int bEnable)
{
#ifndef RA_UTEST
    ra::ui::win32::bindings::ControlBinding::SetNeedsUpdateWindow(bEnable != 0);
#endif
}

API void CCONV _RA_SuspendRepaint()
{
#ifndef RA_UTEST
    ra::ui::win32::bindings::ControlBinding::SuspendRepaint();
#endif
}

API void CCONV _RA_ResumeRepaint()
{
#ifndef RA_UTEST
    ra::ui::win32::bindings::ControlBinding::ResumeRepaint();
#endif
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
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

    if (!ra::services::ServiceLocator::Get<ra::data::context::UserContext>().IsLoggedIn())
    {
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
            return false;
    }

    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        // save state is being allowed by app (user should have been warned!)
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Disabling Hardcore mode.", L"Loading save states is not allowed in Hardcore mode.");

        RA_LOG_INFO("Hardcore disabled by loading state");
        ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().DisableHardcoreMode();
    }

    return true;
}

static void OnStateRestored()
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.ClearPopups();

    auto& pAssets = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>().Assets();
    pAssets.BeginUpdate();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pAssets.Count()); ++nIndex)
    {
        auto* pAsset = pAssets.GetItemAt(nIndex);
        Expects(pAsset != nullptr);

        switch (pAsset->GetType())
        {
            case ra::data::models::AssetType::Achievement:
            {
                auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pAsset);
                Expects(pAchievement != nullptr);

                // synchronize the state
                pAchievement->DoFrame();

                // if it's Primed, show the indicator
                if (pAchievement->GetState() == ra::data::models::AssetState::Primed)
                    pOverlayManager.AddChallengeIndicator(pAchievement->GetID(), ra::ui::ImageType::Badge, ra::Narrow(pAchievement->GetBadge()));
                break;
            }

            case ra::data::models::AssetType::Leaderboard:
            {
                auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(pAsset);
                Expects(pLeaderboard != nullptr);

                // synchronize the state
                pLeaderboard->DoFrame();

                // if it's Primed, show the tracker
                if (pLeaderboard->GetState() == ra::data::models::AssetState::Primed)
                {
                    if (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker) != ra::ui::viewmodels::PopupLocation::None)
                    {
                        auto& pScoreTracker = pOverlayManager.AddScoreTracker(pLeaderboard->GetID());

                        const auto pDefinition = pRuntime.GetLeaderboardDefinition(pLeaderboard->GetID());
                        if (pDefinition != nullptr)
                        {
                            const auto sDisplayText = pLeaderboard->FormatScore(pDefinition->value.value.value);
                            pScoreTracker.SetDisplayText(ra::Widen(sDisplayText));
                        }
                    }
                }
                break;
            }
        }
    }
    pAssets.EndUpdate();

#ifndef RA_UTEST
    UpdateUIForFrameChange();
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
