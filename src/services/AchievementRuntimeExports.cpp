#include "AchievementRuntime.hh"

#include "Exports.hh"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "data\context\ConsoleContext.hh"
#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"

#include "ui\viewmodels\IntegrationMenuViewModel.hh"

#include <rcheevos\src\rc_client_internal.h>
#include <rcheevos\src\rc_client_external.h>
#include <rcheevos\include\rc_client_raintegration.h>

namespace ra {
namespace services {

class AchievementRuntimeExports : private AchievementRuntime
{
public:
    static void destroy() noexcept
    {
        memset(&s_callbacks, 0, sizeof(s_callbacks));

        if (s_pIntegrationMenu)
        {
            s_pIntegrationMenu = nullptr;
            rc_buffer_destroy(&s_pIntegrationMenuBuffer);
        }

        s_bIsExternalRcheevosClient = false;
    }

    static void enable_logging(rc_client_t* client, int level, rc_client_message_callback_t callback)
    {
        s_callbacks.log_callback = callback;
        s_callbacks.log_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_enable_logging(pClient.GetClient(), level, AchievementRuntimeExports::LogMessageExternal);
    }

    static void set_event_handler(rc_client_t* client, rc_client_event_handler_t handler)
    {
        s_callbacks.event_handler = handler;
        s_callbacks.event_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_event_handler(pClient.GetClient(), AchievementRuntimeExports::EventHandlerExternal);
    }

    static void set_read_memory(rc_client_t* client, rc_client_read_memory_func_t handler)
    {
        s_callbacks.read_memory_handler = handler;
        s_callbacks.read_memory_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_read_memory_function(pClient.GetClient(), AchievementRuntimeExports::ReadMemoryExternal);

        ResetMemory();
    }

    static void ResetMemory()
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
        const auto nNumBytes = pConsoleContext.MaxAddress() + 1;

        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.ClearMemoryBlocks();

        if (nNumBytes > 1)
        {
            pEmulatorContext.AddMemoryBlock(0, nNumBytes,
                                            AchievementRuntimeExports::ReadMemoryByte,
                                            AchievementRuntimeExports::WriteMemoryByte);
            pEmulatorContext.AddMemoryBlockReader(0, AchievementRuntimeExports::ReadMemoryBlock);
        }
    }

    static void set_get_time_millisecs(rc_client_t* client, rc_get_time_millisecs_func_t handler)
    {
        s_callbacks.get_time_millisecs_handler = handler;
        s_callbacks.get_time_millisecs_client = client;

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_get_time_millisecs_function(pClient.GetClient(), AchievementRuntimeExports::GetTimeMillisecsExternal);
    }

    static void set_host(const char* value)
    {
        auto* pConfiguration = dynamic_cast<ra::services::impl::JsonFileConfiguration*>(
            &ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>());
        if (pConfiguration != nullptr)
            pConfiguration->SetHost(value);
    }

    static size_t get_user_agent_clause(char buffer[], size_t buffer_size)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        const auto sUserAgent = pEmulatorContext.GetUserAgentClause();
        return snprintf(buffer, buffer_size, "%s", sUserAgent.c_str());
    }

    static bool IsUpdatingHardcore() noexcept { return s_bUpdatingHardcore; }

    static void set_hardcore_enabled(int value)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();

        s_bUpdatingHardcore = true; // prevent raising RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED event

        if (value)
            pEmulatorContext.EnableHardcoreMode(false);
        else
            pEmulatorContext.DisableHardcoreMode();

        s_bUpdatingHardcore = false;
    }

    static int get_hardcore_enabled()
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    }

    static void set_unofficial_enabled(int) noexcept
    {
        // do nothing. unofficial achievements should always be available when using the toolkit.
    }

    static int get_unofficial_enabled() noexcept
    {
        // unofficial achievements should always be available when using the toolkit.
        return true;
    }

    static void set_encore_mode_enabled(int value)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_encore_mode_enabled(pClient.GetClient(), value);
    }

    static int get_encore_mode_enabled()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_encore_mode_enabled(pClient.GetClient());
    }

    static void set_spectator_mode_enabled(int value)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        rc_client_set_spectator_mode_enabled(pClient.GetClient(), value);
    }

    static int get_spectator_mode_enabled()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_spectator_mode_enabled(pClient.GetClient());
    }

    static void abort_async(rc_client_async_handle_t* handle)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        rc_client_abort_async(pClient.GetClient(), handle);
    }

    static rc_client_async_handle_t* begin_login_with_password(rc_client_t* client, const char* username,
                                                               const char* password, rc_client_callback_t callback,
                                                               void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoginWithPassword(username, password, pCallbackData);
    }

    static rc_client_async_handle_t* begin_login_with_token(rc_client_t* client, const char* username,
                                                            const char* token, rc_client_callback_t callback,
                                                            void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackWrapper = new LoadGameCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoginWithToken(username, token, pCallbackWrapper);
    }

    static const rc_client_user_t* get_user_info()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_get_user_info(pClient.GetClient());
    }

    static void logout()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_logout(pClient.GetClient());
    }

    class LoadExternalGameCallbackWrapper : public CallbackWrapper
    {
    public:
        LoadExternalGameCallbackWrapper(rc_client_t* client, rc_client_callback_t callback,
                                        void* callback_userdata) noexcept :
            CallbackWrapper(client, callback, callback_userdata)
        {}

        bool bWasPaused = false;
    };

    static void load_game_callback(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
    {
        auto* wrapper = static_cast<LoadExternalGameCallbackWrapper*>(pUserdata);
        Expects(wrapper != nullptr);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        const auto* pGame = pClient.GetClient()->game;
        if (pGame)
        {
            _RA_SetConsoleID(pGame->public_.console_id);
            ResetMemory();
        }

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.EndLoadGame(nResult, wrapper->bWasPaused, false);

        wrapper->DoCallback(nResult, sErrorMessage);

        delete wrapper;
    }

    static rc_client_async_handle_t* begin_identify_and_load_game(rc_client_t* client, uint32_t console_id,
                                                                  const char* file_path, const uint8_t* data,
                                                                  size_t data_size, rc_client_callback_t callback,
                                                                  void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pNestedCallbackData = new LoadExternalGameCallbackWrapper(client, callback, callback_userdata);
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LoadGameCallbackWrapper(client, load_game_callback, pNestedCallbackData);

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.BeginLoadGame(0xFFFFFFFF, ra::data::context::GameContext::Mode::Normal,
                                   pNestedCallbackData->bWasPaused);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginIdentifyAndLoadGame(console_id, file_path, data, data_size, pCallbackData);
    }

    static rc_client_async_handle_t* begin_load_game(rc_client_t* client, const char* hash,
                                                     rc_client_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pNestedCallbackData = new LoadExternalGameCallbackWrapper(client, callback, callback_userdata);
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LoadGameCallbackWrapper(client, load_game_callback, pNestedCallbackData);

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.BeginLoadGame(0xFFFFFFFF, ra::data::context::GameContext::Mode::Normal,
                                   pNestedCallbackData->bWasPaused);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginLoadGame(hash, 0, pCallbackData);
    }

    static void unload_game()
    {
        _RA_ActivateGame(0);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_unload_game(pClient.GetClient());
    }

    static const rc_client_game_t* get_game_info()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_game_info(pClient.GetClient());
    }

    static const rc_client_subset_t* get_subset_info(uint32_t subset_id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_subset_info(pClient.GetClient(), subset_id);
    }

    static void get_user_game_summary(rc_client_user_game_summary_t* summary)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_user_game_summary(pClient.GetClient(), summary);
    }

    static rc_client_async_handle_t* begin_change_media(rc_client_t* client, const char* file_path,
                                                        const uint8_t* data, size_t data_size,
                                                        rc_client_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new CallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return pClient.BeginChangeMedia(file_path, data, data_size, pCallbackData);
    }

    static rc_client_achievement_list_info_t* create_achievement_list(int category, int grouping)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        GSL_SUPPRESS_TYPE1
        auto* list = reinterpret_cast<rc_client_achievement_list_info_t*>(
            rc_client_create_achievement_list(pClient.GetClient(), category, grouping));
        list->destroy_func = destroy_achievement_list;
        return list;
    }

    static int has_achievements()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_achievements(pClient.GetClient());
    }

    static const rc_client_achievement_t* get_achievement_info(uint32_t id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_achievement_info(pClient.GetClient(), id);
    }

    static rc_client_leaderboard_list_info_t* create_leaderboard_list(int grouping)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        GSL_SUPPRESS_TYPE1
        auto* list = reinterpret_cast<rc_client_leaderboard_list_info_t*>(
            rc_client_create_leaderboard_list(pClient.GetClient(), grouping));
        list->destroy_func = destroy_leaderboard_list;
        return list;
    }

    static int has_leaderboards()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_leaderboards(pClient.GetClient());
    }

    static const rc_client_leaderboard_t* get_leaderboard_info(uint32_t id)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_leaderboard_info(pClient.GetClient(), id);
    }

private:
    class LeaderboardEntriesListCallbackWrapper
    {
    public:
        LeaderboardEntriesListCallbackWrapper(rc_client_t* client,
                                              rc_client_fetch_leaderboard_entries_callback_t callback,
                                              void* callback_userdata) noexcept :
            m_pClient(client), m_fCallback(callback), m_pCallbackUserdata(callback_userdata)
        {}

        void DoCallback(int nResult, rc_client_leaderboard_entry_list_t* pList, const char* sErrorMessage) noexcept
        {
            m_fCallback(nResult, sErrorMessage, pList, m_pClient, m_pCallbackUserdata);
        }

        static void Dispatch(int nResult, const char* sErrorMessage, rc_client_leaderboard_entry_list_t* pList,
                             rc_client_t*, void* pUserdata)
        {
            auto* wrapper = static_cast<LeaderboardEntriesListCallbackWrapper*>(pUserdata);
            Expects(wrapper != nullptr);

            if (pList)
            {
                GSL_SUPPRESS_TYPE1
                reinterpret_cast<rc_client_leaderboard_entry_list_info_t*>(pList)->destroy_func = destroy_leaderboard_entry_list;
            }

            wrapper->DoCallback(nResult, pList, sErrorMessage);

            delete wrapper;
        }

    private:
        rc_client_t* m_pClient;
        rc_client_fetch_leaderboard_entries_callback_t m_fCallback;
        void* m_pCallbackUserdata;
    };

public:
    static rc_client_async_handle_t* begin_fetch_leaderboard_entries(rc_client_t* client,
        uint32_t leaderboard_id, uint32_t first_entry, uint32_t count,
        rc_client_fetch_leaderboard_entries_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LeaderboardEntriesListCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_begin_fetch_leaderboard_entries(pClient.GetClient(), leaderboard_id, first_entry, count,
                                                         LeaderboardEntriesListCallbackWrapper::Dispatch, pCallbackData);
    }

    static rc_client_async_handle_t* begin_fetch_leaderboard_entries_around_user(rc_client_t* client,
        uint32_t leaderboard_id, uint32_t count,
        rc_client_fetch_leaderboard_entries_callback_t callback, void* callback_userdata)
    {
        GSL_SUPPRESS_R3
        auto* pCallbackData = new LeaderboardEntriesListCallbackWrapper(client, callback, callback_userdata);

        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_begin_fetch_leaderboard_entries_around_user(pClient.GetClient(), leaderboard_id, count,
                                                                     LeaderboardEntriesListCallbackWrapper::Dispatch, pCallbackData);
    }

    static size_t get_rich_presence_message(char buffer[], size_t buffer_size)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_get_rich_presence_message(pClient.GetClient(), buffer, buffer_size);
    }

    static int has_rich_presence()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_has_rich_presence(pClient.GetClient());
    }

    static void do_frame() noexcept
    {
        _RA_DoAchievementsFrame();
    }

    static void idle()
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_idle(pClient.GetClient());
    }

    static int is_processing_required()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_is_processing_required(pClient.GetClient());
    }

    static int can_pause(uint32_t* frames_remaining)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_can_pause(pClient.GetClient(), frames_remaining);
    }

    static void reset() noexcept
    {
#ifndef RA_UTEST
        _RA_OnReset();
#endif
    }

    static size_t progress_size()
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_progress_size(pClient.GetClient());
    }

    static int serialize_progress(uint8_t* buffer, size_t buffer_size)
    {
        const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        return rc_client_serialize_progress_sized(pClient.GetClient(), buffer, buffer_size);
    }

    static int deserialize_progress(const uint8_t* buffer, size_t buffer_size)
    {
        auto& pClient = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        return rc_client_deserialize_progress_sized(pClient.GetClient(), buffer, buffer_size);
    }

    static void set_raintegration_write_memory_function(rc_client_t* client, rc_client_raintegration_write_memory_func_t handler) noexcept
    {
        s_callbacks.write_memory_client = client;
        s_callbacks.write_memory_handler = handler;
    }

    static void set_raintegration_get_game_name_function(rc_client_t* client, rc_client_raintegration_get_game_name_func_t handler)
    {
        s_callbacks.get_game_name_client = client;
        s_callbacks.get_game_name_handler = handler;

        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.SetGetGameTitleFunction(GetGameTitle);
    }

    static int get_achievement_state(uint32_t nAchievementId)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
        if (pAchievement == nullptr)
            return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_NONE;

        if (pAchievement->GetCategory() == ra::data::models::AssetCategory::Local)
            return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_LOCAL;

        if (pAchievement->IsModified() ||                                       // actual modifications
            pAchievement->GetChanges() != ra::data::models::AssetChanges::None) // unpublished changes
            return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_MODIFIED;

        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        if (pEmulatorContext.WasMemoryModified())
            return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_INSECURE;

        if (_RA_HardcoreModeIsActive() && pEmulatorContext.IsMemoryInsecure())
            return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_INSECURE;

        return RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_PUBLISHED;
    }

    static void set_raintegration_event_handler(rc_client_t* client, rc_client_raintegration_event_handler_t handler) noexcept
    {
        s_callbacks.raintegration_event_client = client;
        s_callbacks.raintegration_event_handler = handler;
    }

    static const rc_client_raintegration_menu_t* get_raintegration_menu()
    {
        ra::ui::viewmodels::LookupItemViewModelCollection vmMenuItems;
        ra::ui::viewmodels::IntegrationMenuViewModel::BuildMenu(vmMenuItems);

        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(vmMenuItems.Count()) - 1; nIndex >= 0; --nIndex)
        {
            const auto* pItem = vmMenuItems.GetItemAt(nIndex);
            if (!pItem)
                continue;

            switch (pItem->GetId())
            {
                case IDM_RA_FILES_LOGIN:
                case IDM_RA_FILES_LOGOUT:
                case IDM_RA_TOGGLELEADERBOARDS:
                case IDM_RA_OVERLAYSETTINGS:
                    vmMenuItems.RemoveAt(nIndex);
                    break;

                case 0: // separator - prevent two adjacent separators
                    pItem = vmMenuItems.GetItemAt(nIndex + 1);
                    if (pItem && pItem->GetId() == 0)
                        vmMenuItems.RemoveAt(nIndex + 1);
                    break;
            }
        }

        {
            const auto* pItem = vmMenuItems.GetItemAt(0);
            if (pItem && pItem->GetId() == 0)
                vmMenuItems.RemoveAt(0);
        }

        bool bChanged = true;
        rc_client_raintegration_menu_item_t* pMenuItem = nullptr;

        if (s_pIntegrationMenu && s_pIntegrationMenu->num_items == vmMenuItems.Count())
        {
            bChanged = false;

            pMenuItem = s_pIntegrationMenu->items;
            for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vmMenuItems.Count()); ++nIndex, ++pMenuItem)
            {
                const auto* pItem = vmMenuItems.GetItemAt(nIndex);
                if (!pItem)
                    continue;

                const auto nId = ra::to_unsigned(pItem->GetId());
                if (pMenuItem->id != nId)
                {
                    if (pMenuItem->id == 0 || nId == 0)
                    {
                        bChanged = true;
                        break;
                    }

                    pMenuItem->id = nId;
                }

                pMenuItem->checked = pItem->IsSelected() ? 1 : 0;
            }

            if (!bChanged)
            {
                pMenuItem = s_pIntegrationMenu->items;
                for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vmMenuItems.Count());
                     ++nIndex, ++pMenuItem)
                {
                    const auto* pItem = vmMenuItems.GetItemAt(nIndex);
                    if (!pItem)
                        continue;

                    if (pMenuItem->label)
                    {
                        if (ra::Narrow(pItem->GetLabel()) != pMenuItem->label)
                        {
                            bChanged = true;
                            break;
                        }
                    }
                    else if (!pItem->GetLabel().empty())
                    {
                        bChanged = true;
                        break;
                    }
                }
            }
        }

        if (!bChanged)
            return s_pIntegrationMenu;

        if (s_pIntegrationMenu)
            rc_buffer_destroy(&s_pIntegrationMenuBuffer);

        rc_buffer_init(&s_pIntegrationMenuBuffer);
        s_pIntegrationMenu = static_cast<rc_client_raintegration_menu_t*>(
            rc_buffer_alloc(&s_pIntegrationMenuBuffer, sizeof(*s_pIntegrationMenu)));
        s_pIntegrationMenu->num_items = 0;
        s_pIntegrationMenu->items = static_cast<rc_client_raintegration_menu_item_t*>(
            rc_buffer_alloc(&s_pIntegrationMenuBuffer, sizeof(rc_client_raintegration_menu_item_t) * vmMenuItems.Count()));

        pMenuItem = s_pIntegrationMenu->items;
        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vmMenuItems.Count()); ++nIndex, ++pMenuItem)
        {
            const auto* pItem = vmMenuItems.GetItemAt(nIndex);
            const auto nId = pItem ? pItem->GetId() : 0;
            if (nId == 0)
            {
                memset(pMenuItem, 0, sizeof(*pMenuItem));
            }
            else
            {
                pMenuItem->label = rc_buffer_strcpy(&s_pIntegrationMenuBuffer, ra::Narrow(pItem->GetLabel()).c_str());
                pMenuItem->id = nId;
                pMenuItem->checked = pItem->IsSelected();
            }

            pMenuItem->enabled = (nId != IDM_RA_FILES_LOGIN);
        }

        s_pIntegrationMenu->num_items = gsl::narrow_cast<uint32_t>(pMenuItem - s_pIntegrationMenu->items);
        return s_pIntegrationMenu;
    }

    static void SyncMenuItem(int nMenuItemId)
    {
        if (!s_pIntegrationMenu)
            return;

        auto* pMenuItem = s_pIntegrationMenu->items;
        const auto* pStop = pMenuItem + s_pIntegrationMenu->num_items;
        while (pMenuItem < pStop && ra::to_signed(pMenuItem->id) != nMenuItemId)
            ++pMenuItem;
        if (pMenuItem == pStop)
            return;

        rc_client_raintegration_event_t pEvent;
        memset(&pEvent, 0, sizeof(pEvent));
        pEvent.menu_item = pMenuItem;

        ra::ui::viewmodels::LookupItemViewModelCollection vmMenuItems;
        ra::ui::viewmodels::IntegrationMenuViewModel::BuildMenu(vmMenuItems);

        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(vmMenuItems.Count()) - 1; nIndex >= 0; --nIndex)
        {
            const auto* pItem = vmMenuItems.GetItemAt(nIndex);
            if (pItem && pItem->GetId() == nMenuItemId)
            {
                const uint8_t checked = pItem->IsSelected() ? 1 : 0;
                if (pMenuItem->checked != checked)
                {
                    pMenuItem->checked = checked;

                    if (s_callbacks.raintegration_event_handler)
                    {
                        pEvent.type = RC_CLIENT_RAINTEGRATION_EVENT_MENUITEM_CHECKED_CHANGED;
                        s_callbacks.raintegration_event_handler(&pEvent, s_callbacks.raintegration_event_client);
                    }
                }

                break;
            }
        }
    }

    static int activate_menu_item(uint32_t nMenuItemId)
    {
        if (nMenuItemId < IDM_RA_MENUSTART || nMenuItemId > IDM_RA_MENUEND)
            return 0;

        if (!s_pIntegrationMenu)
            return 0;

        auto* pMenuItem = s_pIntegrationMenu->items;
        const auto* pStop = pMenuItem + s_pIntegrationMenu->num_items;
        while (pMenuItem < pStop && pMenuItem->id != nMenuItemId)
            ++pMenuItem;
        if (pMenuItem == pStop || !pMenuItem->enabled)
            return 0;

        ra::ui::viewmodels::IntegrationMenuViewModel::ActivateMenuItem(ra::to_signed(nMenuItemId));
        return 1;
    }

    static void RaiseIntegrationEvent(uint32_t nType) noexcept
    {
        if (s_callbacks.raintegration_event_handler)
        {
            rc_client_raintegration_event_t pEvent;
            memset(&pEvent, 0, sizeof(pEvent));
            pEvent.type = nType;

            s_callbacks.raintegration_event_handler(&pEvent, s_callbacks.raintegration_event_client);
        }
    }

    static bool IsExternalRcheevosClient() noexcept
    {
        return s_bIsExternalRcheevosClient;
    }

    static void HookupCallbackEvents()
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.SetPauseFunction(RaisePauseEvent);
        pEmulatorContext.SetResetFunction(RaiseResetEvent);

        s_bIsExternalRcheevosClient = true;
    }

private:
    typedef struct ExternalClientCallbacks
    {
        rc_client_message_callback_t log_callback;
        rc_client_t* log_client;

        rc_client_event_handler_t event_handler;
        rc_client_t* event_client;

        rc_client_read_memory_func_t read_memory_handler;
        rc_client_t* read_memory_client;

        rc_client_raintegration_write_memory_func_t write_memory_handler;
        rc_client_t* write_memory_client;

        rc_client_raintegration_get_game_name_func_t get_game_name_handler;
        rc_client_t* get_game_name_client;

        rc_get_time_millisecs_func_t get_time_millisecs_handler;
        rc_client_t* get_time_millisecs_client;

        rc_client_raintegration_event_handler_t raintegration_event_handler;
        rc_client_t* raintegration_event_client;

    } ExternalClientCallbacks;

    static ExternalClientCallbacks s_callbacks;

    static void LogMessageExternal(const char* sMessage, const rc_client_t*)
    {
        const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
        if (pLogger.IsEnabled(ra::services::LogLevel::Info))
            pLogger.LogMessage(ra::services::LogLevel::Info, sMessage);

        if (s_callbacks.log_callback)
            s_callbacks.log_callback(sMessage, s_callbacks.log_client);
    }

    static void EventHandlerExternal(const rc_client_event_t* event, rc_client_t*) noexcept(false)
    {
        if (s_callbacks.event_handler)
            s_callbacks.event_handler(event, s_callbacks.event_client);
    }

    static uint8_t ReadMemoryByte(uint32_t address) noexcept
    {
        if (s_callbacks.read_memory_handler)
        {
            uint8_t value = 0;

            if (s_callbacks.read_memory_handler(address, &value, 1, s_callbacks.read_memory_client) == 1)
                return value;
        }

        return 0;
    }

    static void WriteMemoryByte(uint32_t address, uint8_t value) noexcept
    {
        if (s_callbacks.write_memory_handler)
            s_callbacks.write_memory_handler(address, &value, 1, s_callbacks.write_memory_client);
    }

    static uint32_t ReadMemoryBlock(uint32_t address, uint8_t* buffer, uint32_t num_bytes) noexcept(false)
    {
        if (s_callbacks.read_memory_handler)
            return s_callbacks.read_memory_handler(address, buffer, num_bytes, s_callbacks.read_memory_client);

        return 0;
    }

    static uint32_t ReadMemoryExternal(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t*)  noexcept(false)
    {
        if (s_callbacks.read_memory_handler)
            return s_callbacks.read_memory_handler(address, buffer, num_bytes, s_callbacks.read_memory_client);

        return 0;
    }
    
    static void RaisePauseEvent() noexcept
    {
        RaiseIntegrationEvent(RC_CLIENT_RAINTEGRATION_EVENT_PAUSE);
    }

    static void RaiseResetEvent() noexcept
    {
        if (s_callbacks.event_handler)
        {
            rc_client_event_t client_event;
            memset(&client_event, 0, sizeof(client_event));
            client_event.type = RC_CLIENT_EVENT_RESET;

            s_callbacks.event_handler(&client_event, s_callbacks.event_client);
        }
    }

    static void GetGameTitle(char buffer[])
    {
        Expects(buffer != nullptr);
        buffer[0] = '\0';

        if (s_callbacks.get_game_name_handler)
            s_callbacks.get_game_name_handler(buffer, 256, s_callbacks.raintegration_event_client);
    }

    static rc_clock_t GetTimeMillisecsExternal(const rc_client_t*) noexcept(false)
    {
        if (s_callbacks.get_time_millisecs_handler)
            return s_callbacks.get_time_millisecs_handler(s_callbacks.get_time_millisecs_client);

        return 0;
    }

    static void destroy_achievement_list(rc_client_achievement_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }

    static void destroy_leaderboard_list(rc_client_leaderboard_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }

    static void destroy_leaderboard_entry_list(rc_client_leaderboard_entry_list_info_t* list) noexcept
    {
        if (list)
            free(list);
    }

    static bool s_bIsExternalRcheevosClient;
    static bool s_bUpdatingHardcore;

    static rc_buffer_t s_pIntegrationMenuBuffer;
    static rc_client_raintegration_menu_t* s_pIntegrationMenu;
};

AchievementRuntimeExports::ExternalClientCallbacks AchievementRuntimeExports::s_callbacks{};
bool AchievementRuntimeExports::s_bIsExternalRcheevosClient = false;
bool AchievementRuntimeExports::s_bUpdatingHardcore = false;
rc_client_raintegration_menu_t* AchievementRuntimeExports::s_pIntegrationMenu = nullptr;
rc_buffer_t AchievementRuntimeExports::s_pIntegrationMenuBuffer{};

} // namespace services
} // namespace ra

void ResetExternalRcheevosClient() noexcept
{
    ra::services::AchievementRuntimeExports::destroy();
}

bool IsExternalRcheevosClient() noexcept
{
    return ra::services::AchievementRuntimeExports::IsExternalRcheevosClient();
}

void ResetEmulatorMemoryRegionsForRcheevosClient()
{
    ra::services::AchievementRuntimeExports::ResetMemory();
}

void SyncClientExternalRAIntegrationMenuItem(int nMenuItemId)
{
    ra::services::AchievementRuntimeExports::SyncMenuItem(nMenuItemId);
}

void SyncClientExternalHardcoreState()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const int bHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore) ? 1 : 0;

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    if (rc_client_get_hardcore_enabled(pRuntime.GetClient()) != bHardcore)
    {
        rc_client_set_hardcore_enabled(pRuntime.GetClient(), bHardcore);

        if (!ra::services::AchievementRuntimeExports::IsUpdatingHardcore())
            ra::services::AchievementRuntimeExports::RaiseIntegrationEvent(RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED);
    }
}

static void GetExternalClientV1(rc_client_external_t* pClientExternal) noexcept
{
    pClientExternal->destroy = ra::services::AchievementRuntimeExports::destroy;

    pClientExternal->enable_logging = ra::services::AchievementRuntimeExports::enable_logging;
    pClientExternal->set_event_handler = ra::services::AchievementRuntimeExports::set_event_handler;
    pClientExternal->set_read_memory = ra::services::AchievementRuntimeExports::set_read_memory;
    pClientExternal->set_get_time_millisecs = ra::services::AchievementRuntimeExports::set_get_time_millisecs;
    pClientExternal->set_host = ra::services::AchievementRuntimeExports::set_host;
    pClientExternal->get_user_agent_clause = ra::services::AchievementRuntimeExports::get_user_agent_clause;

    pClientExternal->set_hardcore_enabled = ra::services::AchievementRuntimeExports::set_hardcore_enabled;
    pClientExternal->get_hardcore_enabled = ra::services::AchievementRuntimeExports::get_hardcore_enabled;
    pClientExternal->set_unofficial_enabled = ra::services::AchievementRuntimeExports::set_unofficial_enabled;
    pClientExternal->get_unofficial_enabled = ra::services::AchievementRuntimeExports::get_unofficial_enabled;
    pClientExternal->set_encore_mode_enabled = ra::services::AchievementRuntimeExports::set_encore_mode_enabled;
    pClientExternal->get_encore_mode_enabled = ra::services::AchievementRuntimeExports::get_encore_mode_enabled;
    pClientExternal->set_spectator_mode_enabled = ra::services::AchievementRuntimeExports::set_spectator_mode_enabled;
    pClientExternal->get_spectator_mode_enabled = ra::services::AchievementRuntimeExports::get_spectator_mode_enabled;

    pClientExternal->abort_async = ra::services::AchievementRuntimeExports::abort_async;

    pClientExternal->begin_login_with_password = ra::services::AchievementRuntimeExports::begin_login_with_password;
    pClientExternal->begin_login_with_token = ra::services::AchievementRuntimeExports::begin_login_with_token;
    pClientExternal->logout = ra::services::AchievementRuntimeExports::logout;
    pClientExternal->get_user_info = ra::services::AchievementRuntimeExports::get_user_info;

    pClientExternal->begin_identify_and_load_game = ra::services::AchievementRuntimeExports::begin_identify_and_load_game;
    pClientExternal->begin_load_game = ra::services::AchievementRuntimeExports::begin_load_game;
    pClientExternal->get_game_info = ra::services::AchievementRuntimeExports::get_game_info;
    pClientExternal->get_subset_info = ra::services::AchievementRuntimeExports::get_subset_info;
    pClientExternal->unload_game = ra::services::AchievementRuntimeExports::unload_game;
    pClientExternal->get_user_game_summary = ra::services::AchievementRuntimeExports::get_user_game_summary;
    pClientExternal->begin_change_media = ra::services::AchievementRuntimeExports::begin_change_media;

    pClientExternal->create_achievement_list = ra::services::AchievementRuntimeExports::create_achievement_list;
    pClientExternal->has_achievements = ra::services::AchievementRuntimeExports::has_achievements;
    pClientExternal->get_achievement_info = ra::services::AchievementRuntimeExports::get_achievement_info;

    pClientExternal->create_leaderboard_list = ra::services::AchievementRuntimeExports::create_leaderboard_list;
    pClientExternal->has_leaderboards = ra::services::AchievementRuntimeExports::has_leaderboards;
    pClientExternal->get_leaderboard_info = ra::services::AchievementRuntimeExports::get_leaderboard_info;
    pClientExternal->begin_fetch_leaderboard_entries = ra::services::AchievementRuntimeExports::begin_fetch_leaderboard_entries;
    pClientExternal->begin_fetch_leaderboard_entries_around_user =
        ra::services::AchievementRuntimeExports::begin_fetch_leaderboard_entries_around_user;

    pClientExternal->get_rich_presence_message = ra::services::AchievementRuntimeExports::get_rich_presence_message;
    pClientExternal->has_rich_presence = ra::services::AchievementRuntimeExports::has_rich_presence;

    pClientExternal->do_frame = ra::services::AchievementRuntimeExports::do_frame;
    pClientExternal->idle = ra::services::AchievementRuntimeExports::idle;
    pClientExternal->is_processing_required = ra::services::AchievementRuntimeExports::is_processing_required;
    pClientExternal->can_pause = ra::services::AchievementRuntimeExports::can_pause;
    pClientExternal->reset = ra::services::AchievementRuntimeExports::reset;

    pClientExternal->progress_size = ra::services::AchievementRuntimeExports::progress_size;
    pClientExternal->serialize_progress = ra::services::AchievementRuntimeExports::serialize_progress;
    pClientExternal->deserialize_progress = ra::services::AchievementRuntimeExports::deserialize_progress;
}

#ifdef __cplusplus
extern "C" {
#endif

API int CCONV _Rcheevos_GetExternalClient(rc_client_external_t* pClientExternal, int nVersion)
{
    switch (nVersion)
    {
        default:
            RA_LOG_WARN("Unknown rc_client_external interface version: %s", nVersion);
            __fallthrough;

        case 1:
            GetExternalClientV1(pClientExternal);
            break;
    }

    ra::services::AchievementRuntimeExports::HookupCallbackEvents();

    return 1;
}

API const rc_client_raintegration_menu_t* CCONV _Rcheevos_RAIntegrationGetMenu()
{
    return ra::services::AchievementRuntimeExports::get_raintegration_menu();
}

API int CCONV _Rcheevos_ActivateRAIntegrationMenuItem(uint32_t nId)
{
    return ra::services::AchievementRuntimeExports::activate_menu_item(nId);
}

API void CCONV _Rcheevos_SetRAIntegrationWriteMemoryFunction(rc_client_t* client, rc_client_raintegration_write_memory_func_t handler)
{
    ra::services::AchievementRuntimeExports::set_raintegration_write_memory_function(client, handler);
}

API void CCONV _Rcheevos_SetRAIntegrationGetGameNameFunction(rc_client_t* client, rc_client_raintegration_get_game_name_func_t handler)
{
    ra::services::AchievementRuntimeExports::set_raintegration_get_game_name_function(client, handler);
}

API void CCONV _Rcheevos_SetRAIntegrationEventHandler(rc_client_t* client, rc_client_raintegration_event_handler_t handler)
{
    ra::services::AchievementRuntimeExports::set_raintegration_event_handler(client, handler);
}

API int CCONV _Rcheevos_HasModifications(void)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset && pAsset->IsModified())
        {
            switch (pAsset->GetType())
            {
                case ra::data::models::AssetType::Achievement:
                case ra::data::models::AssetType::Leaderboard:
                    return true;
            }
        }
    }

    return false;
}

API int CCONV _Rcheevos_GetAchievementState(uint32_t nId)
{
    return ra::services::AchievementRuntimeExports::get_achievement_state(nId);
}

#ifdef __cplusplus
} // extern "C"
#endif
