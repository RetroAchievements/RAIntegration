#include "services\impl\JsonFileConfiguration.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockFileSystem.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockFileSystem;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::PopupLocation>(
    const ra::ui::viewmodels::PopupLocation& nPopupLocation)
{
    switch (nPopupLocation)
    {
        case ra::ui::viewmodels::PopupLocation::None:
            return L"None";
        case ra::ui::viewmodels::PopupLocation::TopLeft:
            return L"TopLeft";
        case ra::ui::viewmodels::PopupLocation::TopMiddle:
            return L"TopMiddle";
        case ra::ui::viewmodels::PopupLocation::TopRight:
            return L"TopRight";
        case ra::ui::viewmodels::PopupLocation::BottomLeft:
            return L"BottomLeft";
        case ra::ui::viewmodels::PopupLocation::BottomMiddle:
            return L"BottomMiddle";
        case ra::ui::viewmodels::PopupLocation::BottomRight:
            return L"BottomRight";
        default:
            return std::to_wstring(static_cast<int>(nPopupLocation));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

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

    TEST_METHOD(TestNonHardcoreWarning)
    {
        TestFeature(ra::services::Feature::NonHardcoreWarning, "Non Hardcore Warning", false);
    }

    TEST_METHOD(TestAchievementTriggeredScreenshot)
    {
        TestFeature(ra::services::Feature::AchievementTriggeredScreenshot, "Achievement Triggered Screenshot", false);
    }

    TEST_METHOD(TestMasteryScreenshot)
    {
        TestFeature(ra::services::Feature::MasteryNotificationScreenshot, "Mastery Screenshot", false);
    }

    TEST_METHOD(TestLeaderboardsActive)
    {
        TestFeature(ra::services::Feature::Leaderboards, "Leaderboards Active", true);
    }

    TEST_METHOD(TestPreferDecimal)
    {
        TestFeature(ra::services::Feature::PreferDecimal, "Prefer Decimal", false);
    }

    void TestPopupLocation(ra::ui::viewmodels::Popup nPopup, const std::string& sJsonKey, ra::ui::viewmodels::PopupLocation nDefault)
    {
        MockFileSystem fileSystem;

        // uninitialized
        JsonFileConfiguration config;
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::None, config.GetPopupLocation(nPopup), L"default");

        // no file
        Assert::IsFalse(config.Load(sFilename));
        Assert::AreEqual(nDefault, config.GetPopupLocation(nPopup), L"no file");

        // no value provided
        fileSystem.MockFile(sFilename, "{}");
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(nDefault, config.GetPopupLocation(nPopup), L"empty file");

        // value provided
        std::string sJson = "{\"" + sJsonKey + "\":\"TopRight\"}";
        fileSystem.MockFile(sFilename, sJson);
        Assert::IsTrue(config.Load(sFilename));
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopRight, config.GetPopupLocation(nPopup), L"value provided");

        // provide value
        config.SetPopupLocation(nPopup, ra::ui::viewmodels::PopupLocation::TopLeft);
        Assert::AreEqual(ra::ui::viewmodels::PopupLocation::TopLeft, config.GetPopupLocation(nPopup), L"value set");

        // persist value
        config.Save();
        sJson = "\"" + sJsonKey + "\":\"TopLeft\"";
        AssertContains(fileSystem.GetFileContents(sFilename), sJson);
    }

    TEST_METHOD(TestLeaderboardNotificationDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted, "Leaderboard Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft);
    }

    TEST_METHOD(TestLeaderboardCanceledDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled, "Leaderboard Cancel Display", ra::ui::viewmodels::PopupLocation::BottomLeft);
    }

    TEST_METHOD(TestLeaderboardCounterDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker, "Leaderboard Counter Display", ra::ui::viewmodels::PopupLocation::BottomRight);
    }

    TEST_METHOD(TestLeaderboardsScoreboardDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, "Leaderboard Scoreboard Display", ra::ui::viewmodels::PopupLocation::BottomRight);
    }

    TEST_METHOD(TestChallengeNotificationDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::Challenge, "Challenge Notification Display", ra::ui::viewmodels::PopupLocation::BottomRight);
    }

    TEST_METHOD(TestMessageDisplay)
    {
        TestPopupLocation(ra::ui::viewmodels::Popup::Message, "Informational Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft);
    }

    TEST_METHOD(TestHostNameNoFile)
    {
        MockFileSystem mockFileSystem;
        JsonFileConfiguration config;
        Assert::AreEqual(std::string("retroachievements.org"), config.GetHostName());
        Assert::AreEqual(std::string("https://retroachievements.org"), config.GetHostUrl());
        Assert::AreEqual(std::string("http://i.retroachievements.org"), config.GetImageHostUrl());
    }

    TEST_METHOD(TestHostNameFromFile)
    {
        MockFileSystem mockFileSystem;
        mockFileSystem.MockFile(L"host.txt", "stage.retroachievements.org");
        JsonFileConfiguration config;
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());
        Assert::AreEqual(std::string("http://stage.retroachievements.org"), config.GetHostUrl());
        Assert::AreEqual(std::string("http://stage.retroachievements.org"), config.GetImageHostUrl());

        // file should only be read once
        mockFileSystem.MockFile(L"host.txt", "dev.retroachievements.org");
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());
        Assert::AreEqual(std::string("http://stage.retroachievements.org"), config.GetHostUrl());
        Assert::AreEqual(std::string("http://stage.retroachievements.org"), config.GetImageHostUrl());
    }

    TEST_METHOD(TestHostNameFromFileWithProtocol)
    {
        MockFileSystem mockFileSystem;
        mockFileSystem.MockFile(L"host.txt", "https://stage.retroachievements.org");
        JsonFileConfiguration config;
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());
        Assert::AreEqual(std::string("https://stage.retroachievements.org"), config.GetHostUrl());
        Assert::AreEqual(std::string("https://stage.retroachievements.org"), config.GetImageHostUrl());

        // file should only be read once
        mockFileSystem.MockFile(L"host.txt", "https://dev.retroachievements.org");
        Assert::AreEqual(std::string("stage.retroachievements.org"), config.GetHostName());
        Assert::AreEqual(std::string("https://stage.retroachievements.org"), config.GetHostUrl());
        Assert::AreEqual(std::string("https://stage.retroachievements.org"), config.GetImageHostUrl());
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
