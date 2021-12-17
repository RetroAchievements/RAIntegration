#include "CppUnitTest.h"

#include "data\models\AchievementModel.hh"
#include "data\models\LocalBadgesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(AchievementModel_Tests)
{
private:
    class AchievementModelHarness : public AchievementModel
    {
    public:
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::impl::StringTextWriter textWriter;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockClock mockClock;

        void AddLocalBadgesModel()
        {
            auto pLocalBadges = std::make_unique<LocalBadgesModel>();
            pLocalBadges->CreateServerCheckpoint();
            pLocalBadges->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(pLocalBadges));
        }

        ra::data::models::LocalBadgesModel& GetLocalBadgesModel()
        {
            return *(dynamic_cast<LocalBadgesModel*>(mockGameContext.Assets().FindAsset(AssetType::LocalBadges, 0)));
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AchievementModelHarness achievement;

        Assert::AreEqual(AssetType::Achievement, achievement.GetType());
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
        AchievementModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

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
        AchievementModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
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
        AchievementModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
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
        AchievementModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
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

    TEST_METHOD(TestStatePrimed)
    {
        AchievementModelHarness achievement;
        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1_T:0xH1234=2");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();
        achievement.Activate();

        auto* pTrigger = achievement.mockRuntime.GetAchievementTrigger(1U);
        Assert::IsNotNull(pTrigger);
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_PRIMED;
        achievement.DoFrame();
        Assert::AreEqual(AssetState::Primed, achievement.GetState());
    }

    TEST_METHOD(TestSetBadgeNameLocal)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.AddLocalBadgesModel();
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.SetBadge(L"local\\22-ABC.png");

        const auto& pLocalBadges = achievement.GetLocalBadgesModel();
        Assert::AreEqual(1, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", false));
        Assert::AreEqual(0, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", true));

        // this sets the committed badge reference so it'll show up in the serialized string
        achievement.UpdateLocalCheckpoint();

        Assert::AreEqual(0, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", false));
        Assert::AreEqual(1, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", true));
    }

    TEST_METHOD(TestSetBadgeNameAndReset)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.AddLocalBadgesModel();
        achievement.SetBadge(L"12345.png");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        achievement.SetID(1U);
        achievement.SetTrigger("0xH1234=1");
        achievement.SetBadge(L"local\\22-ABC.png");

        const auto& pLocalBadges = achievement.GetLocalBadgesModel();
        Assert::AreEqual(1, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", false));
        Assert::AreEqual(0, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", true));

        achievement.SetBadge(L"12345.png");

        // the local badge is no longer referenced, so shouldn't be serialized
        achievement.UpdateLocalCheckpoint();

        Assert::AreEqual(0, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", false));
        Assert::AreEqual(0, pLocalBadges.GetReferenceCount(L"local\\22-ABC.png", true));
    }

    TEST_METHOD(TestValidate)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.AddLocalBadgesModel();
        achievement.SetBadge(L"12345.png");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        Assert::IsTrue(achievement.Validate());
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());

        achievement.SetID(1U);
        achievement.SetTrigger("A:0xH1234=1");

        // Validation checks the uncommited state.
        Assert::IsFalse(achievement.Validate());
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), achievement.GetValidationError());

        // Validation checks the uncommited state.
        achievement.UpdateLocalCheckpoint();
        achievement.SetTrigger("0xH1234=1");

        Assert::IsTrue(achievement.Validate());
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());

        // Restoring the local checkpoint automatically updates the validation error.
        achievement.RestoreLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), achievement.GetValidationError());

        // Fixing the error does not automatically update the validation error.
        achievement.SetTrigger("0xH1234=1");
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), achievement.GetValidationError());

        // Committing the fix does update the validation error.
        achievement.UpdateLocalCheckpoint();
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
