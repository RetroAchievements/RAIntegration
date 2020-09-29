#include "CppUnitTest.h"

#include "ui\viewmodels\AssetEditorViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetEditorViewModel_Tests)
{
private:
    class AssetEditorViewModelHarness : public AssetEditorViewModel
    {
    public:
        AssetEditorViewModelHarness() = default;
        ~AssetEditorViewModelHarness()
        {
            // prevent exception if asset is destroyed before harness
            m_pAsset = nullptr;
        }

        AssetEditorViewModelHarness(const AssetEditorViewModelHarness&) noexcept = delete;
        AssetEditorViewModelHarness& operator=(const AssetEditorViewModelHarness&) noexcept = delete;
        AssetEditorViewModelHarness(AssetEditorViewModelHarness&&) noexcept = delete;
        AssetEditorViewModelHarness& operator=(AssetEditorViewModelHarness&&) noexcept = delete;
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        AssetEditorViewModelHarness editor;

        Assert::AreEqual(std::wstring(L"Asset Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Asset Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an asset from the Assets List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
    }

    TEST_METHOD(TestLoadAchievement)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetName(L"Test Achievement");
        achievement.SetID(1234U);
        achievement.SetState(AssetState::Active);
        achievement.SetDescription(L"Do something cool");
        achievement.SetCategory(AssetCategory::Unofficial);
        achievement.SetPoints(10);
        achievement.SetBadge(L"58329");

        editor.LoadAsset(&achievement);

        Assert::AreEqual(std::wstring(L"Achievement Editor"), editor.GetWindowTitle());
        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Do something cool"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsTrue(editor.IsAssetLoaded());

        editor.LoadAsset(nullptr);

        Assert::AreEqual(std::wstring(L"Asset Editor"), editor.GetWindowTitle());
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(std::wstring(L"[No Asset Loaded]"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Open an asset from the Assets List"), editor.GetDescription());
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetState::Inactive, editor.GetState());
        Assert::AreEqual(0, editor.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), editor.GetBadge());
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(editor.IsAssetLoaded());
    }

    TEST_METHOD(TestSyncId)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetID(1234U);

        editor.LoadAsset(&achievement);
        Assert::AreEqual(1234U, editor.GetID());
        Assert::AreEqual(1234U, achievement.GetID());

        achievement.SetID(2345U);
        Assert::AreEqual(2345U, editor.GetID());
        Assert::AreEqual(2345U, achievement.GetID());
    }

    TEST_METHOD(TestLocalIdIgnored)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetCategory(AssetCategory::Local);
        achievement.SetID(11100002U);

        editor.LoadAsset(&achievement);
        Assert::AreEqual(0U, editor.GetID());
        Assert::AreEqual(11100002U, achievement.GetID());

        achievement.SetID(2345U);
        Assert::AreEqual(2345U, editor.GetID());
        Assert::AreEqual(2345U, achievement.GetID());
    }

    TEST_METHOD(TestSyncName)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetName(L"Test Achievement");

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Test Achievement"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Test Achievement"), achievement.GetName());

        editor.SetName(L"New Name");
        Assert::AreEqual(std::wstring(L"New Name"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement.GetName());

        editor.SetName(L"New Name 2");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement.GetName());

        achievement.SetName(L"New Name 3");
        Assert::AreEqual(std::wstring(L"New Name 3"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 3"), achievement.GetName());
    }

    TEST_METHOD(TestSyncDescription)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetDescription(L"Do something cool");

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"Do something cool"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"Do something cool"), achievement.GetDescription());

        editor.SetDescription(L"New description");
        Assert::AreEqual(std::wstring(L"New description"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description"), achievement.GetDescription());

        editor.SetDescription(L"New description 2");
        Assert::AreEqual(std::wstring(L"New description 2"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description 2"), achievement.GetDescription());

        achievement.SetDescription(L"New description 3");
        Assert::AreEqual(std::wstring(L"New description 3"), editor.GetDescription());
        Assert::AreEqual(std::wstring(L"New description 3"), achievement.GetDescription());
    }

    TEST_METHOD(TestSyncState)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetState(AssetState::Active);

        editor.LoadAsset(&achievement);
        Assert::AreEqual(AssetState::Active, editor.GetState());
        Assert::AreEqual(AssetState::Active, achievement.GetState());

        editor.SetState(AssetState::Paused);
        Assert::AreEqual(AssetState::Paused, editor.GetState());
        Assert::AreEqual(AssetState::Paused, achievement.GetState());

        editor.SetState(AssetState::Triggered);
        Assert::AreEqual(AssetState::Triggered, editor.GetState());
        Assert::AreEqual(AssetState::Triggered, achievement.GetState());

        achievement.SetState(AssetState::Waiting);
        Assert::AreEqual(AssetState::Waiting, editor.GetState());
        Assert::AreEqual(AssetState::Waiting, achievement.GetState());
    }

    TEST_METHOD(TestSyncCategory)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetCategory(AssetCategory::Unofficial);

        editor.LoadAsset(&achievement);
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Unofficial, achievement.GetCategory());

        editor.SetCategory(AssetCategory::Core);
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Core, achievement.GetCategory());

        editor.SetCategory(AssetCategory::Unofficial);
        Assert::AreEqual(AssetCategory::Unofficial, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Unofficial, achievement.GetCategory());

        achievement.SetCategory(AssetCategory::Core);
        Assert::AreEqual(AssetCategory::Core, editor.GetCategory());
        Assert::AreEqual(AssetCategory::Core, achievement.GetCategory());
    }

    TEST_METHOD(TestSyncPoints)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetPoints(10);

        editor.LoadAsset(&achievement);
        Assert::AreEqual(10, editor.GetPoints());
        Assert::AreEqual(10, achievement.GetPoints());

        editor.SetPoints(25);
        Assert::AreEqual(25, editor.GetPoints());
        Assert::AreEqual(25, achievement.GetPoints());

        editor.SetPoints(5);
        Assert::AreEqual(5, editor.GetPoints());
        Assert::AreEqual(5, achievement.GetPoints());

        editor.SetPoints(50);
        Assert::AreEqual(50, editor.GetPoints());
        Assert::AreEqual(50, achievement.GetPoints());
    }

    TEST_METHOD(TestSyncBadge)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetBadge(L"58329");

        editor.LoadAsset(&achievement);
        Assert::AreEqual(std::wstring(L"58329"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"58329"), achievement.GetBadge());

        editor.SetBadge(L"12345");
        Assert::AreEqual(std::wstring(L"12345"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"12345"), achievement.GetBadge());

        editor.SetBadge(L"34567");
        Assert::AreEqual(std::wstring(L"34567"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"34567"), achievement.GetBadge());

        achievement.SetBadge(L"67890");
        Assert::AreEqual(std::wstring(L"67890"), editor.GetBadge());
        Assert::AreEqual(std::wstring(L"67890"), achievement.GetBadge());
    }

    TEST_METHOD(TestSyncPauseOnReset)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetPauseOnReset(true);

        editor.LoadAsset(&achievement);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::IsTrue(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(true);
        Assert::IsTrue(editor.IsPauseOnReset());
        Assert::IsTrue(achievement.IsPauseOnReset());

        editor.SetPauseOnReset(false);
        Assert::IsFalse(editor.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnReset());
    }

    TEST_METHOD(TestSyncPauseOnTrigger)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement;
        achievement.SetPauseOnTrigger(true);

        editor.LoadAsset(&achievement);
        Assert::IsTrue(editor.IsPauseOnTrigger());
        Assert::IsTrue(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(false);
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(true);
        Assert::IsTrue(editor.IsPauseOnTrigger());
        Assert::IsTrue(achievement.IsPauseOnTrigger());

        editor.SetPauseOnTrigger(false);
        Assert::IsFalse(editor.IsPauseOnTrigger());
        Assert::IsFalse(achievement.IsPauseOnTrigger());
    }

    TEST_METHOD(TestSyncOneRecordAtATime)
    {
        AssetEditorViewModelHarness editor;
        AchievementViewModel achievement1;
        achievement1.SetName(L"Achievement1");
        AchievementViewModel achievement2;
        achievement2.SetName(L"Achievement2");

        editor.LoadAsset(&achievement1);
        Assert::AreEqual(std::wstring(L"Achievement1"), editor.GetName());
        Assert::AreEqual(std::wstring(L"Achievement1"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.SetName(L"New Name");
        Assert::AreEqual(std::wstring(L"New Name"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.LoadAsset(&achievement2);
        Assert::AreEqual(std::wstring(L"Achievement2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"Achievement2"), achievement2.GetName());

        editor.SetName(L"New Name 2");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement2.GetName());

        achievement1.SetName(L"New Name 3");
        Assert::AreEqual(std::wstring(L"New Name 2"), editor.GetName());
        Assert::AreEqual(std::wstring(L"New Name 3"), achievement1.GetName());
        Assert::AreEqual(std::wstring(L"New Name 2"), achievement2.GetName());
    }};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
