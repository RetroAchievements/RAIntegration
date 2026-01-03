#include "CppUnitTest.h"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\MemoryRegionsViewModel.hh"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemoryRegionsViewModel_Tests)
{
private:
    class MemoryRegionsViewModelHarness : public MemoryRegionsViewModel
    {
    public:
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        ra::data::models::MemoryRegionsModel& MemoryRegionsModel()
        {
            auto* pModel = mockGameContext.Assets().FindMemoryRegions();
            if (!pModel)
            {
                auto pNewMemoryRegions = std::make_unique<ra::data::models::MemoryRegionsModel>();
                pModel = dynamic_cast<ra::data::models::MemoryRegionsModel*>(&mockGameContext.Assets().Append(std::move(pNewMemoryRegions)));
            }
            return *pModel;
        }

        void MockRegions()
        {
            mockConsoleContext.AddMemoryRegion(0x0000, 0x1FFF, ra::data::MemoryRegion::Type::SystemRAM, L"System RAM");
            mockConsoleContext.AddMemoryRegion(0x2000, 0x3FFF, ra::data::MemoryRegion::Type::VideoRAM, L"Video RAM");
            mockConsoleContext.AddMemoryRegion(0x4000, 0x7FFF, ra::data::MemoryRegion::Type::SaveRAM, L"Cartridge RAM");
            InitializeRegions();
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryRegionsViewModelHarness regions;
        regions.InitializeRegions();

        Assert::AreEqual({ 0U }, regions.Regions().Count());
        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestInitializeRegions)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MockRegions();

        Assert::AreEqual({ 2U }, regions.Regions().Count());

        Assert::AreEqual(std::wstring(L"0x0000-0x1fff"), regions.Regions().GetItemAt(0)->GetRange());
        Assert::AreEqual(std::wstring(L"System RAM"), regions.Regions().GetItemAt(0)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x4000-0x7fff"), regions.Regions().GetItemAt(1)->GetRange());
        Assert::AreEqual(std::wstring(L"Cartridge RAM"), regions.Regions().GetItemAt(1)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsCustom());

        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestInitializeRegionsWithCustom)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MemoryRegionsModel().AddCustomRegion(0x1200, 0x12FF, L"My region");
        regions.MockRegions();

        Assert::AreEqual({ 3U }, regions.Regions().Count());

        Assert::AreEqual(std::wstring(L"0x0000-0x1fff"), regions.Regions().GetItemAt(0)->GetRange());
        Assert::AreEqual(std::wstring(L"System RAM"), regions.Regions().GetItemAt(0)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x4000-0x7fff"), regions.Regions().GetItemAt(1)->GetRange());
        Assert::AreEqual(std::wstring(L"Cartridge RAM"), regions.Regions().GetItemAt(1)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x1200-0x12ff"), regions.Regions().GetItemAt(2)->GetRange());
        Assert::AreEqual(std::wstring(L"My region"), regions.Regions().GetItemAt(2)->GetLabel());
        Assert::IsTrue(regions.Regions().GetItemAt(2)->IsCustom());

        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestAddNewRegion)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MockRegions();

        Assert::AreEqual({ 2U }, regions.Regions().Count());

        regions.AddNewRegion();

        Assert::AreEqual({ 3U }, regions.Regions().Count());

        Assert::AreEqual(std::wstring(L"0x0000-0x1fff"), regions.Regions().GetItemAt(0)->GetRange());
        Assert::AreEqual(std::wstring(L"System RAM"), regions.Regions().GetItemAt(0)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x4000-0x7fff"), regions.Regions().GetItemAt(1)->GetRange());
        Assert::AreEqual(std::wstring(L"Cartridge RAM"), regions.Regions().GetItemAt(1)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x0000-0x0000"), regions.Regions().GetItemAt(2)->GetRange());
        Assert::AreEqual(std::wstring(L"New Custom Region"), regions.Regions().GetItemAt(2)->GetLabel());
        Assert::IsTrue(regions.Regions().GetItemAt(2)->IsCustom());

        Assert::AreEqual(2, regions.GetSelectedRegionIndex());
        Assert::IsTrue(regions.CanRemove());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestRemoveRegion)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MemoryRegionsModel().AddCustomRegion(0x1200, 0x12FF, L"My region");
        regions.MockRegions();

        Assert::AreEqual({ 3U }, regions.Regions().Count());
        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());

        regions.SetSelectedRegionIndex(1);
        Assert::AreEqual(1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());
        regions.RemoveRegion();
        Assert::AreEqual({ 3U }, regions.Regions().Count());
        Assert::AreEqual(1, regions.GetSelectedRegionIndex());

        regions.SetSelectedRegionIndex(2);
        Assert::AreEqual(2, regions.GetSelectedRegionIndex());
        Assert::IsTrue(regions.CanRemove());
        regions.RemoveRegion();

        Assert::AreEqual({ 2U }, regions.Regions().Count());

        Assert::AreEqual(std::wstring(L"0x0000-0x1fff"), regions.Regions().GetItemAt(0)->GetRange());
        Assert::AreEqual(std::wstring(L"System RAM"), regions.Regions().GetItemAt(0)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsCustom());

        Assert::AreEqual(std::wstring(L"0x4000-0x7fff"), regions.Regions().GetItemAt(1)->GetRange());
        Assert::AreEqual(std::wstring(L"Cartridge RAM"), regions.Regions().GetItemAt(1)->GetLabel());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsCustom());

        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.CanRemove());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestSelectedRegionIndex)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MockRegions();

        Assert::AreEqual({ 2U }, regions.Regions().Count());

        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsSelected());

        regions.SetSelectedRegionIndex(0);
        Assert::AreEqual(0, regions.GetSelectedRegionIndex());
        Assert::IsTrue(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsSelected());

        regions.SetSelectedRegionIndex(1);
        Assert::AreEqual(1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(regions.Regions().GetItemAt(1)->IsSelected());

        regions.SetSelectedRegionIndex(-1);
        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsSelected());

        regions.Regions().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(0, regions.GetSelectedRegionIndex());
        Assert::IsTrue(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsSelected());

        regions.Regions().GetItemAt(1)->SetSelected(true);
        Assert::AreEqual(1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(regions.Regions().GetItemAt(1)->IsSelected());

        regions.Regions().GetItemAt(1)->SetSelected(false);
        Assert::AreEqual(-1, regions.GetSelectedRegionIndex());
        Assert::IsFalse(regions.Regions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(regions.Regions().GetItemAt(1)->IsSelected());
    }

    TEST_METHOD(TestInvalidRange)
    {
        MemoryRegionsViewModelHarness regions;
        regions.MemoryRegionsModel().AddCustomRegion(0x1200, 0x12FF, L"My region");
        regions.MockRegions();

        Assert::AreEqual({ 3U }, regions.Regions().Count());

        auto& pRegion = *regions.Regions().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x1200-0x12ff"), pRegion.GetRange());
        Assert::AreEqual(std::wstring(L"My region"), pRegion.GetLabel());
        Assert::IsTrue(pRegion.IsCustom());
        Assert::IsFalse(pRegion.IsInvalid());

        pRegion.SetRange(L"banana");
        Assert::IsTrue(pRegion.IsInvalid());
        Assert::IsFalse(regions.CanSave());

        pRegion.SetLabel(L"Banana");
        Assert::IsTrue(pRegion.IsInvalid());
        Assert::IsFalse(regions.CanSave());

        pRegion.SetRange(L"0-$ff");
        Assert::IsFalse(pRegion.IsInvalid());
        Assert::IsTrue(regions.CanSave());
    }

    TEST_METHOD(TestSaveChanges)
    {
        MemoryRegionsViewModelHarness regions;
        regions.mockGameContext.SetGameId(1);
        regions.mockGameContext.SetGameTitle(L"Game");
        regions.MemoryRegionsModel().AddCustomRegion(0x1200, 0x12FF, L"My region");
        regions.MockRegions();

        Assert::AreEqual({ 3U }, regions.Regions().Count());

        auto& pRegion = *regions.Regions().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x1200-0x12ff"), pRegion.GetRange());
        Assert::AreEqual(std::wstring(L"My region"), pRegion.GetLabel());
        pRegion.SetLabel(L"Banana");
        pRegion.SetRange(L"0-$ff");

        regions.SetDialogResult(ra::ui::DialogResult::OK);

        Assert::IsTrue(regions.mockLocalStorage.HasStoredData(ra::services::StorageItemType::UserAchievements, L"1"));
        Assert::AreEqual(std::string(
            "0.0.0.0\n"
            "Game\n"
            "M0:0x0000-0x00ff:\"Banana\"\n"
        ), regions.mockLocalStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, L"1"));
    }

    TEST_METHOD(TestDiscardChanges)
    {
        MemoryRegionsViewModelHarness regions;
        regions.mockGameContext.SetGameId(1);
        regions.mockGameContext.SetGameTitle(L"Game");
        regions.MemoryRegionsModel().AddCustomRegion(0x1200, 0x12FF, L"My region");
        regions.MockRegions();

        Assert::AreEqual({ 3U }, regions.Regions().Count());

        auto& pRegion = *regions.Regions().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x1200-0x12ff"), pRegion.GetRange());
        Assert::AreEqual(std::wstring(L"My region"), pRegion.GetLabel());
        pRegion.SetLabel(L"Banana");
        pRegion.SetRange(L"0-$ff");

        regions.SetDialogResult(ra::ui::DialogResult::Cancel);

        Assert::IsFalse(regions.mockLocalStorage.HasStoredData(ra::services::StorageItemType::UserAchievements, L"1"));
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
