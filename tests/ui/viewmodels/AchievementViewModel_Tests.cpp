#include "CppUnitTest.h"

#include "ui\viewmodels\AchievementViewModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AchievementViewModel_Tests)
{
private:
    class AchievementViewModelHarness : public AchievementViewModel
    {
    public:
        ra::services::impl::StringTextWriter textWriter;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockClock mockClock;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AchievementViewModelHarness achievement;

        Assert::AreEqual((int)AssetType::Achievement, (int)achievement.GetType());
        Assert::AreEqual(0U, achievement.GetID());
        Assert::AreEqual(std::wstring(L""), achievement.GetName());
        Assert::AreEqual(std::wstring(L""), achievement.GetDescription());
        Assert::AreEqual(AssetCategory::Core, achievement.GetCategory());
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        Assert::AreEqual(AssetChanges::None, achievement.GetChanges());
        Assert::AreEqual(5, achievement.GetPoints());
        Assert::AreEqual(std::wstring(L"00000"), achievement.GetBadge());
        Assert::IsFalse(achievement.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnTrigger());
        Assert::AreEqual(std::string(""), achievement.GetTrigger());
    }

    TEST_METHOD(TestActivateDeactivate)
    {
        AchievementViewModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");

        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        Assert::IsNull(achievement.mockRuntime.GetAchievementTrigger(1U));

        achievement.Activate();

        Assert::AreEqual(AssetState::Waiting, achievement.GetState());
        const auto* pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Assert::AreEqual((int)RC_TRIGGER_STATE_WAITING, (int)pTrigger->state);

        achievement.Deactivate();

        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNull(pTrigger);
    }

    TEST_METHOD(TestActivateDeactivateActive)
    {
        AchievementViewModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.Activate();

        auto* pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
        achievement.DoFrame();
        Assert::AreEqual(AssetState::Active, achievement.GetState());

        achievement.Activate();
        Assert::AreEqual(AssetState::Active, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Assert::AreEqual((int)RC_TRIGGER_STATE_ACTIVE, (int)pTrigger->state);

        achievement.Deactivate();
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNull(pTrigger);
    }

    TEST_METHOD(TestActivateDeactivatePaused)
    {
        AchievementViewModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.Activate();

        auto* pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_PAUSED;
        achievement.DoFrame();
        Assert::AreEqual(AssetState::Paused, achievement.GetState());

        achievement.Activate();
        Assert::AreEqual(AssetState::Paused, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Assert::AreEqual((int)RC_TRIGGER_STATE_PAUSED, (int)pTrigger->state);

        achievement.Deactivate();
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNull(pTrigger);
        Assert::AreEqual(std::chrono::system_clock::duration::zero().count(), achievement.GetUnlockTime().time_since_epoch().count());
    }

    TEST_METHOD(TestActivateDeactivateTriggered)
    {
        AchievementViewModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.Activate();

        auto* pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_TRIGGERED;

        // first DoFrame will set State to Triggered, which deactivates the achievement
        achievement.DoFrame();
        Assert::AreEqual(AssetState::Triggered, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNull(pTrigger);
        Assert::AreNotEqual(std::chrono::system_clock::duration::zero().count(), achievement.GetUnlockTime().time_since_epoch().count());

        // second DoFrame won't be able to find the trigger, but should not modify the State
        achievement.DoFrame();
        Assert::AreEqual(AssetState::Triggered, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNull(pTrigger);

        // reactivating the achievement will set it back to waiting, but not reset the unlock time
        achievement.Activate();
        Assert::AreEqual(AssetState::Waiting, achievement.GetState());
        pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Assert::AreEqual((int)RC_TRIGGER_STATE_WAITING, (int)pTrigger->state);
        Assert::AreNotEqual(std::chrono::system_clock::duration::zero().count(), achievement.GetUnlockTime().time_since_epoch().count());
    }
};


} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
