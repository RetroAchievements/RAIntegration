#include "CppUnitTest.h"

#include "Exports.hh"

#include "RA_BuildVer.h"
#include "RA_Emulators.h"
#include "RA_Resource.h"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockOverlayTheme.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockSurface.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"
#include "tests\ui\UIAsserts.hh"

#include "services\AchievementRuntime.hh"

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::mocks::MockServer;
using ra::data::context::mocks::MockEmulatorContext;
using ra::data::context::mocks::MockGameContext;
using ra::data::context::mocks::MockSessionTracker;
using ra::data::context::mocks::MockUserContext;
using ra::services::mocks::MockAchievementRuntime;
using ra::services::mocks::MockAudioSystem;
using ra::services::mocks::MockClock;
using ra::services::mocks::MockConfiguration;
using ra::services::mocks::MockFrameEventQueue;
using ra::services::mocks::MockThreadPool;
using ra::ui::mocks::MockDesktop;
using ra::ui::mocks::MockOverlayTheme;
using ra::ui::drawing::mocks::MockSurfaceFactory;
using ra::ui::viewmodels::MessageBoxViewModel;
using ra::ui::viewmodels::mocks::MockOverlayManager;
using ra::ui::viewmodels::mocks::MockWindowManager;

namespace ra {
namespace tests {

TEST_CLASS(Exports_Tests)
{
public:
    TEST_METHOD(TestIntegrationVersion)
    {
        Assert::AreEqual(RA_INTEGRATION_VERSION, _RA_IntegrationVersion());
    }

    TEST_METHOD(TestHostName)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual("retroachievements.org", _RA_HostName());

        mockConfiguration.SetHostName("stage.retroachievements.org");
        Assert::AreEqual("stage.retroachievements.org", _RA_HostName());
    }

    TEST_METHOD(TestHostUrl)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual("http://retroachievements.org", _RA_HostUrl());

        mockConfiguration.SetHostName("stage.retroachievements.org");
        Assert::AreEqual("http://stage.retroachievements.org", _RA_HostUrl());
    }

    TEST_METHOD(TestHardcoreModeIsActive)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(1, _RA_HardcoreModeIsActive());

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(0, _RA_HardcoreModeIsActive());
    }

private:
    class AttemptLoginHarness
    {
    public:
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockAchievementRuntime mockRcheevosClient;
        MockAudioSystem mockAudioSystem;
        MockServer mockServer;
        MockOverlayManager mockOverlayManager;
        MockEmulatorContext mockEmulatorContext;
        MockDesktop mockDesktop;
        MockThreadPool mockThreadPool;
        MockWindowManager mockWindowManager;
    };

public:

    TEST_METHOD(TestAttemptLoginNoUser)
    {
        AttemptLoginHarness harness;

        bool bLoginDialogShown = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bLoginDialogShown](_UNUSED ra::ui::viewmodels::LoginViewModel&)
        {
            bLoginDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsTrue(bLoginDialogShown);

        harness.mockRcheevosClient.AssertNoPendingRequests();
    }

    TEST_METHOD(TestAttemptLoginSuccess)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });
        harness.mockEmulatorContext.MockClient("RATests", "0.1.2.0");
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        // user context
        Assert::IsTrue(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string("User"), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("UserDisplay"), harness.mockUserContext.GetDisplayName());
        Assert::AreEqual(std::string("ApiToken"), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, harness.mockUserContext.GetScore());

        // session context
        Assert::AreEqual(std::wstring(L"User"), harness.mockSessionTracker.GetUsername());

        // popup notification and sound
        Assert::IsTrue(harness.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\login.wav"));
        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome UserDisplay"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"12345 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());

        // menu
        Assert::IsTrue(bWasMenuRebuilt);

        // titlebar (should not be updated unless _RA_UpdateAppTitle was called first)
        Assert::AreEqual(std::wstring(L"Window"), harness.mockWindowManager.Emulator.GetWindowTitle());
    }

    TEST_METHOD(TestAttemptLoginSuccessTitlebar)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });
        harness.mockEmulatorContext.MockClient("RATests", "0.1.2.0");
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());

        _RA_UpdateAppTitle(""); // enable automatic update of titlebar
        _RA_AttemptLogin(true);

        // user context
        Assert::IsTrue(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string("User"), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("UserDisplay"), harness.mockUserContext.GetDisplayName());
        Assert::AreEqual(std::string("ApiToken"), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, harness.mockUserContext.GetScore());

        // session context
        Assert::AreEqual(std::wstring(L"User"), harness.mockSessionTracker.GetUsername());

        // popup notification and sound
        Assert::IsTrue(harness.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\login.wav"));
        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome UserDisplay"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"12345 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());

        // menu
        Assert::IsTrue(bWasMenuRebuilt);

        // titlebar
        Assert::AreEqual(std::wstring(L"RATests - 0.1 - UserDisplay []"), harness.mockWindowManager.Emulator.GetWindowTitle());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithMessages)
    {
        AttemptLoginHarness harness;
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":0,\"SoftcoreScore\":0,"
            "\"Messages\":3,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome UserDisplay"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have 3 new messages"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithPreviousSessionDataHardcore)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        harness.mockSessionTracker.MockSession(6U, 123456789, std::chrono::hours(2));

        _RA_AttemptLogin(true);

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome back UserDisplay"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"12345 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
        Assert::IsTrue(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginSuccessWithPreviousSessionDataSoftcore)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        harness.mockSessionTracker.MockSession(6U, 123456789, std::chrono::hours(2));

        _RA_AttemptLogin(true);

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome back UserDisplay"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"123 points (softcore)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
        Assert::IsTrue(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginInvalid)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Login Failed"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Invalid user/password combination. Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());

            return ra::ui::DialogResult::OK;
        });

        harness.mockRcheevosClient.MockResponse("r=login2&u=User&t=ApiToken",
            "{\"Success\":false,\"Error\":\"Invalid user/password combination. Please try again.\"}");

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        Assert::IsTrue(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginDisabled)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        harness.mockUserContext.DisableLogin();
        _RA_AttemptLogin(true);

        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginDisabledDuringRequest)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(false);

        harness.mockUserContext.DisableLogin();
        harness.mockThreadPool.ExecuteNextTask();

        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestUserNameNotLoggedIn)
    {
        MockUserContext mockUserContext;
        Assert::AreEqual(_RA_UserName(), "");
    }

    TEST_METHOD(TestUserNameLoggedIn)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Test", "TestDisplay", "ABCDEFG");
        Assert::AreEqual(_RA_UserName(), "TestDisplay");
    }

private:
    class DoAchievementsFrameHarness
    {
    public:
        MockAchievementRuntime mockRuntime;
        MockGameContext mockGameContext;
        MockDesktop mockDesktop;
        MockOverlayManager mockOverlayManager;
        MockAudioSystem mockAudioSystem;
        MockConfiguration mockConfiguration;
        MockEmulatorContext mockEmulatorContext;
        MockSurfaceFactory mockSurfaceFactory;
        MockOverlayTheme mockTheme;
        MockFrameEventQueue mockFrameEventQueue;
        MockWindowManager mockWindowManager;
        MockClock mockClock;

        ra::data::models::AchievementModel& MockAchievement(unsigned int nId)
        {
            auto& pAch = mockGameContext.Assets().NewAchievement();
            pAch.SetCategory(ra::data::models::AssetCategory::Core);
            pAch.SetID(nId);
            pAch.SetTrigger("1=1");
            pAch.UpdateServerCheckpoint();
            pAch.SetState(ra::data::models::AssetState::Active);
            return pAch;
        }

    private:
        MockThreadPool mockThreadPool;
        MockServer mockServer;
        MockUserContext mockUserContext;
    };

    void AssertMenuItem(const RA_MenuItem* pItem, LPARAM nId, const wchar_t* sLabel, bool bChecked = false)
    {
        Assert::AreEqual((int)nId, (int)pItem->nID);

        if (sLabel == nullptr)
            Assert::IsNull(pItem->sLabel);
        else
            Assert::AreEqual(std::wstring(sLabel), std::wstring(pItem->sLabel));

        Assert::AreEqual(bChecked ? 1 : 0, pItem->bChecked);
    }

    TEST_METHOD(TestGetPopupMenuItemsNotLoggedIn)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::data::context::mocks::MockUserContext mockUserContext;

        RA_MenuItem menu[32];
        Assert::AreEqual(16, _RA_GetPopupMenuItems(menu));
        AssertMenuItem(&menu[0], IDM_RA_FILES_LOGIN, L"&Login");
        AssertMenuItem(&menu[1], 0, nullptr);
        AssertMenuItem(&menu[2], IDM_RA_HARDCORE_MODE, L"&Hardcore Mode");
        AssertMenuItem(&menu[3], IDM_RA_NON_HARDCORE_WARNING, L"Non-Hardcore &Warning");
        AssertMenuItem(&menu[4], 0, nullptr);
        AssertMenuItem(&menu[5], IDM_RA_TOGGLELEADERBOARDS, L"Enable &Leaderboards");
        AssertMenuItem(&menu[6], IDM_RA_OVERLAYSETTINGS, L"O&verlay Settings");
        AssertMenuItem(&menu[7], 0, nullptr);
        AssertMenuItem(&menu[8], IDM_RA_FILES_OPENALL, L"&Open All");
        AssertMenuItem(&menu[9], IDM_RA_FILES_ACHIEVEMENTS, L"Assets Li&st");
        AssertMenuItem(&menu[10], IDM_RA_FILES_ACHIEVEMENTEDITOR, L"Assets &Editor");
        AssertMenuItem(&menu[11], IDM_RA_FILES_MEMORYFINDER, L"&Memory Inspector");
        AssertMenuItem(&menu[12], IDM_RA_FILES_MEMORYBOOKMARKS, L"Memory &Bookmarks");
        AssertMenuItem(&menu[13], IDM_RA_FILES_POINTERFINDER, L"Pointer &Finder");
        AssertMenuItem(&menu[14], IDM_RA_FILES_CODENOTES, L"Code &Notes");
        AssertMenuItem(&menu[15], IDM_RA_PARSERICHPRESENCE, L"Rich &Presence Monitor");
    }

    TEST_METHOD(TestGetPopupMenuItemsLoggedIn)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::data::context::mocks::MockUserContext mockUserContext;
        mockUserContext.Initialize("User", "TOKEN");

        RA_MenuItem menu[32];
        Assert::AreEqual(22, _RA_GetPopupMenuItems(menu));
        AssertMenuItem(&menu[0], IDM_RA_FILES_LOGOUT, L"Log&out");
        AssertMenuItem(&menu[1], 0, nullptr);
        AssertMenuItem(&menu[2], IDM_RA_OPENUSERPAGE, L"Open my &User Page");
        AssertMenuItem(&menu[3], IDM_RA_OPENGAMEPAGE, L"Open this &Game's Page");
        AssertMenuItem(&menu[4], 0, nullptr);
        AssertMenuItem(&menu[5], IDM_RA_HARDCORE_MODE, L"&Hardcore Mode");
        AssertMenuItem(&menu[6], IDM_RA_NON_HARDCORE_WARNING, L"Non-Hardcore &Warning");
        AssertMenuItem(&menu[7], 0, nullptr);
        AssertMenuItem(&menu[8], IDM_RA_TOGGLELEADERBOARDS, L"Enable &Leaderboards");
        AssertMenuItem(&menu[9], IDM_RA_OVERLAYSETTINGS, L"O&verlay Settings");
        AssertMenuItem(&menu[10], 0, nullptr);
        AssertMenuItem(&menu[11], IDM_RA_FILES_OPENALL, L"&Open All");
        AssertMenuItem(&menu[12], IDM_RA_FILES_ACHIEVEMENTS, L"Assets Li&st");
        AssertMenuItem(&menu[13], IDM_RA_FILES_ACHIEVEMENTEDITOR, L"Assets &Editor");
        AssertMenuItem(&menu[14], IDM_RA_FILES_MEMORYFINDER, L"&Memory Inspector");
        AssertMenuItem(&menu[15], IDM_RA_FILES_MEMORYBOOKMARKS, L"Memory &Bookmarks");
        AssertMenuItem(&menu[16], IDM_RA_FILES_POINTERFINDER, L"Pointer &Finder");
        AssertMenuItem(&menu[17], IDM_RA_FILES_CODENOTES, L"Code &Notes");
        AssertMenuItem(&menu[18], IDM_RA_PARSERICHPRESENCE, L"Rich &Presence Monitor");
        AssertMenuItem(&menu[19], 0, nullptr);
        AssertMenuItem(&menu[20], IDM_RA_REPORTBROKENACHIEVEMENTS, L"&Report Achievement Problem");
        AssertMenuItem(&menu[21], IDM_RA_GETROMCHECKSUM, L"View Game H&ash");
    }

    TEST_METHOD(TestGetPopupMenuItemsChecked)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::data::context::mocks::MockUserContext mockUserContext;
        mockUserContext.Initialize("User", "TOKEN");
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);

        RA_MenuItem menu[32];
        Assert::AreEqual(22, _RA_GetPopupMenuItems(menu));
        AssertMenuItem(&menu[0], IDM_RA_FILES_LOGOUT, L"Log&out");
        AssertMenuItem(&menu[1], 0, nullptr);
        AssertMenuItem(&menu[2], IDM_RA_OPENUSERPAGE, L"Open my &User Page");
        AssertMenuItem(&menu[3], IDM_RA_OPENGAMEPAGE, L"Open this &Game's Page");
        AssertMenuItem(&menu[4], 0, nullptr);
        AssertMenuItem(&menu[5], IDM_RA_HARDCORE_MODE, L"&Hardcore Mode", true);
        AssertMenuItem(&menu[6], IDM_RA_NON_HARDCORE_WARNING, L"Non-Hardcore &Warning", true);
        AssertMenuItem(&menu[7], 0, nullptr);
        AssertMenuItem(&menu[8], IDM_RA_TOGGLELEADERBOARDS, L"Enable &Leaderboards", true);
        AssertMenuItem(&menu[9], IDM_RA_OVERLAYSETTINGS, L"O&verlay Settings");
        AssertMenuItem(&menu[10], 0, nullptr);
        AssertMenuItem(&menu[11], IDM_RA_FILES_OPENALL, L"&Open All");
        AssertMenuItem(&menu[12], IDM_RA_FILES_ACHIEVEMENTS, L"Assets Li&st");
        AssertMenuItem(&menu[13], IDM_RA_FILES_ACHIEVEMENTEDITOR, L"Assets &Editor");
        AssertMenuItem(&menu[14], IDM_RA_FILES_MEMORYFINDER, L"&Memory Inspector");
        AssertMenuItem(&menu[15], IDM_RA_FILES_MEMORYBOOKMARKS, L"Memory &Bookmarks");
        AssertMenuItem(&menu[16], IDM_RA_FILES_POINTERFINDER, L"Pointer &Finder");
        AssertMenuItem(&menu[17], IDM_RA_FILES_CODENOTES, L"Code &Notes");
        AssertMenuItem(&menu[18], IDM_RA_PARSERICHPRESENCE, L"Rich &Presence Monitor");
        AssertMenuItem(&menu[19], 0, nullptr);
        AssertMenuItem(&menu[20], IDM_RA_REPORTBROKENACHIEVEMENTS, L"&Report Achievement Problem");
        AssertMenuItem(&menu[21], IDM_RA_GETROMCHECKSUM, L"View Game H&ash");
    }

    TEST_METHOD(TestUpdateAppTitle)
    {
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockConfiguration mockConfiguration;

        mockUserContext.Initialize("User", "TOKEN");
        mockEmulatorContext.MockClient("MyClient", "1.2.3");
        mockConfiguration.SetHostName("retroachievements.org");

        Assert::AreEqual(std::wstring(L"Window"), mockWindowManager.Emulator.GetWindowTitle());

        _RA_UpdateAppTitle("");
        Assert::AreEqual(std::wstring(L"MyClient - 1.2 - User_"), mockWindowManager.Emulator.GetWindowTitle());

        _RA_UpdateAppTitle("MyGame");
        Assert::AreEqual(std::wstring(L"MyClient - 1.2 - MyGame - User_"), mockWindowManager.Emulator.GetWindowTitle());

        _RA_UpdateAppTitle("");
        Assert::AreEqual(std::wstring(L"MyClient - 1.2 - User_"), mockWindowManager.Emulator.GetWindowTitle());
    }
};

} // namespace tests
} // namespace ra
