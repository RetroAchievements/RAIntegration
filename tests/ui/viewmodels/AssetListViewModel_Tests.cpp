#include "CppUnitTest.h"

#include "ui\viewmodels\AssetListViewModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetListViewModel_Tests)
{
private:
    class AssetListViewModelHarness : public AssetListViewModel
    {
    public:
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle)
        {
            auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            Assets().Append(std::move(vmAchievement));
        }

        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            Assets().Append(std::move(vmAchievement));
        }

        void AddNewAchievement(unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(Assets().Count() + 1));
            vmAchievement->SetCategory(AssetCategory::Local);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateLocalCheckpoint();
            Assets().Append(std::move(vmAchievement));
        }

        void AddThreeAchievements()
        {
            AddAchievement(AssetCategory::Core, 5, L"Ach1");
            AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");
            AddAchievement(AssetCategory::Core, 15, L"Ach3");
        }

        void ForceUpdateButtons()
        {
            mockThreadPool.AdvanceTime(std::chrono::milliseconds(500));
        }

        const std::string& GetUserFile(const std::wstring& sGameId)
        {
            return mockLocalStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, sGameId);
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AssetListViewModelHarness vmAssetList;

        Assert::AreEqual({ 0U }, vmAssetList.Assets().Count());
        Assert::AreEqual(0U, vmAssetList.GetGameId());
        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
        Assert::AreEqual(true, vmAssetList.IsProcessingActive());

        Assert::AreEqual({ 5U }, vmAssetList.States().Count());
        Assert::AreEqual((int)AssetState::Inactive, vmAssetList.States().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Inactive"), vmAssetList.States().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetState::Waiting, vmAssetList.States().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Waiting"), vmAssetList.States().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetState::Active, vmAssetList.States().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Active"), vmAssetList.States().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetState::Paused, vmAssetList.States().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Paused"), vmAssetList.States().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)AssetState::Triggered, vmAssetList.States().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Triggered"), vmAssetList.States().GetItemAt(4)->GetLabel());

        Assert::AreEqual({ 3U }, vmAssetList.Categories().Count());
        Assert::AreEqual((int)AssetCategory::Core, vmAssetList.Categories().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Core"), vmAssetList.Categories().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetCategory::Unofficial, vmAssetList.Categories().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Unofficial"), vmAssetList.Categories().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetCategory::Local, vmAssetList.Categories().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Local"), vmAssetList.Categories().GetItemAt(2)->GetLabel());

        Assert::AreEqual({ 3U }, vmAssetList.Changes().Count());
        Assert::AreEqual((int)AssetChanges::None, vmAssetList.Changes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), vmAssetList.Changes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Modified, vmAssetList.Changes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Modified"), vmAssetList.Changes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Unpublished, vmAssetList.Changes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Unpublished"), vmAssetList.Changes().GetItemAt(2)->GetLabel());
    }

    TEST_METHOD(TestAddRemoveCoreAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 20, L"Ach3");

        Assert::AreEqual(3, vmAssetList.GetAchievementCount());
        Assert::AreEqual(35, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(1);

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddRemoveUnofficialAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Ach1");

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddRemoveLocalAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Ach1");

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Local, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestPointsChange)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        vmAssetList.AddAchievement(AssetCategory::Local, 10, L"Ach2");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        auto ach = dynamic_cast<ra::ui::viewmodels::AchievementViewModel*>(vmAssetList.Assets().GetItemAt(0));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(50);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::ui::viewmodels::AchievementViewModel*>(vmAssetList.Assets().GetItemAt(1));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(40);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::ui::viewmodels::AchievementViewModel*>(vmAssetList.Assets().GetItemAt(2));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(30);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddItemWithFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual({ 1U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
    }

    TEST_METHOD(TestAddItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.Assets().BeginUpdate();

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual({ 1U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().EndUpdate();

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
    }

    TEST_METHOD(TestRemoveItemWithFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestRemoveItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.Assets().BeginUpdate();
        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.Assets().EndUpdate();

        Assert::AreEqual({ 0U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestChangeItemForFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.Assets().GetItemAt(0)->SetCategory(ra::ui::viewmodels::AssetCategory::Unofficial);

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.Assets().GetItemAt(1)->SetCategory(ra::ui::viewmodels::AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(1)->GetId()); // item changed to match filter appears at end of list

        vmAssetList.Assets().GetItemAt(0)->SetCategory(ra::ui::viewmodels::AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(30, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(2)->GetId()); // item changed to match filter appears at end of list
    }

    TEST_METHOD(TestSyncFilteredItem)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        auto pAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        pAchievement->SetID(1U);
        pAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        pAchievement->SetPoints(5);
        pAchievement->SetName(L"Title");
        pAchievement->SetState(ra::ui::viewmodels::AssetState::Inactive);
        pAchievement->CreateServerCheckpoint();
        pAchievement->CreateLocalCheckpoint();
        auto& vmAchievement = dynamic_cast<ra::ui::viewmodels::AchievementViewModel&>(vmAssetList.Assets().Append(std::move(pAchievement)));

        Assert::AreEqual({ 1U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        const auto* pItem = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pItem != nullptr);

        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(ra::ui::viewmodels::AssetCategory::Core, pItem->GetCategory());
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Title"), pItem->GetLabel());
        Assert::AreEqual(5, pItem->GetPoints());
        Assert::AreEqual(ra::ui::viewmodels::AssetState::Inactive, pItem->GetState());

        vmAchievement.SetName(L"New Title");
        Assert::AreEqual(std::wstring(L"New Title"), pItem->GetLabel());
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, pItem->GetChanges());

        vmAchievement.SetPoints(10);
        Assert::AreEqual(10, pItem->GetPoints());

        vmAchievement.SetState(ra::ui::viewmodels::AssetState::Active);
        Assert::AreEqual(ra::ui::viewmodels::AssetState::Active, pItem->GetState());
    }

    TEST_METHOD(TestUpdateButtonsNoGame)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsFalse(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsFalse(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsFalse(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsFalse(vmAssetList.CanCreate());

        vmAssetList.SetGameId(1U);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SetGameId(0U);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsFalse(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsFalse(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsFalse(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsFalse(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsNoAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsFalse(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsFalse(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsNoSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsInactiveSingleSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsActiveSingleSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsActiveAndInactiveSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsChangeSelectionUpdatesButtons)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true); // active
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true); // inactive (both selected)
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false); // active (only inactive selected)
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(false); // inactive (nothing selected)
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsRemoveItems)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().RemoveAt(2); // remove selected item (which was active), nothing selected
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().RemoveAt(0); // remove unselected item (which was inactive), nothing remaining
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh()); // still has non-visible items
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert()); // still has non-visible items
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1"); // add back one achievement (inactive, unselected)
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsChangeFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.SetFilterCategory(AssetCategory::Unofficial);
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate All"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh All"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert All"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsFalse(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsModifiedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetName(L"Modified");       

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save"), vmAssetList.GetSaveButtonText());
        Assert::IsTrue(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsModifiedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(0)->SetName(L"Modified");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsTrue(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsModifiedChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save"), vmAssetList.GetSaveButtonText());
        Assert::IsTrue(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.Assets().GetItemAt(2)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish"), vmAssetList.GetPublishButtonText());
        Assert::IsTrue(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(0)->SetName(L"Modified");
        vmAssetList.Assets().GetItemAt(0)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsTrue(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish"), vmAssetList.GetPublishButtonText());
        Assert::IsTrue(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());

        vmAssetList.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::AreEqual(std::wstring(L"&Save All"), vmAssetList.GetSaveButtonText());
        Assert::IsFalse(vmAssetList.CanSave());
        Assert::AreEqual(std::wstring(L"&Publish All"), vmAssetList.GetPublishButtonText());
        Assert::IsFalse(vmAssetList.CanPublish());
        Assert::AreEqual(std::wstring(L"&Refresh"), vmAssetList.GetRefreshButtonText());
        Assert::IsTrue(vmAssetList.CanRefresh());
        Assert::AreEqual(std::wstring(L"Re&vert"), vmAssetList.GetRevertButtonText());
        Assert::IsTrue(vmAssetList.CanRevert());
        Assert::IsTrue(vmAssetList.CanClone());
        Assert::IsTrue(vmAssetList.CanCreate());
    }

    TEST_METHOD(TestActivateSelectedSingle)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
    }

    TEST_METHOD(TestActivateSelectedMultiple)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true); // inactive
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true); // active
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
    }

    TEST_METHOD(TestSaveSelectedLocalUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1:Desc1:::::5:::::12345");
    }

    TEST_METHOD(TestSaveSelectedLocalModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Local);
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddNewAchievement(7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<AchievementViewModel*>(vmAssetList.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetPoints(10);
        pItem->SetName(L"Test1b");
        pItem->SetDescription(L"Desc1b");
        pItem->SetBadge(L"54321");
        pItem->SetTrigger("0xH1234=2");

        // when an item is selected, non-selected modified items should be written using their unmodified state
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1:Desc1:::::5:::::12345");
        AssertContains(sText, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());

        // when no items are selected, all items should be committed and written
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false);

        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"0xH1234=2\":Test1b:Desc1b:::::10:::::54321");
        AssertContains(sText2, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        // when an item is selected, non-selected modified items should be written using their unmodified state
        pItem->SetName(L"Test1c");
        auto* pItem2 = dynamic_cast<AchievementViewModel*>(vmAssetList.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        pItem2->SetName(L"Test2b");

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText3 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText3, "1:\"0xH1234=2\":Test1c:Desc1b:::::10:::::54321");
        AssertContains(sText3, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");
    }

    TEST_METHOD(TestSaveSelectedUnofficialUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "1:\"0xH1234=1\":");
    }

    TEST_METHOD(TestSaveSelectedUnofficialModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Unofficial);
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<AchievementViewModel*>(vmAssetList.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetPoints(10);
        pItem->SetName(L"Test1b");
        pItem->SetDescription(L"Desc1b");
        pItem->SetBadge(L"54321");
        pItem->SetTrigger("0xH1234=2");

        // when an item is selected, non-selected modified items should be written using their unmodified state
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "1:\"0xH1234=1\":");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());

        // when no items are selected, all items should be committed and written
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false);

        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"0xH1234=2\":Test1b:Desc1b:::::10:::::54321");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        // when an item is selected, non-selected modified items should be written using their unmodified state
        pItem->SetName(L"Test1c");
        auto* pItem2 = dynamic_cast<AchievementViewModel*>(vmAssetList.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        pItem2->SetName(L"Test2b");

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText3 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText3, "1:\"0xH1234=2\":Test1c:Desc1b:::::10:::::54321");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");
    }

    TEST_METHOD(TestSaveSelectedDiscardCoreChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<AchievementViewModel*>(vmAssetList.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1b:Desc1:::::5:::::12345");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        // change the value back to match the server checkpoint, should no longer appear in local file
        pItem->SetName(L"Test1");
        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText2, "1:\"0xH1234=1\":");
        AssertDoesNotContain(sText2, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
    }

    TEST_METHOD(TestDoFrameUpdatesAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pAchievement = vmAssetList.FindAchievement(1U);
        Expects(pAchievement != nullptr);
        pAchievement->Activate();
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        auto* pTrigger = vmAssetList.mockRuntime.GetAchievementTrigger(1U);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        vmAssetList.DoFrame();
        Assert::AreEqual(AssetState::Active, pAchievement->GetState());
    }

    TEST_METHOD(TestCreateNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockGameContext.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Core, vmAssetList.GetFilterCategory());

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;

        // second new Local achievement should be focused, deselecting the first
        vmAssetList.CreateNew();
        Assert::AreEqual({ 4U }, vmAssetList.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
