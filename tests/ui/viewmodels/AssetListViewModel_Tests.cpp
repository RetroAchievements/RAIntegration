#include "CppUnitTest.h"

#include "ui\viewmodels\AssetListViewModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"

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

        auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(5);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(10);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(20);
        vmAssetList.Assets().Append(std::move(vmAchievement));

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

        auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Unofficial);
        vmAchievement->SetPoints(5);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(10);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Unofficial);
        vmAchievement->SetPoints(20);
        vmAssetList.Assets().Append(std::move(vmAchievement));

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

        auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Local);
        vmAchievement->SetPoints(5);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(10);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Local);
        vmAchievement->SetPoints(20);
        vmAssetList.Assets().Append(std::move(vmAchievement));

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

        auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        vmAchievement->SetPoints(5);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Local);
        vmAchievement->SetPoints(10);
        vmAssetList.Assets().Append(std::move(vmAchievement));

        vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Unofficial);
        vmAchievement->SetPoints(20);
        vmAssetList.Assets().Append(std::move(vmAchievement));

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
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
