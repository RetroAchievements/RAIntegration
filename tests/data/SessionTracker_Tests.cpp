#include "CppUnitTest.h"

#include "data\SessionTracker.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::StorageItemType;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(SessionTracker_Tests)
{
public:
    class SessionTrackerHarness : public SessionTracker
    {
    public:
        GSL_SUPPRESS_F6 SessionTrackerHarness(const char* sUsername = "User") noexcept
            : m_sUsername(sUsername), m_sUsernameWide(ra::Widen(sUsername))
        {
        }


        ra::api::mocks::MockServer mockServer;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;

        bool HasStoredData() const
        {
            return mockStorage.HasStoredData(StorageItemType::SessionStats, m_sUsernameWide);
        }

        const std::string& GetStoredData() const
        {
            return mockStorage.GetStoredData(StorageItemType::SessionStats, m_sUsernameWide);
        }

        void MockStoredData(const std::string& sContents)
        {
            mockStorage.MockStoredData(StorageItemType::SessionStats, m_sUsernameWide, sContents);
        }

        void MockInspectingMemory(bool bInspectingMemory) noexcept 
        {
            m_bInspectingMemory = bInspectingMemory;
        }

        std::wstring GetActivity() const
        {
            return SessionTracker::GetCurrentActivity();
        }

    protected:
        bool IsInspectingMemory() const noexcept override { return m_bInspectingMemory; }

    private:
        bool m_bInspectingMemory{ false };
        std::string m_sUsername;
        std::wstring m_sUsernameWide;
    };

    TEST_METHOD(TestEmptyFile)
    {
        SessionTrackerHarness tracker;

        tracker.Initialize("User");
        Assert::IsFalse(tracker.HasStoredData());

        bool bSessionStarted = false;
        tracker.mockServer.HandleRequest<ra::api::StartSession>([&bSessionStarted](const ra::api::StartSession::Request& request, _UNUSED ra::api::StartSession::Response& /*response*/)
        {
            Assert::AreEqual(1234U, request.GameId);
            bSessionStarted = true;
            return false;
        });

        // BeginSession should begin the session, but not write to the file
        tracker.mockGameContext.SetGameId(1234U);
        tracker.BeginSession(1234U);
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::IsFalse(tracker.HasStoredData());
        Assert::IsTrue(bSessionStarted);

        // after 30 seconds, the callback will be called, but the file shouldn't be written until at least 60 seconds have elapsed
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::IsFalse(tracker.HasStoredData());

        // after two minutes, the callback will be called again, and the file finally written
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::IsTrue(tracker.HasStoredData());
        Assert::AreEqual(std::string("1234:1534889323:150:5d\n"), tracker.GetStoredData());

        // after two more minutes, the callback will be called again, and the file updated
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::IsTrue(tracker.HasStoredData());
        Assert::AreEqual(std::string("1234:1534889323:270:f9\n"), tracker.GetStoredData());
    }

    TEST_METHOD(TestNonEmptyFile)
    {
        std::string sInitialValue("1234:1534000000:1732:f5\n");
        SessionTrackerHarness tracker;
        tracker.MockStoredData(sInitialValue);

        tracker.Initialize("User");
        Assert::IsTrue(tracker.HasStoredData());

        bool bSessionStarted = false;
        tracker.mockServer.HandleRequest<ra::api::StartSession>([&bSessionStarted](const ra::api::StartSession::Request& request, _UNUSED ra::api::StartSession::Response& /*response*/)
        {
            Assert::AreEqual(1234U, request.GameId);
            bSessionStarted = true;
            return false;
        });

        // BeginSession should begin the session, but not write to the file
        tracker.mockGameContext.SetGameId(1234U);
        tracker.BeginSession(1234U);
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(sInitialValue, tracker.GetStoredData());
        Assert::IsTrue(bSessionStarted);

        // after 30 seconds, the callback will be called, but the file shouldn't be written until at least 60 seconds have elapsed
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(sInitialValue, tracker.GetStoredData());

        // after two minutes, the callback will be called again, and a new entry added to the file
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:150:5d\n"), tracker.GetStoredData());

        // after two more minutes, the callback will be called again, and the new entry updated
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:270:f9\n"), tracker.GetStoredData());

        // total playtime should include current session and previous session
        Assert::AreEqual(1732U + 270U, static_cast<unsigned int>(tracker.GetTotalPlaytime(1234U).count()));

        // ending session should count any time in the since the last callback
        tracker.mockClock.AdvanceTime(std::chrono::seconds(23));
        tracker.EndSession();
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:293:c9\n"), tracker.GetStoredData());
        Assert::AreEqual(1732U + 293U, static_cast<unsigned int>(tracker.GetTotalPlaytime(1234U).count()));
    }

    TEST_METHOD(TestNonEmptyFileBadChecksum)
    {
        std::string sInitialValue("1234:1534000000:1732:00\n");
        SessionTrackerHarness tracker;
        tracker.MockStoredData(sInitialValue);
        tracker.mockGameContext.SetGameId(1234U);

        tracker.Initialize("User");
        tracker.BeginSession(1234U);
        tracker.mockClock.AdvanceTime(std::chrono::seconds(180));
        Assert::AreEqual(180U, static_cast<unsigned int>(tracker.GetTotalPlaytime(1234U).count()));
    }

    TEST_METHOD(TestNonEmptyFileMultipleGames)
    {
        std::string sInitialValue("1234:1534000000:1732:f5\n"
                                  "9999:1534100000:963:4e\n"
                                  "1234:1534200000:591:0b\n");
        SessionTrackerHarness tracker;
        tracker.MockStoredData(sInitialValue);
        tracker.mockGameContext.SetGameId(1234U);

        tracker.Initialize("User");
        tracker.BeginSession(1234U);
        tracker.mockClock.AdvanceTime(std::chrono::seconds(180));
        Assert::AreEqual(1732U + 591U + 180U, static_cast<unsigned int>(tracker.GetTotalPlaytime(1234U).count()));
        Assert::AreEqual(963U, static_cast<unsigned int>(tracker.GetTotalPlaytime(9999U).count()));
    }

    TEST_METHOD(TestMultipleSessions)
    {
        std::string sInitialValue("1234:1534000000:1732:f5\n");
        SessionTrackerHarness tracker;
        tracker.MockStoredData(sInitialValue);

        tracker.Initialize("User");
        Assert::IsTrue(tracker.HasStoredData());

        tracker.mockGameContext.SetGameId(1234U);
        tracker.BeginSession(1234U);
        Assert::AreEqual(sInitialValue, tracker.GetStoredData());

        // after 30 seconds, the callback will be called, but the file shouldn't be written until at least 60 seconds have elapsed
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));

        // after two minutes, the callback will be called again, and a new entry added to the file
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:150:5d\n"), tracker.GetStoredData());

        // end session should include time since last callback
        tracker.mockClock.AdvanceTime(std::chrono::seconds(23));
        tracker.EndSession();
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:173:bb\n"), tracker.GetStoredData());

        // start new session
        tracker.BeginSession(9999U);

        // after 30 seconds, the callback will be called, but the file shouldn't be written until at least 60 seconds have elapsed
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));

        // after two minutes, the callback will be called again, and a new entry added to the file
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:173:bb\n9999:1534889496:150:d0\n"), tracker.GetStoredData());

        // end session should include time since last callback
        tracker.mockClock.AdvanceTime(std::chrono::seconds(19));
        tracker.EndSession();
        Assert::AreEqual(std::string("1234:1534000000:1732:f5\n1234:1534889323:173:bb\n9999:1534889496:169:14\n"), tracker.GetStoredData());
    }

    TEST_METHOD(TestPing)
    {
        SessionTrackerHarness tracker;

        tracker.Initialize("User");

        int nPings = 0;
        tracker.mockServer.HandleRequest<ra::api::Ping>([&nPings](const ra::api::Ping::Request& request, _UNUSED ra::api::Ping::Response& /*response*/)
        {
            Assert::AreEqual(1234U, request.GameId);
            Assert::AreEqual(std::wstring(L"Playing Banana"), request.CurrentActivity);
            ++nPings;
            return false;
        });

        // BeginSession should begin the session, but not write to the file
        tracker.mockGameContext.SetGameId(1234U);
        tracker.mockGameContext.SetGameTitle(L"Banana");
        tracker.BeginSession(1234U);
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call StartSession
        Assert::AreEqual(0, nPings);

        // after 30 seconds, the callback will be called, and the first ping should occur
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::AreEqual(1, nPings);

        // after two minutes, the callback will be called again, and the second ping should occur
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(2, nPings);

        // after two more minutes, the callback will be called again, and the third ping should occur
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(3, nPings);

        // once the session is ended, pinging should stop
        tracker.EndSession();
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(3, nPings);
    }

    TEST_METHOD(TestPingUnknownGame)
    {
        SessionTrackerHarness tracker;

        tracker.Initialize("User");
        tracker.mockGameContext.SetGameId(0U);
        tracker.mockGameContext.SetGameTitle(L"FileUnknown");

        int nPings = 0;
        tracker.mockServer.HandleRequest<ra::api::Ping>([&nPings](const ra::api::Ping::Request& request, _UNUSED ra::api::Ping::Response& /*response*/)
        {
            Assert::AreEqual(0U, request.GameId);
            Assert::AreEqual(std::wstring(L"Playing FileUnknown"), request.CurrentActivity);
            ++nPings;
            return false;
        });
        tracker.mockServer.ExpectUncalled<ra::api::StartSession>();

        // BeginSession should begin the session, but not send a start session
        tracker.BeginSession(0U);
        tracker.mockThreadPool.ExecuteNextTask(); // StartSession should not be queued
        Assert::AreEqual(0, nPings);

        // after 30 seconds, the callback will be called, and the first ping should occur
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(1U, tracker.mockThreadPool.PendingTasks());
        Assert::AreEqual(1, nPings);

        // after two minutes, the callback will be called again, and the second ping should occur
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(2, nPings);

        // total playtime should still be tallied, but not written to the file
        Assert::AreEqual(150, gsl::narrow<int>(tracker.GetTotalPlaytime(0U).count()));
        Assert::AreEqual(std::string(""), tracker.GetStoredData());
    }

    TEST_METHOD(TestCurrentActivityNoAchievements)
    {
        SessionTrackerHarness tracker;
        tracker.mockGameContext.SetGameTitle(L"GameTitle");
        Assert::AreEqual(std::wstring(L"Playing GameTitle"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityNoRichPresence)
    {
        SessionTrackerHarness tracker;
        tracker.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        Assert::AreEqual(std::wstring(L"Earning Achievements"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityRichPresence)
    {
        SessionTrackerHarness tracker;
        tracker.mockGameContext.SetRichPresenceDisplayString(L"Level 10");
        Assert::AreEqual(std::wstring(L"Level 10"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryNoAchievements)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        Assert::AreEqual(std::wstring(L"Developing Achievements"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryNoAchievementsHardcore)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        tracker.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(std::wstring(L"Developing Achievements"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryHardcore)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        tracker.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        tracker.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(std::wstring(L"Inspecting Memory in Hardcore mode"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryCoreAchievements)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        tracker.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        tracker.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        tracker.mockGameContext.SetActiveAchievementType(AchievementSet::Type::Core);
        Assert::AreEqual(std::wstring(L"Fixing Achievements"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryUnofficialAchievements)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        tracker.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        tracker.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        tracker.mockGameContext.SetActiveAchievementType(AchievementSet::Type::Unofficial);
        Assert::AreEqual(std::wstring(L"Developing Achievements"), tracker.GetActivity());
    }

    TEST_METHOD(TestCurrentActivityInspectingMemoryLocalAchievements)
    {
        SessionTrackerHarness tracker;
        tracker.MockInspectingMemory(true);
        tracker.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        tracker.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        tracker.mockGameContext.SetActiveAchievementType(AchievementSet::Type::Local);
        Assert::AreEqual(std::wstring(L"Developing Achievements"), tracker.GetActivity());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
