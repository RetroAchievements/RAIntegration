#include "CppUnitTest.h"

#include "data\models\RichPresenceModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(RichPresenceModel_Tests)
{
private:
    class RichPresenceModelHarness : public RichPresenceModel
    {
    public:
        RichPresenceModelHarness()
        {
            mockRuntime.MockGame();
        }

        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::impl::StringTextWriter textWriter;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        RichPresenceModelHarness richPresence;

        Assert::AreEqual(AssetType::RichPresence, richPresence.GetType());
        Assert::AreEqual(0U, richPresence.GetID());
        Assert::AreEqual(std::wstring(L"Rich Presence"), richPresence.GetName());
        Assert::AreEqual(std::wstring(L""), richPresence.GetDescription());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());
        Assert::AreEqual(AssetState::Inactive, richPresence.GetState());
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());
        Assert::AreEqual(std::string(""), richPresence.GetScript());
    }

    TEST_METHOD(TestNormalizeScript)
    {
        RichPresenceModelHarness richPresence;
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("Display:\r\nTest\r\n");
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("Display:\nTest");
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("Display:\nTest2\n");
        Assert::AreEqual(AssetChanges::Modified, richPresence.GetChanges());
    }

    TEST_METHOD(TestNormalizeScriptOnlyWhitespace)
    {
        RichPresenceModelHarness richPresence;
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("\n");
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("     ");
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.SetScript("\n\nDisplay:\nTest\n");
        Assert::AreEqual(AssetChanges::Modified, richPresence.GetChanges());
    }

    TEST_METHOD(TestReloadWritesFile)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());
        Assert::AreEqual(std::string(), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());
        Assert::AreEqual(std::string("Display:\nTest\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestReloadReadsFile)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom file\n");
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());

        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestReloadNormalizesFile)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "\xef\xbb\xbf\nDisplay:\r\nFrom file\r\n");
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(std::string("\nDisplay:\nFrom file\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());

        Assert::AreEqual(std::string("\xef\xbb\xbf\nDisplay:\r\nFrom file\r\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestReloadReadsFileNoServerData)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom file\n");
        richPresence.CreateServerCheckpoint();
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Local, richPresence.GetCategory());

        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestReloadDiscardsChanges)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom file\n");
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.SetScript("Display:\nLocal\n");
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());

        Assert::AreEqual(std::string("Display:\nFrom file\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestReloadDiscardsChangesMatchesServer)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nTest\n");
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.SetScript("Display:\nLocal\n");
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());

        richPresence.ReloadRichPresenceScript();
        Assert::AreEqual(std::string("Display:\nTest\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());

        Assert::AreEqual(std::string("Display:\nTest\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }

    TEST_METHOD(TestRevertUpdatesFile)
    {
        RichPresenceModelHarness richPresence;
        richPresence.mockGameContext.SetGameId(1);
        richPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom file\n");
        richPresence.SetScript("Display:\nTest\n");
        richPresence.CreateServerCheckpoint();
        richPresence.SetScript("Display:\nLocal\n");
        richPresence.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::Unpublished, richPresence.GetChanges());

        richPresence.RestoreServerCheckpoint();
        Assert::AreEqual(std::string("Display:\nTest\n"), richPresence.GetScript());
        Assert::AreEqual(AssetChanges::None, richPresence.GetChanges());
        Assert::AreEqual(AssetCategory::Core, richPresence.GetCategory());

        Assert::AreEqual(std::string("Display:\nTest\n"), richPresence.mockLocalStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
