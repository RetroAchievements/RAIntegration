#include "CppUnitTest.h"

#include "ui\viewmodels\AssetListViewModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AssetType;
using ra::data::models::AssetCategory;
using ra::data::models::AssetState;
using ra::data::models::AssetChanges;
using ra::ui::viewmodels::MessageBoxViewModel;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetListViewModel_Tests)
{
private:
    enum class ActivateButtonState
    {
        Activate,
        Deactivate,
        ActivateAll,
        Disabled
    };

    enum class SaveButtonState
    {
        Save,
        Publish,
        SaveDisabled,
        SaveAll,
        PublishAll,
        SaveAllDisabled
    };

    enum class ResetButtonState
    {
        Reset,
        ResetAll,
        Disabled
    };

    enum class RevertButtonState
    {
        Revert,
        RevertAll,
        Delete,
        Disabled
    };

    enum class CreateButtonState
    {
        Enabled,
        Disabled
    };

    enum class CloneButtonState
    {
        Enabled,
        Disabled
    };

    class AssetListViewModelHarness : public AssetListViewModel
    {
    public:
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::ui::mocks::MockDesktop mockDesktop;
        mocks::MockWindowManager mockWindowManager;

        AssetListViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 InitializeNotifyTargets();
        }

        void AssertButtonState(ActivateButtonState nActivateButtonState)
        {
            switch (nActivateButtonState)
            {
                case ActivateButtonState::Activate:
                    Assert::AreEqual(std::wstring(L"&Activate"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::Deactivate:
                    Assert::AreEqual(std::wstring(L"De&activate"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::ActivateAll:
                    Assert::AreEqual(std::wstring(L"&Activate All"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::Disabled:
                    Assert::AreEqual(std::wstring(L"&Activate All"), GetValue(ActivateButtonTextProperty));
                    Assert::IsFalse(CanActivate());
                    break;
            }
        }

        void AssertButtonState(SaveButtonState nSaveButtonState)
        {
            switch (nSaveButtonState)
            {
                case SaveButtonState::Save:
                    Assert::AreEqual(std::wstring(L"&Save"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::Publish:
                    Assert::AreEqual(std::wstring(L"Publi&sh"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::SaveDisabled:
                    Assert::AreEqual(std::wstring(L"&Save"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::SaveAll:
                    Assert::AreEqual(std::wstring(L"&Save All"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::PublishAll:
                    Assert::AreEqual(std::wstring(L"Publi&sh All"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::SaveAllDisabled:
                    Assert::AreEqual(std::wstring(L"&Save All"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;
            }
        }

        void AssertButtonState(ResetButtonState nResetButtonState)
        {
            switch (nResetButtonState)
            {
                case ResetButtonState::Reset:
                    Assert::AreEqual(std::wstring(L"&Reset"), GetValue(ResetButtonTextProperty));
                    Assert::IsTrue(CanReset());
                    break;

                case ResetButtonState::ResetAll:
                    Assert::AreEqual(std::wstring(L"&Reset All"), GetValue(ResetButtonTextProperty));
                    Assert::IsTrue(CanReset());
                    break;

                case ResetButtonState::Disabled:
                    Assert::AreEqual(std::wstring(L"&Reset All"), GetValue(ResetButtonTextProperty));
                    Assert::IsFalse(CanReset());
                    break;
            }
        }

        void AssertButtonState(RevertButtonState nRevertButtonState)
        {
            switch (nRevertButtonState)
            {
                case RevertButtonState::Revert:
                    Assert::AreEqual(std::wstring(L"Re&vert"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::RevertAll:
                    Assert::AreEqual(std::wstring(L"Re&vert All"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::Delete:
                    Assert::AreEqual(std::wstring(L"&Delete"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::Disabled:
                    Assert::AreEqual(std::wstring(L"Re&vert All"), GetValue(RevertButtonTextProperty));
                    Assert::IsFalse(CanRevert());
                    break;
            }
        }

        void AssertButtonState(CreateButtonState nCreateButtonState)
        {
            switch (nCreateButtonState)
            {
                case CreateButtonState::Enabled:
                    Assert::IsTrue(CanCreate());
                    break;

                case CreateButtonState::Disabled:
                    Assert::IsFalse(CanCreate());
                    break;
            }
        }

        void AssertButtonState(CloneButtonState nCloneButtonState)
        {
            switch (nCloneButtonState)
            {
                case CloneButtonState::Enabled:
                    Assert::IsTrue(CanClone());
                    break;

                case CloneButtonState::Disabled:
                    Assert::IsFalse(CanClone());
                    break;
            }
        }

        void AssertButtonState(ActivateButtonState nActivateButtonState,
            SaveButtonState nSaveButtonState, ResetButtonState nResetButtonState,
            RevertButtonState nRevertButtonState, CreateButtonState nCreateButtonState,
            CloneButtonState nCloneButtonState)
        {
            AssertButtonState(nActivateButtonState);
            AssertButtonState(nSaveButtonState);
            AssertButtonState(nResetButtonState);
            AssertButtonState(nRevertButtonState);
            AssertButtonState(nCreateButtonState);
            AssertButtonState(nCloneButtonState);
        }

        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle)
        {
            auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(vmAchievement));
        }

        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(vmAchievement));
        }

        void AddNewAchievement(unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(AssetCategory::Local);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateLocalCheckpoint();
            vmAchievement->SetNew();
            mockGameContext.Assets().Append(std::move(vmAchievement));
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

        void MockGameId(unsigned int nGameId)
        {
            mockGameContext.SetGameId(nGameId);
            mockGameContext.NotifyActiveGameChanged();
        }

        const std::string& GetUserFile(const std::wstring& sGameId)
        {
            return mockLocalStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, sGameId);
        }

        void MockUserFile(const std::string& sContents)
        {
            return mockLocalStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, std::to_wstring(GetGameId()), sContents);
        }

        void MockUserFileContents(const std::string& sContents)
        {
            MockUserFile("0.0.0.0\nGameName\n" + sContents);
        }

        void Publish(std::vector<ra::data::models::AssetModelBase*>& vAssets) override
        {
            PublishedAssets = vAssets;

            for (auto* pAsset : vAssets)
                pAsset->UpdateServerCheckpoint();
        }

        std::vector<ra::data::models::AssetModelBase*> PublishedAssets;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AssetListViewModelHarness vmAssetList;

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
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

        Assert::AreEqual({ 4U }, vmAssetList.Changes().Count());
        Assert::AreEqual((int)AssetChanges::None, vmAssetList.Changes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), vmAssetList.Changes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Modified, vmAssetList.Changes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Modified"), vmAssetList.Changes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Unpublished, vmAssetList.Changes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Unpublished"), vmAssetList.Changes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetChanges::New, vmAssetList.Changes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"New"), vmAssetList.Changes().GetItemAt(3)->GetLabel());
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

        vmAssetList.mockGameContext.Assets().RemoveAt(1);

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

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

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

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

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

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

        auto ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(50);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(40);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
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

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
    }

    TEST_METHOD(TestAddItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.mockGameContext.Assets().BeginUpdate();

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().EndUpdate();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
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

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestRemoveItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().BeginUpdate();
        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().EndUpdate();

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestChangeItemForFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetCategory(AssetCategory::Unofficial);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().GetItemAt(1)->SetCategory(AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(1)->GetId()); // item changed to match filter appears at end of list

        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetCategory(AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
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

        auto pAchievement = std::make_unique<ra::data::models::AchievementModel>();
        pAchievement->SetID(1U);
        pAchievement->SetCategory(AssetCategory::Core);
        pAchievement->SetPoints(5);
        pAchievement->SetName(L"Title");
        pAchievement->SetState(AssetState::Inactive);
        pAchievement->CreateServerCheckpoint();
        pAchievement->CreateLocalCheckpoint();
        auto& vmAchievement = dynamic_cast<ra::data::models::AchievementModel&>(vmAssetList.mockGameContext.Assets().Append(std::move(pAchievement)));

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        const auto* pItem = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pItem != nullptr);

        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(AssetCategory::Core, pItem->GetCategory());
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Title"), pItem->GetLabel());
        Assert::AreEqual(5, pItem->GetPoints());
        Assert::AreEqual(AssetState::Inactive, pItem->GetState());

        vmAchievement.SetName(L"New Title");
        Assert::AreEqual(std::wstring(L"New Title"), pItem->GetLabel());
        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());

        vmAchievement.SetPoints(10);
        Assert::AreEqual(10, pItem->GetPoints());

        vmAchievement.SetState(AssetState::Active);
        Assert::AreEqual(AssetState::Active, pItem->GetState());
    }

    TEST_METHOD(TestUpdateButtonsNoGame)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Disabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::Disabled, RevertButtonState::Disabled,
            CreateButtonState::Disabled, CloneButtonState::Disabled
        );

        vmAssetList.SetGameId(1U);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SetGameId(0U);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Disabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::Disabled, RevertButtonState::Disabled,
            CreateButtonState::Disabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsNoAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Disabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
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

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
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

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsActiveSingleSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsActiveAndInactiveSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsChangeSelectionUpdatesButtons)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true); // active
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true); // inactive (both selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false); // active (only inactive selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(false); // inactive (nothing selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsRemoveItems)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().RemoveAt(2); // remove selected item (which was active), nothing selected
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.mockGameContext.Assets().RemoveAt(0); // remove unselected item (which was inactive), nothing remaining
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Disabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1"); // add back one achievement (inactive, unselected)
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsChangeFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.SetFilterCategory(AssetCategory::Unofficial);
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsModifiedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");       

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsModifiedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetName(L"Modified");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        // single unmodified selected record, show save button instead of save all, but disable it
        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
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

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsNewSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Local);
        vmAssetList.AddNewAchievement(5, L"Ach1", L"Test1", L"12345", "0xH1234=1");
        vmAssetList.AddNewAchievement(10, L"Ach2", L"Test2", L"12345", "0xH1234=2");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::New, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::Reset, RevertButtonState::Delete,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Publish,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetFilterCategory(AssetCategory::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
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

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Publish,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::SaveDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
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
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

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
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1:Desc1:::::5:::::12345");
    }

    TEST_METHOD(TestSaveSelectedLocalModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Local);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddNewAchievement(7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetPoints(10);
        pItem->SetName(L"Test1b");
        pItem->SetDescription(L"Desc1b");
        pItem->SetBadge(L"54321");
        pItem->SetTrigger("0xH1234=2");

        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);

        // when an item is selected, non-selected modified items should be written using their unmodified state
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "Test1");
        AssertContains(sText, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        // item 2 was saved, but item 1 should still be new
        Assert::AreEqual(AssetChanges::New, pItem->GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        // deselect all. when no items are selected, all items should be committed and written
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false);

        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"0xH1234=2\":Test1b:Desc1b:::::10:::::54321");
        AssertContains(sText2, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        // when an item is selected, non-selected modified items should be written using their unmodified state
        pItem->SetName(L"Test1c");
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
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "1:\"0xH1234=1\":");
    }

    TEST_METHOD(TestSaveSelectedUnofficialModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetFilterCategory(AssetCategory::Unofficial);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
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
        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
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
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
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

    TEST_METHOD(TestSaveSelectedPublishCoreModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveDisabled);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreModifiedCancel)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::No;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 0U }, vmAssetList.PublishedAssets.size());
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test3", L"Desc3", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 3 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        pItem1->SetName(L"Test1b");
        pItem1->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem1->GetChanges());

        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        pItem2->SetName(L"Test2b");
        pItem2->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
        Expects(pItem3 != nullptr);
        pItem3->SetName(L"Test3b");
        pItem3->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::PublishAll);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 3U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::IsTrue(pItem2 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(1)));
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(2)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreAllOnlyUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test3", L"Desc3", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 2 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        pItem1->SetName(L"Test1b");
        pItem1->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem1->GetChanges());

        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());

        auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
        Expects(pItem3 != nullptr);
        pItem3->SetName(L"Test3b");
        pItem3->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::PublishAll);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 2U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(1)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPublishLocal)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.mockGameContext.SetGameTitle(L"GameName");
        vmAssetList.SetFilterCategory(AssetCategory::Local);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        std::vector<ra::data::models::AssetModelBase*> vEmptyList;
        vmAssetList.mockGameContext.Assets().ReloadAssets(vEmptyList);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveDisabled);

        // local file should be updated without the local achievement
        const auto& sText = vmAssetList.GetUserFile(L"22");
        Assert::AreEqual(std::string("0.0.0.0\nGameName\n"), sText);
    }

    TEST_METHOD(TestDoFrameUpdatesAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(1U);
        Expects(pAchievement != nullptr);
        pAchievement->Activate();
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        auto* pTrigger = vmAssetList.mockRuntime.GetAchievementTrigger(1U);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        vmAssetList.mockGameContext.DoFrame();
        Assert::AreEqual(AssetState::Active, pAchievement->GetState());
    }

    TEST_METHOD(TestCreateNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
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
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;

        // second new Local achievement should be focused, deselecting the first
        vmAssetList.CreateNew();
        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
    }

    TEST_METHOD(TestCloneSingle)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Core, vmAssetList.GetFilterCategory());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        const auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc2"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"11111"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1111=1"), pAchievement->GetTrigger());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;

        // copying the copy should create another copy, deselecting the first
        vmAssetList.CloneSelected();
        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
    }

    TEST_METHOD(TestCloneMultiple)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 9, L"Test3", L"Desc3", L"12321", "0xH5342=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 11, L"Test4", L"Desc4", L"55555", "0xHface=1");

        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 4U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Core, vmAssetList.GetFilterCategory());

        vmAssetList.FilteredAssets().GetItemAt(3)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.CloneSelected();

        // new Local achievements should be created and focused
        Assert::AreEqual({ 6U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetCategory::Local, vmAssetList.GetFilterCategory());

        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc2"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"11111"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1111=1"), pAchievement->GetTrigger());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test4 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000002U }, pAsset->GetId());
        Assert::AreEqual(11, pAsset->GetPoints());

        pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc4"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"55555"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xHface=1"), pAchievement->GetTrigger());

        // editor is not shown when multiple assets are cloned
        Assert::IsFalse(vmAssetList.mockDesktop.WasDialogShown());

        // both items should be selected
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
    }

    TEST_METHOD(TestResetSelectedAllUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"This will discard all unsaved changes and reset the assets to the last locally saved state."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"This will discard any changes to the selected assets and reset them to the last locally saved state."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedUnmodifiedFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedAbort)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0xH2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::No;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        Assert::AreEqual(std::string("0xH2345=1"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");
        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetNew();

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // item marked as new should be removed, and not restored from the file.
        // NOTE: new items should not be in the file. this is mostly just testing the removal of new items.
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 1U }));
    }

    TEST_METHOD(TestResetSelectedAllNewFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // new item will be added when there is no selection.
        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedNewFromFileWithSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // new item will be ignored when a selection is reset.
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 111000001U }));
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
