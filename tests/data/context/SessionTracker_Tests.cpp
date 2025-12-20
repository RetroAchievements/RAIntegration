#include "CppUnitTest.h"

#include "data\context\SessionTracker.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::StorageItemType;

namespace ra {
namespace data {
namespace context {
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
            mockRuntime.MockGame();
        }

        ra::context::mocks::MockRcClient mockRcClient;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
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

    private:
        std::string m_sUsername;
        std::wstring m_sUsernameWide;
    };

    TEST_METHOD(TestEmptyFile)
    {
        SessionTrackerHarness tracker;

        tracker.Initialize("User");
        Assert::IsFalse(tracker.HasStoredData());

        // BeginSession should begin the session, but not write to the file
        tracker.mockGameContext.SetGameId(1234U);
        tracker.BeginSession(1234U);
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual({ 1U }, tracker.mockThreadPool.PendingTasks());
        Assert::IsFalse(tracker.HasStoredData());

        // after 30 seconds, the callback will be called, but the file shouldn't be written until at least 60 seconds have elapsed
        tracker.mockClock.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(30));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual({ 1U }, tracker.mockThreadPool.PendingTasks());
        Assert::IsFalse(tracker.HasStoredData());

        // after two minutes, the callback will be called again, and the file finally written
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual({ 1U }, tracker.mockThreadPool.PendingTasks());
        Assert::IsTrue(tracker.HasStoredData());
        Assert::AreEqual(std::string("1234:1534889323:150:5d\n"), tracker.GetStoredData());

        // after two more minutes, the callback will be called again, and the file updated
        tracker.mockClock.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.AdvanceTime(std::chrono::seconds(120));
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual({ 1U }, tracker.mockThreadPool.PendingTasks());
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

        // BeginSession should begin the session, but not write to the file
        tracker.mockGameContext.SetGameId(1234U);
        tracker.BeginSession(1234U);
        tracker.mockThreadPool.ExecuteNextTask(); // execute async server call
        Assert::AreEqual(sInitialValue, tracker.GetStoredData());

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
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
