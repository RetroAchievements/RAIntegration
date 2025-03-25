#include "CppUnitTest.h"

#include "ui\viewmodels\BrokenAchievementsViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::ui::viewmodels::MessageBoxViewModel;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(BrokenAchievementsViewModel_Tests)
{
private:
    class BrokenAchievementsViewModelHarness : public BrokenAchievementsViewModel
    {
    public:
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockThreadPool mockThreadPool; // needed by asset list
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        BrokenAchievementsViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockWindowManager.AssetList.InitializeNotifyTargets();
            mockConfiguration.SetHostUrl("http://host");
        }

        void MockAchievements()
        {
            mockGameContext.SetGameId(1U);
            mockGameContext.SetGameTitle(L"GAME");
            mockGameContext.SetGameHash("HASH");

            auto& ach1 = mockGameContext.Assets().NewAchievement();
            ach1.SetCategory(ra::data::models::AssetCategory::Core);
            ach1.SetID(1U);
            ach1.SetName(L"Title1");
            ach1.SetState(ra::data::models::AssetState::Inactive);
            auto& ach2 = mockGameContext.Assets().NewAchievement();
            ach2.SetCategory(ra::data::models::AssetCategory::Core);
            ach2.SetID(2U);
            ach2.SetName(L"Title2");
            ach2.SetState(ra::data::models::AssetState::Active);
            auto& ach3 = mockGameContext.Assets().NewAchievement();
            ach3.SetCategory(ra::data::models::AssetCategory::Core);
            ach3.SetID(3U);
            ach3.SetName(L"Title3");
            ach3.SetState(ra::data::models::AssetState::Inactive);
            auto& ach4 = mockGameContext.Assets().NewAchievement();
            ach4.SetCategory(ra::data::models::AssetCategory::Core);
            ach4.SetID(4U);
            ach4.SetName(L"Title4");
            ach4.SetState(ra::data::models::AssetState::Active);
            auto& ach5 = mockGameContext.Assets().NewAchievement();
            ach5.SetCategory(ra::data::models::AssetCategory::Core);
            ach5.SetID(5U);
            ach5.SetName(L"Title5");
            ach5.SetState(ra::data::models::AssetState::Active);
            auto& ach6 = mockGameContext.Assets().NewAchievement();
            ach6.SetCategory(ra::data::models::AssetCategory::Core);
            ach6.SetID(6U);
            ach6.SetName(L"Title6");
            ach6.SetState(ra::data::models::AssetState::Active);

            Assert::IsTrue(InitializeAchievements());
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        Assert::AreEqual(-1, vmBrokenAchievements.GetSelectedIndex());
        Assert::AreEqual({ 0U }, vmBrokenAchievements.Achievements().Count());
    }

    TEST_METHOD(TestInitializeAchievementsNoGameLoaded)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You must load a game before you can report achievement problems."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsLocalAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Local);
        vmBrokenAchievements.mockWindowManager.AssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report local achievement problems."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsCompatibilityTestMode)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report achievement problems in compatibility test mode."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsNoAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"There are no active achievements to report."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitNoAchievementsSelected)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Please select an achievement."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(std::string(), vmBrokenAchievements.mockDesktop.LastOpenedUrl());

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitAchievement)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsFalse(vmBrokenAchievements.mockDesktop.WasDialogShown());

        Assert::AreEqual(std::string("http://host/achievement/3/report-issue"),
                         vmBrokenAchievements.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestSubmitAchievementWithRichPresence)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);

        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(1U)->SetUnlockRichPresence(L"Nowhere, 2 Lives");

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsFalse(vmBrokenAchievements.mockDesktop.WasDialogShown());

        Assert::AreEqual(std::string("http://host/achievement/1/report-issue?extra=eyJ0cmlnZ2VyUmljaFByZXNlbmNlIjoiTm93aGVyZSwgMiBMaXZlcyJ9"),
                         vmBrokenAchievements.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestMultiSelect)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();

        Assert::AreEqual(-1, vmBrokenAchievements.GetSelectedIndex());

        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        Assert::AreEqual(2, vmBrokenAchievements.GetSelectedIndex());

        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        Assert::AreEqual(4, vmBrokenAchievements.GetSelectedIndex());

        Assert::IsFalse(vmBrokenAchievements.Achievements().GetItemAt(2)->IsSelected());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
