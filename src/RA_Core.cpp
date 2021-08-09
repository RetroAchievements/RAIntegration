#include "RA_Core.h"

#include "Exports.hh"

#include "RA_Log.h"

#include "data\context\SessionTracker.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\IThreadPool.hh"
#include "services\Initialization.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

HMODULE g_hThisDLLInst = nullptr;
HWND g_RAMainWnd = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, _UNUSED LPVOID)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hThisDLLInst = hModule;
            ra::services::Initialization::RegisterCoreServices();
            break;

        case DLL_PROCESS_DETACH:
            _RA_Shutdown();
            IsolationAwareCleanup();
            break;
    }

    return TRUE;
}

static inline constexpr const char* __gsl_filename(const char* const str)
{
    if (str == nullptr)
        return str;

    if (str[0] == 's' && str[1] == 'r' && str[2] == 'c' && str[3] == '\\')
        return str;

    const char* scan = str;
    if (scan == nullptr)
        return str;

    while (*scan != '\\')
    {
        if (!*scan)
            return str;
        scan++;
    }

    return __gsl_filename(scan + 1);
}

#ifdef NDEBUG
void __gsl_contract_handler(const char* const file, unsigned int line)
{
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "Assertion failure at %s: %d", __gsl_filename(file), line);

    if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
    {
        RA_LOG_ERR(buffer);
    }

    if (ra::services::ServiceLocator::Exists<ra::ui::IDesktop>())
    {
        const auto sBuffer = ra::Widen(buffer);
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unexpected error", sBuffer);
    }

    gsl::details::throw_exception(gsl::fail_fast(buffer));
}
#else
void __gsl_contract_handler(const char* const file, unsigned int line, const char* const error)
{
    const char* const filename = __gsl_filename(file);
    const auto sError = ra::StringPrintf("Assertion failure at %s: %d: %s", filename, line, error);

    if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
    {
        RA_LOG_ERR("%s", sError.c_str());
    }

    _wassert(ra::Widen(error).c_str(), ra::Widen(filename).c_str(), line);
}
#endif

API int CCONV _RA_Shutdown()
{
    // if _RA_Init wasn't called, the services won't have been registered, so there's nothing to shut down
    if (ra::services::ServiceLocator::Exists<ra::services::IThreadPool>())
    {
        // detach any client-registered functions
        _RA_InstallSharedFunctionsExt(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        // notify the background threads as soon as possible so they start to wind down
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().Shutdown(false);

        ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();

        ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>().EndSession();

        ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>().LoadGame(0U);
    }

    if (ra::services::ServiceLocator::Exists<ra::ui::viewmodels::WindowManager>())
    {
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();

        if (pWindowManager.RichPresenceMonitor.IsVisible())
            pDesktop.CloseWindow(pWindowManager.RichPresenceMonitor);

        if (pWindowManager.MemoryInspector.IsVisible())
            pDesktop.CloseWindow(pWindowManager.MemoryInspector);

        if (pWindowManager.MemoryBookmarks.IsVisible())
            pDesktop.CloseWindow(pWindowManager.MemoryBookmarks);

        if (pWindowManager.AssetEditor.IsVisible())
            pDesktop.CloseWindow(pWindowManager.AssetEditor);

        if (pWindowManager.AssetList.IsVisible())
            pDesktop.CloseWindow(pWindowManager.AssetList);

        if (pWindowManager.CodeNotes.IsVisible())
            pDesktop.CloseWindow(pWindowManager.CodeNotes);
    }

    ra::services::Initialization::Shutdown();

    RA_LOG_INFO("Shutdown complete");

    return 0;
}

API int CCONV _RA_ConfirmLoadNewRom(int bQuittingApp)
{
    //	Returns true if we can go ahead and load the new rom.
    std::wstring sModifiedSet;

    bool bCoreModified = false;
    bool bUnofficialModified = false;
    bool bLocalModified = false;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pGameContext.Assets().GetItemAt(nIndex));
        if (pAchievement != nullptr && pAchievement->IsModified())
        {
            switch (pAchievement->GetCategory())
            {
                case ra::data::models::AssetCategory::Local:
                    bLocalModified = true;
                    break;
                case ra::data::models::AssetCategory::Core:
                    bCoreModified = true;
                    break;
                case ra::data::models::AssetCategory::Unofficial:
                    bUnofficialModified = true;
                    break;
            }
        }
    }

    if (bCoreModified)
        sModifiedSet = L"Core";
    else if (bUnofficialModified)
        sModifiedSet = L"Unofficial";
    else if (bLocalModified)
        sModifiedSet = L"Local";
    else
        return true;

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(bQuittingApp ? L"Are you sure that you want to exit?" : L"Continue load?");
    vmMessageBox.SetMessage(ra::StringPrintf(L"You have unsaved changes in the %s achievements set. If you %s, you will lose these changes.",
        sModifiedSet, bQuittingApp ? L"quit now" : L"load a new ROM"));
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    return (vmMessageBox.ShowModal() == ra::ui::DialogResult::Yes);
}

API void CCONV _RA_OnReset()
{
    // if there's no game loaded, there shouldn't be any active achievements or popups to clear - except maybe the
    // logging in messages, which we don't want to clear.
    if (ra::services::ServiceLocator::Get<ra::data::context::GameContext>().GameId() == 0U)
        return;

    // Temporarily disable achievements while the system is resetting. They will automatically re-enable when
    // DoAchievementsFrame is called if the trigger is not active. Prevents most unexpected triggering caused
    // by resetting the emulator.
    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().ResetActiveAchievements();

    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().ClearPopups();
}
