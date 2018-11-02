#include "services\impl\JsonFileConfiguration.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockFileSystem.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockFileSystem;

namespace ra {
namespace services {
namespace impl {
namespace tests {

TEST_CLASS(JsonFileConfiguration_Tests)
{
private:
    std::wstring sFilename = L"prefs.json";

public:
    TEST_METHOD(TestUsername)
    {
        MockFileSystem fileSystem;

        // default
        JsonFileConfiguration config;
        Assert::AreEqual(std::string(""), config.GetUsername());

        // no file
        Assert::IsFalse(config.Load(sFilename));
        Assert::AreEqual(std::string(""), config.GetUsername());

        // no value provided
        fileSystem.MockFile(sFilename, "{}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(std::string(""), config.GetUsername());

        // value provided
        fileSystem.MockFile(sFilename, "{\"Username\":\"User\"}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(std::string("User"), config.GetUsername());

        // provide value
        config.SetUsername("User2");
        Assert::AreEqual(std::string("User2"), config.GetUsername());

        // persist value
        config.Save();
        AssertContains(fileSystem.GetFileContents(sFilename), "\"Username\":\"User2\"");
    }

    TEST_METHOD(TestToken)
    {
        MockFileSystem fileSystem;

        // default
        JsonFileConfiguration config;
        Assert::AreEqual(std::string(""), config.GetApiToken());

        // no file
        Assert::IsFalse(config.Load(sFilename));
        Assert::AreEqual(std::string(""), config.GetApiToken());

        // no value provided
        fileSystem.MockFile(sFilename, "{}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(std::string(""), config.GetApiToken());

        // value provided
        fileSystem.MockFile(sFilename, "{\"Token\":\"API_TOKEN\"}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(std::string("API_TOKEN"), config.GetApiToken());

        // provide value
        config.SetApiToken("TOKEN");
        Assert::AreEqual(std::string("TOKEN"), config.GetApiToken());

        // persist value
        config.Save();
        AssertContains(fileSystem.GetFileContents(sFilename), "\"Token\":\"TOKEN\"");
    }

    void TestFeature(ra::services::Feature nFeature, const std::string& sJsonKey, bool bDefault)
    {
        MockFileSystem fileSystem;

        // uninitialized
        JsonFileConfiguration config;
        Assert::IsFalse(config.IsFeatureEnabled(nFeature), L"default");

        // no file
        Assert::IsFalse(config.Load(sFilename));
        Assert::AreEqual(bDefault, config.IsFeatureEnabled(nFeature), L"no file");

        // no value provided
        fileSystem.MockFile(sFilename, "{}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(bDefault, config.IsFeatureEnabled(nFeature), L"empty file");

        // value provided
        std::string sJson = "{\"" + sJsonKey + "\":" + (bDefault ? "false" : "true") + "}";
        fileSystem.MockFile(sFilename, sJson);
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(!bDefault, config.IsFeatureEnabled(nFeature), L"value provided");

        // provide value
        config.SetFeatureEnabled(nFeature, bDefault);
        Assert::AreEqual(bDefault, config.IsFeatureEnabled(nFeature), L"value set");

        // persist value
        config.Save();
        sJson = "\"" + sJsonKey + "\":" + (bDefault ? "true" : "false");
        AssertContains(fileSystem.GetFileContents(sFilename), sJson);
    }

    TEST_METHOD(TestHardcore)
    {
        TestFeature(ra::services::Feature::Hardcore, "Hardcore Active", true);
    }

    TEST_METHOD(TestLeaderboardsActive)
    {
        TestFeature(ra::services::Feature::Leaderboards, "Leaderboards Active", true);
    }

    TEST_METHOD(TestLeaderboardNotificationDisplay)
    {
        TestFeature(ra::services::Feature::LeaderboardNotifications, "Leaderboard Notification Display", true);
    }

    TEST_METHOD(TestLeaderboardCounterDisplay)
    {
        TestFeature(ra::services::Feature::LeaderboardCounters, "Leaderboard Counter Display", true);
    }

    TEST_METHOD(TestLeaderboardsScoreboardDisplay)
    {
        TestFeature(ra::services::Feature::LeaderboardScoreboards, "Leaderboard Scoreboard Display", true);
    }

    TEST_METHOD(TestPreferDecimal)
    {
        TestFeature(ra::services::Feature::PreferDecimal, "Prefer Decimal", false);
    }

    TEST_METHOD(TestHostNameNoFile)
    {
        MockFileSystem mockFileSystem;
        JsonFileConfiguration config;
        Assert::AreEqual(std::string("retroachievements.org"), config.GetHostName());
    }

    TEST_METHOD(TestHostNameFromFile)
    {
        MockFileSystem mockFileSystem;
        mockFileSystem.MockFile(L"host.txt", "stage.retroachievements.org");
        JsonFileConfiguration config;
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());

        // file should only be read once
        mockFileSystem.MockFile(L"host.txt", "dev.retroachievements.org");
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
