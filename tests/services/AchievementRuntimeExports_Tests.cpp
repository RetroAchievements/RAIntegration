#include "services\AchievementRuntimeExports.hh"

#include "services\AchievementRuntime.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockUserContext.hh"

#include <rcheevos\src\rc_client_external.h>
#include <rcheevos\src\rc_client_internal.h>
#include <rcheevos\include\rc_client_raintegration.h>

#include "Exports.hh"
#include "RA_BuildVer.h"
#include "RA_Resource.h"

// the exported functions are not defined in a header anywhere. the only thing that should normally be
// calling them is rcheevos/rc_client_raintegration, and it binds the function pointers using its own
// definition of the functions.
extern "C" API int CCONV _Rcheevos_GetExternalClient(rc_client_external_t* pClientExternal, int nVersion);
extern "C" API const rc_client_raintegration_menu_t* CCONV _Rcheevos_RAIntegrationGetMenu();
extern "C" API void CCONV _Rcheevos_SetRAIntegrationWriteMemoryFunction(rc_client_t* client, rc_client_raintegration_write_memory_func_t handler);
extern "C" API void CCONV _Rcheevos_SetRAIntegrationEventHandler(rc_client_t* client, rc_client_raintegration_event_handler_t handler);

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

constexpr int ResetEventBit = (1 << 24);

class AchievementRuntimeExportsHarness : public AchievementRuntime
{
public:
    GSL_SUPPRESS_F6 AchievementRuntimeExportsHarness() : m_Override(this)
    {
        mockUserContext.Initialize("User", "ApiToken");
        GetClient()->user.display_name = "UserDisplay";
        GetClient()->state.user = RC_CLIENT_USER_STATE_LOGGED_IN;
    }

    ~AchievementRuntimeExportsHarness()
    {
        ResetExternalRcheevosClient();
    }

    AchievementRuntimeExportsHarness(const AchievementRuntimeExportsHarness&) noexcept = delete;
    AchievementRuntimeExportsHarness& operator=(const AchievementRuntimeExportsHarness&) noexcept = delete;
    AchievementRuntimeExportsHarness(AchievementRuntimeExportsHarness&&) noexcept = delete;
    AchievementRuntimeExportsHarness& operator=(AchievementRuntimeExportsHarness&&) noexcept = delete;

    ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
    ra::data::context::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockConfiguration mockConfiguration;

    void InitializeEventHandler() noexcept
    {
        s_pRuntimeHarness = this;

        _Rcheevos_SetRAIntegrationEventHandler(&m_pExternalClient, DispatchRAIntegrationEvent);
    }

    void InitializeClientEventHandler() noexcept
    {
        s_pRuntimeHarness = this;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 1);

        pClient.set_event_handler(&m_pExternalClient, DispatchEvent);
    }

    void AssertHardcoreChangedEventSeen() const
    {
        Assert::IsTrue(m_nEventsSeen & (1 << RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED),
                       L"HARDCORE event not seen");
    }

    void AssertPauseEventSeen() const
    {
        Assert::IsTrue(m_nEventsSeen & (1 << RC_CLIENT_RAINTEGRATION_EVENT_PAUSE),
                       L"PAUSE event not seen");
    }

    void AssertMenuChangedEventSeen() const
    {
        Assert::IsTrue(m_nEventsSeen & (1 << RC_CLIENT_RAINTEGRATION_EVENT_MENU_CHANGED),
                       L"MENU_CHANGED event not seen");
    }

    void AssertResetEventSeen() const
    {
        Assert::IsTrue(m_nEventsSeen & ResetEventBit, L"RESET event not seen");
    }

    void AssertMenuItemCheckedChangedEventSeen(uint32_t nMenuItemId) const
    {
        Assert::IsTrue(m_nEventsSeen & (1 << RC_CLIENT_RAINTEGRATION_EVENT_MENUITEM_CHECKED_CHANGED),
                       L"MENUITEM event not seen");

        for (const auto nChangedMenuItemId : m_vMenuItemsChanged)
        {
            if (nChangedMenuItemId == nMenuItemId)
                return;
        }
        Assert::Fail(ra::StringPrintf(L"MENUITEM check changed event not seen for %d", nMenuItemId).c_str());
    }

    void ResetSeenEvents() noexcept
    {
        m_nEventsSeen = 0;
        m_vMenuItemsChanged.clear();
    }

    void InitializeMemoryFunctions() noexcept
    {
        s_pRuntimeHarness = this;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 1);

        pClient.set_read_memory(&m_pExternalClient, DispatchReadMemory);

        _Rcheevos_SetRAIntegrationWriteMemoryFunction(&m_pExternalClient, DispatchWriteMemory);
    }

    uint8_t GetMemoryByte(uint32_t nAddress)
    {
        if (nAddress >= m_vMockMemory.size())
            return 0;

        return m_vMockMemory.at(nAddress);
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;

    static void DispatchEvent(const rc_client_event_t* event, rc_client_t* client)
    {
        Assert::AreEqual((void*)&s_pRuntimeHarness->m_pExternalClient, (void*)client);

        switch (event->type)
        {
            case RC_CLIENT_EVENT_RESET:
                s_pRuntimeHarness->m_nEventsSeen |= ResetEventBit;
                break;
        }
    }

    static void DispatchRAIntegrationEvent(const rc_client_raintegration_event_t* event, rc_client_t* client)
    {
        Assert::AreEqual((void*)&s_pRuntimeHarness->m_pExternalClient, (void*)client);

        switch (event->type)
        {
            case RC_CLIENT_RAINTEGRATION_EVENT_MENUITEM_CHECKED_CHANGED:
                s_pRuntimeHarness->m_vMenuItemsChanged.push_back(event->menu_item->id);
                break;
        }

        s_pRuntimeHarness->m_nEventsSeen |= (1 << event->type);
    }

    static uint32_t DispatchReadMemory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
    {
        Assert::AreEqual((void*)&s_pRuntimeHarness->m_pExternalClient, (void*)client);

        auto nBytesAvailable = (s_pRuntimeHarness->m_vMockMemory.size() > address) ?
            gsl::narrow_cast<uint32_t>(s_pRuntimeHarness->m_vMockMemory.size() - address) : 0;

        if (nBytesAvailable > num_bytes)
            nBytesAvailable = num_bytes;

        if (nBytesAvailable > 0)
            memcpy(buffer, &s_pRuntimeHarness->m_vMockMemory.at(address), nBytesAvailable);

        return nBytesAvailable;
    }

    GSL_SUPPRESS_CON3
    static void DispatchWriteMemory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
    {
        Assert::AreEqual((void*)&s_pRuntimeHarness->m_pExternalClient, (void*)client);

        const auto nNextAddress = gsl::narrow_cast<size_t>(address + num_bytes + 1);
        if (s_pRuntimeHarness->m_vMockMemory.size() < nNextAddress)
            s_pRuntimeHarness->m_vMockMemory.resize(nNextAddress);

        memcpy(&s_pRuntimeHarness->m_vMockMemory.at(address), buffer, num_bytes);
    }

    static AchievementRuntimeExportsHarness* s_pRuntimeHarness;
    rc_client_t m_pExternalClient{};
    int m_nEventsSeen = 0;
    std::vector<uint32_t> m_vMenuItemsChanged;
    std::vector<uint8_t> m_vMockMemory;
};

AchievementRuntimeExportsHarness* AchievementRuntimeExportsHarness::s_pRuntimeHarness = nullptr;

TEST_CLASS(AchievementRuntimeExports_Tests)
{
private:
    static void AssertMenuItem(const rc_client_raintegration_menu_t* pMenu, int nIndex, uint32_t nId, const char* label)
    {
        const auto* pItem = &pMenu->items[nIndex];
        Assert::AreEqual(nId, pItem->id);
        Assert::AreEqual(std::string(label), std::string(pItem->label));
        Assert::AreEqual({1}, pItem->enabled);
        Assert::AreEqual({0}, pItem->checked);
    };

    static void AssertMenuItemChecked(const rc_client_raintegration_menu_t* pMenu, int nIndex, uint32_t nId, const char* label)
    {
        const auto* pItem = &pMenu->items[nIndex];
        Assert::AreEqual(nId, pItem->id);
        Assert::AreEqual(std::string(label), std::string(pItem->label));
        Assert::AreEqual({1}, pItem->enabled);
        Assert::AreEqual({1}, pItem->checked);
    };

    static void AssertMenuSeparator(const rc_client_raintegration_menu_t* pMenu, int nIndex)
    {
        const auto* pItem = &pMenu->items[nIndex];
        Assert::AreEqual(0U, pItem->id);
        Assert::IsNull(pItem->label);
        Assert::AreEqual({1}, pItem->enabled);
        Assert::AreEqual({0}, pItem->checked);
    };

    GSL_SUPPRESS_TYPE4
    static void AssertV1Exports(const rc_client_external_t& pClient)
    {
        Assert::IsNotNull((void*)pClient.destroy, L"destory not set");

        Assert::IsNotNull((void*)pClient.enable_logging, L"enable_logging not set");
        Assert::IsNotNull((void*)pClient.set_event_handler, L"set_event_handler not set");
        Assert::IsNotNull((void*)pClient.set_read_memory, L"set_read_memory not set");
        Assert::IsNotNull((void*)pClient.set_get_time_millisecs, L"set_get_time_millisecs not set");
        Assert::IsNotNull((void*)pClient.set_host, L"set_host not set");
        Assert::IsNotNull((void*)pClient.get_user_agent_clause, L"set_host not set");

        Assert::IsNotNull((void*)pClient.set_hardcore_enabled, L"set_hardcore_enabled not set");
        Assert::IsNotNull((void*)pClient.get_hardcore_enabled, L"get_hardcore_enabled not set");
        Assert::IsNotNull((void*)pClient.set_unofficial_enabled, L"set_unofficial_enabled not set");
        Assert::IsNotNull((void*)pClient.get_unofficial_enabled, L"get_unofficial_enabled not set");
        Assert::IsNotNull((void*)pClient.set_encore_mode_enabled, L"set_encore_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.get_encore_mode_enabled, L"get_encore_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.set_spectator_mode_enabled, L"set_spectator_mode_enabled not set");
        Assert::IsNotNull((void*)pClient.get_spectator_mode_enabled, L"get_spectator_mode_enabled not set");

        Assert::IsNotNull((void*)pClient.abort_async, L"abort_async not set");

        Assert::IsNotNull((void*)pClient.begin_login_with_password, L"begin_login_with_password not set");
        Assert::IsNotNull((void*)pClient.begin_login_with_token, L"begin_login_with_token not set");
        Assert::IsNotNull((void*)pClient.logout, L"logout not set");
        Assert::IsNotNull((void*)pClient.get_user_info, L"get_user_info not set");

        Assert::IsNotNull((void*)pClient.begin_identify_and_load_game, L"begin_identify_and_load_game not set");
        Assert::IsNotNull((void*)pClient.begin_load_game, L"begin_load_game not set");
        Assert::IsNotNull((void*)pClient.get_game_info, L"get_game_info not set");
        Assert::IsNull((void*)pClient.begin_load_subset, L"begin_load_subset set");
        Assert::IsNotNull((void*)pClient.get_subset_info, L"get_subset_info not set");
        Assert::IsNotNull((void*)pClient.unload_game, L"unload_game not set");
        Assert::IsNotNull((void*)pClient.get_user_game_summary, L"get_user_game_summary not set");
        Assert::IsNotNull((void*)pClient.begin_change_media, L"begin_change_media not set");

        Assert::IsNotNull((void*)pClient.create_achievement_list, L"create_achievement_list not set");
        Assert::IsNotNull((void*)pClient.has_achievements, L"has_achievements not set");
        Assert::IsNotNull((void*)pClient.get_achievement_info, L"get_achievement_info not set");

        Assert::IsNotNull((void*)pClient.create_leaderboard_list, L"create_leaderboard_list not set");
        Assert::IsNotNull((void*)pClient.has_leaderboards, L"has_leaderboards not set");
        Assert::IsNotNull((void*)pClient.get_leaderboard_info, L"get_leaderboard_info not set");
        Assert::IsNotNull((void*)pClient.begin_fetch_leaderboard_entries, L"begin_fetch_leaderboard_entries not set");
        Assert::IsNotNull((void*)pClient.begin_fetch_leaderboard_entries_around_user, L"begin_fetch_leaderboard_entries_around_user not set");

        Assert::IsNotNull((void*)pClient.get_rich_presence_message, L"get_rich_presence_message not set");
        Assert::IsNotNull((void*)pClient.has_rich_presence, L"has_rich_presence not set");

        Assert::IsNotNull((void*)pClient.do_frame, L"do_frame not set");
        Assert::IsNotNull((void*)pClient.idle, L"idle not set");
        Assert::IsNotNull((void*)pClient.is_processing_required, L"is_processing_required not set");
        Assert::IsNotNull((void*)pClient.can_pause, L"can_pause not set");
        Assert::IsNotNull((void*)pClient.reset, L"reset not set");

        Assert::IsNotNull((void*)pClient.progress_size, L"progress_size not set");
        Assert::IsNotNull((void*)pClient.serialize_progress, L"serialize_progress not set");
        Assert::IsNotNull((void*)pClient.deserialize_progress, L"deserialize_progress not set");
    }

    GSL_SUPPRESS_TYPE4
        static void AssertV2Exports(const rc_client_external_t & pClient)
    {
        Assert::IsNotNull((void*)pClient.add_game_hash, L"add_game_hash not set");
        Assert::IsNotNull((void*)pClient.load_unknown_game, L"load_unknown_game not set");
    }

    GSL_SUPPRESS_TYPE4
        static void AssertV3Exports(const rc_client_external_t & pClient)
    {
        Assert::IsNotNull((void*)pClient.get_user_info_v3, L"get_user_info_v3 not set");
        Assert::IsNotNull((void*)pClient.get_game_info_v3, L"get_game_info_v3 not set");
        Assert::IsNotNull((void*)pClient.get_subset_info_v3, L"get_subset_info_v3 not set");
        Assert::IsNotNull((void*)pClient.get_achievement_info_v3, L"get_achievement_info_v3 not set");
        Assert::IsNotNull((void*)pClient.create_achievement_list_v3, L"create_achievement_list_v3 not set");
    }

public:
    TEST_METHOD(TestGetExternalClientV1)
    {
        AchievementRuntimeExportsHarness runtime;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));

        _Rcheevos_GetExternalClient(&pClient, 1);

        AssertV1Exports(pClient);

        Assert::IsTrue(IsExternalRcheevosClient());
    }

    TEST_METHOD(TestGetExternalClientV2)
    {
        AchievementRuntimeExportsHarness runtime;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));

        _Rcheevos_GetExternalClient(&pClient, 2);

        AssertV1Exports(pClient);
        AssertV2Exports(pClient);

        Assert::IsTrue(IsExternalRcheevosClient());
    }

    TEST_METHOD(TestGetExternalClientV3)
    {
        AchievementRuntimeExportsHarness runtime;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));

        _Rcheevos_GetExternalClient(&pClient, 3);

        AssertV1Exports(pClient);
        AssertV2Exports(pClient);
        AssertV3Exports(pClient);

        Assert::IsTrue(IsExternalRcheevosClient());
    }

    TEST_METHOD(TestRAIntegrationGetMenu)
    {
        AchievementRuntimeExportsHarness runtime;
        runtime.mockUserContext.Logout();
        runtime.InitializeEventHandler();

        const rc_client_raintegration_menu_t* pMenu;

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(13U, pMenu->num_items);
        AssertMenuItem(pMenu, 0, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItem(pMenu, 1, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(pMenu, 2);
        AssertMenuItem(pMenu, 3, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(pMenu, 4, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(pMenu, 5, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(pMenu, 6, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(pMenu, 7, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(pMenu, 8, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(pMenu, 9, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");
        AssertMenuSeparator(pMenu, 10);
        AssertMenuItem(pMenu, 11, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(pMenu, 12, IDM_RA_FILES_POINTERINSPECTOR, "Pointer &Inspector");

        runtime.mockUserContext.Initialize("User", "ApiToken");
        runtime.AssertMenuChangedEventSeen();
        runtime.ResetSeenEvents();

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(19U, pMenu->num_items);
        AssertMenuItem(pMenu, 0, IDM_RA_OPENUSERPAGE, "Open my &User Page");
        AssertMenuItem(pMenu, 1, IDM_RA_OPENGAMEPAGE, "Open this &Game's Page");
        AssertMenuSeparator(pMenu, 2);
        AssertMenuItem(pMenu, 3, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItem(pMenu, 4, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(pMenu, 5);
        AssertMenuItem(pMenu, 6, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(pMenu, 7, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(pMenu, 8, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(pMenu, 9, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(pMenu, 10, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(pMenu, 11, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(pMenu, 12, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");
        AssertMenuSeparator(pMenu, 13);
        AssertMenuItem(pMenu, 14, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(pMenu, 15, IDM_RA_FILES_POINTERINSPECTOR, "Pointer &Inspector");
        AssertMenuSeparator(pMenu, 16);
        AssertMenuItem(pMenu, 17, IDM_RA_REPORTBROKENACHIEVEMENTS, "&Report Achievement Problem");
        AssertMenuItem(pMenu, 18, IDM_RA_GETROMCHECKSUM, "View Game H&ash");

        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        Assert::AreEqual(19U, pMenu->num_items);
        AssertMenuItem(pMenu, 0, IDM_RA_OPENUSERPAGE, "Open my &User Page");
        AssertMenuItem(pMenu, 1, IDM_RA_OPENGAMEPAGE, "Open this &Game's Page");
        AssertMenuSeparator(pMenu, 2);
        AssertMenuItemChecked(pMenu, 3, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");
        AssertMenuItemChecked(pMenu, 4, IDM_RA_NON_HARDCORE_WARNING, "Non-Hardcore &Warning");
        AssertMenuSeparator(pMenu, 5);
        AssertMenuItem(pMenu, 6, IDM_RA_FILES_OPENALL, "&Open All");
        AssertMenuItem(pMenu, 7, IDM_RA_FILES_ACHIEVEMENTS, "Assets Li&st");
        AssertMenuItem(pMenu, 8, IDM_RA_FILES_ACHIEVEMENTEDITOR, "Assets &Editor");
        AssertMenuItem(pMenu, 9, IDM_RA_FILES_MEMORYFINDER, "&Memory Inspector");
        AssertMenuItem(pMenu, 10, IDM_RA_FILES_MEMORYBOOKMARKS, "Memory &Bookmarks");
        AssertMenuItem(pMenu, 11, IDM_RA_FILES_CODENOTES, "Code &Notes");
        AssertMenuItem(pMenu, 12, IDM_RA_PARSERICHPRESENCE, "Rich &Presence Monitor");
        AssertMenuSeparator(pMenu, 13);
        AssertMenuItem(pMenu, 14, IDM_RA_FILES_POINTERFINDER, "Pointer &Finder");
        AssertMenuItem(pMenu, 15, IDM_RA_FILES_POINTERINSPECTOR, "Pointer &Inspector");
        AssertMenuSeparator(pMenu, 16);
        AssertMenuItem(pMenu, 17, IDM_RA_REPORTBROKENACHIEVEMENTS, "&Report Achievement Problem");
        AssertMenuItem(pMenu, 18, IDM_RA_GETROMCHECKSUM, "View Game H&ash");
    }

    
    TEST_METHOD(TestSyncHardcore)
    {
        AchievementRuntimeExportsHarness runtime;
        runtime.InitializeEventHandler();
        runtime.GetClient()->state.hardcore = 0;

        const rc_client_raintegration_menu_t* pMenu;

        pMenu = _Rcheevos_RAIntegrationGetMenu();
        AssertMenuItem(pMenu, 3, IDM_RA_HARDCORE_MODE, "&Hardcore Mode");

        runtime.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        SyncClientExternalRAIntegrationMenuItem(IDM_RA_HARDCORE_MODE);
        SyncClientExternalHardcoreState();

        runtime.AssertHardcoreChangedEventSeen();
        runtime.AssertMenuItemCheckedChangedEventSeen(IDM_RA_HARDCORE_MODE);
    }

    TEST_METHOD(TestReadWriteMemory)
    {
        AchievementRuntimeExportsHarness runtime;
        ra::data::context::mocks::MockConsoleContext mockConsole(NES, L"NES");
        runtime.InitializeMemoryFunctions();

        Assert::AreEqual({0}, runtime.GetMemoryByte(1));
        Assert::AreEqual({0}, runtime.mockEmulatorContext.ReadMemoryByte(1));

        runtime.mockEmulatorContext.WriteMemoryByte(1, 3);

        Assert::AreEqual({3}, runtime.GetMemoryByte(1));
        Assert::AreEqual({3}, runtime.mockEmulatorContext.ReadMemoryByte(1));

        runtime.mockEmulatorContext.WriteMemoryByte(2, 7);
        Assert::AreEqual({7}, runtime.GetMemoryByte(2));
        Assert::AreEqual({7}, runtime.mockEmulatorContext.ReadMemoryByte(2));
        Assert::AreEqual({0x0703}, runtime.mockEmulatorContext.ReadMemory(1, MemSize::SixteenBit));
    }

    TEST_METHOD(TestPauseEvent)
    {
        AchievementRuntimeExportsHarness runtime;
        ra::ui::mocks::MockDesktop mockDesktop; // for InvokeOnUIThread
        runtime.InitializeEventHandler();

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 1);

        runtime.mockEmulatorContext.Pause();

        runtime.AssertPauseEventSeen();
    }

    TEST_METHOD(TestResetEvent)
    {
        AchievementRuntimeExportsHarness runtime;
        ra::ui::mocks::MockDesktop mockDesktop; // for InvokeOnUIThread
        runtime.InitializeClientEventHandler();

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 1);

        runtime.mockEmulatorContext.Reset();

        runtime.AssertResetEventSeen();
    }

    TEST_METHOD(TestUserAgentClause)
    {
        AchievementRuntimeExportsHarness runtime;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 1);

        char buffer[32];
        Assert::IsTrue(pClient.get_user_agent_clause(buffer, 8) > 8);
        Assert::AreEqual("Integra", buffer);

        pClient.get_user_agent_clause(buffer, 32);
        char* ptr = strchr(buffer, '-');
        if (ptr != nullptr)
            *ptr = '\0';
        Assert::AreEqual("Integration/" RA_INTEGRATION_VERSION, buffer);
    }

    TEST_METHOD(TestLoadUnknownGame)
    {
        AchievementRuntimeExportsHarness runtime;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 2);

        pClient.load_unknown_game("ABCDEF0123456789");

        const auto* pGame = rc_client_get_game_info(runtime.GetClient());
        Assert::IsNotNull(pGame);
        Ensures(pGame != nullptr);
        Assert::AreEqual(0U, pGame->id);
        Assert::AreEqual("Unknown Game", pGame->title);
        Assert::AreEqual(0U, pGame->console_id);

        Assert::AreEqual(std::string("ABCDEF0123456789"), mockGameContext.GameHash());
    }

    TEST_METHOD(TestLoadUnknownGameWithConsole)
    {
        AchievementRuntimeExportsHarness runtime;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;

        rc_client_external_t pClient;
        memset(&pClient, 0, sizeof(pClient));
        _Rcheevos_GetExternalClient(&pClient, 2);

        mockConsoleContext.SetId(ConsoleID::GBC);
        pClient.load_unknown_game("ABCDEF0123456789");

        const auto* pGame = rc_client_get_game_info(runtime.GetClient());
        Assert::IsNotNull(pGame);
        Ensures(pGame != nullptr);
        Assert::AreEqual(0U, pGame->id);
        Assert::AreEqual("Unknown Game", pGame->title);
        Assert::AreEqual({ RC_CONSOLE_GAMEBOY_COLOR }, pGame->console_id);

        Assert::AreEqual(std::string("ABCDEF0123456789"), mockGameContext.GameHash());
    }
};

} // namespace tests
} // namespace services
} // namespace ra
