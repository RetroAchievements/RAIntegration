#include "CppUnitTest.h"

#include "data\context\GameAssets.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\LocalBadgesModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\testutil\AssetAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AssetCategory;
using ra::data::models::AssetChanges;
using ra::data::models::AssetType;

namespace ra {
namespace data {
namespace context {
namespace tests {

TEST_CLASS(GameAssets_Tests)
{
public:
    class GameAssetsHarness : public GameAssets
    {
    public:
        ra::context::mocks::MockRcClient mockRcClient;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockLocalStorage mockLocalStorage;

        GameAssetsHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockGameContext.SetGameTitle(L"GameName");
            GSL_SUPPRESS_F6 mockRuntime.MockGame();
        }

        void SaveAllAssets()
        {
            std::vector<ra::data::models::AssetModelBase*> vEmptyList;
            SaveAssets(vEmptyList);
        }

        void ReloadAllAssets()
        {
            std::vector<ra::data::models::AssetModelBase*> vEmptyList;
            ReloadAssets(vEmptyList);
        }

        void ReloadAsset(AssetType nType, uint32_t nId)
        {
            std::vector<ra::data::models::AssetModelBase*> vList;
            auto* pAsset = FindAsset(nType, nId);
            Expects(pAsset != nullptr);
            vList.push_back(pAsset);

            ReloadAssets(vList);
        }

        ra::data::models::AchievementModel& AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto& pAchievement = NewAchievement();
            pAchievement.SetCategory(nCategory);
            pAchievement.SetPoints(nPoints);
            pAchievement.SetName(sTitle);
            pAchievement.SetDescription(sDescription);
            pAchievement.SetBadge(sBadge);
            pAchievement.SetTrigger(sTrigger);

            std::wstring sAuthor = L"Auth";
            sAuthor.push_back(sBadge.front());
            pAchievement.SetAuthor(sAuthor);

            if (nCategory == AssetCategory::Local)
            {
                pAchievement.UpdateLocalCheckpoint();
            }
            else
            {
                pAchievement.SetID(gsl::narrow_cast<uint32_t>(Count()));
                pAchievement.UpdateServerCheckpoint();
            }

            return pAchievement;
        }

        ra::data::models::AchievementModel& AddAchievement()
        {
            return AddAchievement(AssetCategory::Local, 5, L"AchievementTitle", L"AchievementDescription", L"12345", "1=1");
        }

        ra::data::models::AchievementModel& AddNewAchievement(unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto& pAchievement = AddAchievement(AssetCategory::Local, nPoints, sTitle, sDescription, sBadge, sTrigger);
            pAchievement.SetNew();
            return pAchievement;
        }

        void AddThreeAchievements()
        {
            AddAchievement(AssetCategory::Core, 5, L"Ach1", L"Desc1", L"11111", "1=1");
            AddAchievement(AssetCategory::Unofficial, 10, L"Ach2", L"Desc2", L"22222", "2=2");
            AddAchievement(AssetCategory::Core, 15, L"Ach3", L"Desc3", L"33333", "3=3");
            ResetLocalId();
        }

        ra::data::models::LeaderboardModel& AddLeaderboard(AssetCategory nCategory,
            const std::wstring& sTitle, const std::wstring& sDescription, const std::string& sStart,
            const std::string& sCancel, const std::string& sSubmit, const std::string& sValue, ValueFormat nFormat)
        {
            auto& pLeaderboard = NewLeaderboard();
            pLeaderboard.SetCategory(nCategory);
            pLeaderboard.SetName(sTitle);
            pLeaderboard.SetDescription(sDescription);
            pLeaderboard.SetStartTrigger(sStart);
            pLeaderboard.SetSubmitTrigger(sSubmit);
            pLeaderboard.SetCancelTrigger(sCancel);
            pLeaderboard.SetValueDefinition(sValue);
            pLeaderboard.SetValueFormat(nFormat);

            if (nCategory == AssetCategory::Local)
            {
                pLeaderboard.UpdateLocalCheckpoint();
            }
            else
            {
                pLeaderboard.SetID(gsl::narrow_cast<uint32_t>(Count()));
                pLeaderboard.UpdateServerCheckpoint();
            }

            return pLeaderboard;
        }

        void MockUserFile(const std::string& sContents)
        {
            return mockLocalStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, std::to_wstring(mockGameContext.GameId()), sContents);
        }

        void MockUserFileContents(const std::string& sContents)
        {
            MockUserFile("0.0.0.0\nGame Title\n" + sContents);
        }

        const std::string& GetUserFile()
        {
            return mockLocalStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, std::to_wstring(mockGameContext.GameId()));
        }

        void AddLocalBadgesModel()
        {
            auto pLocalBadges = std::make_unique<ra::data::models::LocalBadgesModel>();
            pLocalBadges->CreateServerCheckpoint();
            pLocalBadges->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(pLocalBadges));
        }

        ra::data::models::LocalBadgesModel& GetLocalBadgesModel()
        {
            return *(dynamic_cast<ra::data::models::LocalBadgesModel*>(mockGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0)));
        }

        void AddRichPresenceModel()
        {
            auto pRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
            pRichPresence->CreateServerCheckpoint();
            pRichPresence->CreateLocalCheckpoint();
            Append(std::move(pRichPresence));
        }

        ra::data::models::RichPresenceModel& GetRichPresenceModel()
        {
            return *(dynamic_cast<ra::data::models::RichPresenceModel*>(FindAsset(ra::data::models::AssetType::RichPresence, 0)));
        }

        void AddMemoryRegionsModel()
        {
            auto pMemoryRegions = std::make_unique<ra::data::models::MemoryRegionsModel>();
            pMemoryRegions->CreateServerCheckpoint();
            pMemoryRegions->CreateLocalCheckpoint();
            Append(std::move(pMemoryRegions));
        }

        ra::data::models::MemoryRegionsModel& GetMemoryRegions()
        {
            return *(dynamic_cast<ra::data::models::MemoryRegionsModel*>(FindAsset(ra::data::models::AssetType::MemoryRegions, 0)));
        }
    };

    TEST_METHOD(TestSaveLocalEmpty)
    {
        GameAssetsHarness gameAssets;
        gameAssets.SaveAllAssets();

        Assert::AreEqual(std::string("0.0.0.0\nGame Title\n"), gameAssets.GetUserFile());
    }

    TEST_METHOD(TestSaveLocalOnlySavesLocal)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddAchievement(AssetCategory::Core, 5, L"Ach1", L"Desc1", L"11111", "1=1");
        gameAssets.AddAchievement(AssetCategory::Local, 5, L"Ach2", L"Desc2", L"22222", "2=2");
        gameAssets.AddAchievement(AssetCategory::Unofficial, 5, L"Ach3", L"Desc3", L"33333", "3=3");

        gameAssets.SaveAllAssets();

        const auto& sExpected = ra::util::String::Printf("0.0.0.0\nGame Title\n%u:\"2=2\":Ach2:Desc2::::Auth2:5:::::22222\n", GameAssets::FirstLocalId + 1);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }

    TEST_METHOD(TestSaveLocalEscaped)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddAchievement(AssetCategory::Local, 15, L"Ach:2", L"Desc \"2\"", L"54321", "2=2");

        gameAssets.SaveAllAssets();

        const auto& sExpected = ra::util::String::Printf("0.0.0.0\nGame Title\n%u:\"2=2\":\"Ach:2\":\"Desc \\\"2\\\"\"::::Auth5:15:::::54321\n", GameAssets::FirstLocalId);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }

    TEST_METHOD(TestSaveLocalSubset)
    {
        GameAssetsHarness gameAssets;
        auto& pAch = gameAssets.AddAchievement(AssetCategory::Local, 5, L"Ach2", L"Desc2", L"22222", "2=2");
        pAch.SetSubsetID(77U);

        gameAssets.SaveAllAssets();

        const auto& sExpected = ra::util::String::Printf("0.0.0.0\nGame Title\n%u|77:\"2=2\":Ach2:Desc2::::Auth2:5:::::22222\n", GameAssets::FirstLocalId);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }

    TEST_METHOD(TestSaveLocalBadges)
    {
        GameAssetsHarness gameAssets;
        gameAssets.mockGameContext.SetGameId(22);
        gameAssets.AddLocalBadgesModel();
        gameAssets.AddNewAchievement(10, L"Temp", L"Temp", L"local\\22-ABCDE.png", "1=1");

        // gameAssets is not the same instance as mockGameContext.Assets, so manually update the badge reference count
        auto& pLocalBadges = gameAssets.GetLocalBadgesModel();
        pLocalBadges.AddReference(L"local\\22-ABCDE.png", false);

        // simulate the pre-commit of the achievement by converting the reference from uncommitted to committed
        pLocalBadges.RemoveReference(L"local\\22-ABCDE.png", false);
        pLocalBadges.AddReference(L"local\\22-ABCDE.png", true);

        gameAssets.SaveAllAssets();

        gameAssets.ReloadAsset(AssetType::Achievement, GameAssets::FirstLocalId);

        const auto& sExpected = ra::util::String::Printf("0.0.0.0\nGame Title\n"
            "%u:\"1=1\":Temp:Temp::::Authl:10:::::\"local\\\\22-ABCDE.png\"\n", GameAssets::FirstLocalId);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }

    TEST_METHOD(TestSaveDeleted)
    {
        GameAssetsHarness gameAssets;
        gameAssets.MockUserFileContents(
            "111000001:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "111000002:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();
        Assert::AreEqual({ 2U }, gameAssets.Count());

        auto* pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        pAsset->SetDeleted();
        Assert::AreEqual(AssetChanges::Deleted, pAsset->GetChanges());

        gameAssets.SaveAllAssets();

        const std::string sExpected = "0.0.0.0\nGame Title\n"
            "111000002:\"0xH2345=0\":Test2:::::User:0:::::00000\n";
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());

        Assert::AreEqual({ 1U }, gameAssets.Count());

        pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNull(pAsset);

        pAsset = gameAssets.FindAchievement({ 111000002U });
        Assert::IsNotNull(pAsset);
    }

    TEST_METHOD(TestMergeLocalAssetsNoFile)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        Assert::AreEqual({ 3U }, gameAssets.Count());

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 3U }, gameAssets.Count());
    }

    TEST_METHOD(TestMergeLocalAssetsEmptyFile)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFile("");
        Assert::AreEqual({ 3U }, gameAssets.Count());

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 3U }, gameAssets.Count());
    }

    TEST_METHOD(TestMergeLocalAssetsTwoLocalAchievements)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFileContents(
            "111000001:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "111000002:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 5U }, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindAchievement({ 111000002U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAsset2->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestMergeLocalAssetsTwoLocalAchievementsNoPublished)
    {
        GameAssetsHarness gameAssets;
        gameAssets.MockUserFileContents(
            "111000001:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "111000002:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 2U }, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindAchievement({ 111000002U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAsset2->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestMergeLocalAssetsTwoPublishedAchievements)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFileContents(
            "1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "2:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 3U }, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindAchievement({ 2U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAsset2->GetTrigger());
        Assert::AreEqual(AssetCategory::Unofficial, pAsset2->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestMergeLocalAssetsUnknownPublishedAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFileContents(
            "67:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 4U }, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({ 67U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestMergeLocalAssetsTwoLocalAchievementsWithoutID)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFileContents(
            "0:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "111000002:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 5U }, gameAssets.Count());

        // item without ID will be allocated an ID after all other achievements are processed
        const auto* pAsset = gameAssets.FindAchievement({ 111000003U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindAchievement({ 111000002U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAsset2->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestMergeLocalAssetsLocalBadges)
    {
        GameAssetsHarness gameAssets;
        gameAssets.mockGameContext.SetGameId(22);
        gameAssets.AddLocalBadgesModel();
        gameAssets.MockUserFileContents(
            "111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::local\\22-A.png\n");

        gameAssets.ReloadAllAssets();

        const auto* pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::wstring(L"local\\22-A.png"), pAsset->GetBadge());

        const auto& pLocalBadges = gameAssets.GetLocalBadgesModel();
        Assert::AreEqual(1, pLocalBadges.GetReferenceCount(L"local\\22-A.png", true));
    }

    TEST_METHOD(TestMergeLocalAssetsTwoLocalAchievementsWithSubsetInfo)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.MockUserFileContents(
            "111000001:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "111000002|2223:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({5U}, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({111000001U});
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(0U, pAsset->GetSubsetID());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindAchievement({111000002U});
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAsset2->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset2->GetCategory());
        Assert::AreEqual(2223U, pAsset2->GetSubsetID());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestReloadModifiedFromFile)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();

        auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAsset(AssetType::Achievement, 1);

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestReloadAllModifiedFromFile)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();

        auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestReloadIgnoresUnrequested)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();

        gameAssets.MockUserFileContents(
            "1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "3:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAsset(AssetType::Achievement, 1);

        // item 1 should have been updated from file
        const auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        // item 3 should not have been updated from file
        const auto* pAsset3 = gameAssets.FindAchievement({ 3U });
        Assert::IsNotNull(pAsset3);
        Ensures(pAsset3 != nullptr);
        Assert::AreEqual(std::string("3=3"), pAsset3->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset3->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset3->GetChanges());
    }

    TEST_METHOD(TestReloadDiscardsMissingLocalAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddAchievement(AssetCategory::Local, 10, L"Temp", L"Temp", L"12345", "6=6");

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        Assert::IsNotNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));

        gameAssets.ReloadAsset(AssetType::Achievement, GameAssets::FirstLocalId);

        // resetting local achievement that is no longer in local removes it from the list
        Assert::IsNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));
    }

    TEST_METHOD(TestReloadAllDiscardsMissingLocalAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddAchievement(AssetCategory::Local, 10, L"Temp", L"Temp", L"12345", "6=6");

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        Assert::IsNotNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));

        gameAssets.ReloadAllAssets();

        // resetting local achievement that is no longer in local removes it from the list
        Assert::IsNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));
    }

    TEST_METHOD(TestReloadDiscardsMissingLocalChangesToPublishedAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();

        auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Server");
        pAsset->UpdateServerCheckpoint();
        pAsset->SetName(L"Local");
        pAsset->UpdateLocalCheckpoint();

        // NOTE: 1 is not in the user file
        gameAssets.MockUserFileContents("3:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAsset(AssetType::Achievement, 1);

        // resetting achievement with local changes that is no longer in local resets to server state
        const auto* pAsset2 = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::wstring(L"Server"), pAsset2->GetName());
        Assert::AreEqual(AssetCategory::Core, pAsset2->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset2->GetChanges());
    }

    TEST_METHOD(TestReloadAllDiscardsMissingLocalChangesToPublishedAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();

        auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Server");
        pAsset->UpdateServerCheckpoint();
        pAsset->SetName(L"Local");
        pAsset->UpdateLocalCheckpoint();

        // NOTE: 1 is not in the user file
        gameAssets.MockUserFileContents("3:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        // resetting achievement with local changes that is no longer in local resets to server state
        const auto* pAsset2 = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::wstring(L"Server"), pAsset2->GetName());
        Assert::AreEqual(AssetCategory::Core, pAsset2->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset2->GetChanges());
    }

    TEST_METHOD(TestReloadDiscardsNewAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddNewAchievement(10, L"Temp", L"Temp", L"12345", "1=1");

        auto* pAsset = gameAssets.FindAchievement(GameAssets::FirstLocalId);
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAsset(AssetType::Achievement, GameAssets::FirstLocalId);

        Assert::IsNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));
    }

    TEST_METHOD(TestReloadAllDiscardsNewAchievement)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddNewAchievement(10, L"Temp", L"Temp", L"12345", "1=1");

        auto* pAsset = gameAssets.FindAchievement(GameAssets::FirstLocalId);
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());

        gameAssets.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        gameAssets.ReloadAllAssets();

        Assert::IsNull(gameAssets.FindAchievement(GameAssets::FirstLocalId));
    }

    TEST_METHOD(TestReloadIgnoresMemoryRegions)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddMemoryRegionsModel();

        auto& pMemoryRegions = gameAssets.GetMemoryRegions();
        pMemoryRegions.AddCustomRegion(0x1000, 0x10FF, L"Custom");
        Assert::AreEqual({ 1U }, pMemoryRegions.CustomRegions().size());

        gameAssets.MockUserFileContents(
            "1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "3:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n"
        );

        gameAssets.ReloadAsset(AssetType::Achievement, 1);

        // item 1 should have been updated from file
        const auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        // memory regions should not have been modified
        Assert::AreEqual({ 1U }, pMemoryRegions.CustomRegions().size());
    }

    TEST_METHOD(TestReloadAllPicksUpMemoryRegions)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddMemoryRegionsModel();

        const auto& pMemoryRegions = gameAssets.GetMemoryRegions();
        Assert::AreEqual({ 0U }, pMemoryRegions.CustomRegions().size());

        gameAssets.MockUserFileContents(
            "1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "3:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n"
            "M0:0x1000-0x10FF:\"Custom\"\n"
        );

        gameAssets.ReloadAllAssets();

        // item 1 should have been updated from file
        const auto* pAsset = gameAssets.FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        // memory regions should have been modified
        Assert::AreEqual({ 1U }, pMemoryRegions.CustomRegions().size());
    }

    TEST_METHOD(TestSaveLocalLeaderboard)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddLeaderboard(AssetCategory::Core, L"LB1", L"Desc1", "0xH1234=1", "0xH1234=2", "0xH1234=3", "M:0xH1235", ValueFormat::Seconds);
        gameAssets.AddLeaderboard(AssetCategory::Local, L"LB2", L"Desc2", "0xH2234=1", "0xH2234=2", "0xH2234=3", "M:0xH2235", ValueFormat::Minutes);

        gameAssets.SaveAllAssets();

        const auto& sExpected = ra::util::String::Printf("0.0.0.0\nGame Title\nL%u:\"0xH2234=1\":\"0xH2234=2\":\"0xH2234=3\":\"M:0xH2235\":MINUTES:LB2:Desc2:0\n", GameAssets::FirstLocalId + 1);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }

    TEST_METHOD(TestMergeLocalAssetsAchievementAndLeaderboard)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddThreeAchievements();
        gameAssets.AddLeaderboard(AssetCategory::Core, L"LB1", L"Desc1", "0xH1234=1", "0xH1234=2", "0xH1234=3", "M:0xH1235", ValueFormat::Seconds);
        gameAssets.MockUserFileContents(
            "111000001:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n"
            "L111000002:\"0xH2234=1\":\"0xH2234=2\":\"0xH2234=3\":\"M:0xH2235\":MINUTES:LB2:Desc2\n");

        gameAssets.ReloadAllAssets();

        Assert::AreEqual({ 6U }, gameAssets.Count());

        const auto* pAsset = gameAssets.FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        const auto* pAsset2 = gameAssets.FindLeaderboard({ 111000002U });
        Assert::IsNotNull(pAsset2);
        Ensures(pAsset2 != nullptr);
        Assert::AreEqual(std::string("0xH2234=1"), pAsset2->GetStartTrigger());
        Assert::AreEqual(std::string("0xH2234=2"), pAsset2->GetCancelTrigger());
        Assert::AreEqual(std::string("0xH2234=3"), pAsset2->GetSubmitTrigger());
        Assert::AreEqual(std::string("M:0xH2235"), pAsset2->GetValueDefinition());
        Assert::AreEqual(ValueFormat::Minutes, pAsset2->GetValueFormat());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset2->GetChanges());
    }

    TEST_METHOD(TestSaveUnpublishedRichPresence)
    {
        GameAssetsHarness gameAssets;
        gameAssets.AddAchievement(AssetCategory::Local, 5, L"Ach2", L"Desc2", L"22222", "2=2");
        gameAssets.AddRichPresenceModel();

        auto& pRichPresence = gameAssets.GetRichPresenceModel();
        pRichPresence.SetScript("Display:\nThe Best!");
        pRichPresence.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pRichPresence.GetChanges());

        gameAssets.SaveAllAssets();

        const auto& sExpected = ra::util::String::Printf(
            "0.0.0.0\nGame Title\n%u:\"2=2\":Ach2:Desc2::::Auth2:5:::::22222\n",
            GameAssets::FirstLocalId);
        Assert::AreEqual(sExpected, gameAssets.GetUserFile());
    }
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
