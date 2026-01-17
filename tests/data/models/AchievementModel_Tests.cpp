#include "CppUnitTest.h"

#include "data\models\AchievementModel.hh"
#include "data\models\LocalBadgesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockClock.hh"
#include "tests\devkit\testutil\AssetAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

static bool g_bEventSeen = false; // must be global because callback cannot accept lambda

TEST_CLASS(AchievementModel_Tests)
{
private:
    class AchievementModelHarness : public AchievementModel
    {
    public:
        AchievementModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockRuntime.MockGame();
        }

        ra::context::mocks::MockRcClient mockRcClient;
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
        Assert::AreEqual(AchievementType::None, achievement.GetAchievementType());
        Assert::AreEqual(std::wstring(L"00000"), achievement.GetBadge());
        Assert::IsFalse(achievement.IsPauseOnReset());
        Assert::IsFalse(achievement.IsPauseOnTrigger());
        Assert::AreEqual(std::string(""), achievement.GetTrigger());
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
        Assert::AreEqual(std::wstring(L"Condition 1: AddSource condition type expects another condition to follow"), achievement.GetValidationError());

        // Validation checks the uncommited state.
        achievement.UpdateLocalCheckpoint();
        achievement.SetTrigger("0xH1234=1");

        Assert::IsTrue(achievement.Validate());
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());

        // Restoring the local checkpoint automatically updates the validation error.
        achievement.RestoreLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Condition 1: AddSource condition type expects another condition to follow"), achievement.GetValidationError());

        // Fixing the error does not automatically update the validation error.
        achievement.SetTrigger("0xH1234=1");
        Assert::AreEqual(std::wstring(L"Condition 1: AddSource condition type expects another condition to follow"), achievement.GetValidationError());

        // Committing the fix does update the validation error.
        achievement.UpdateLocalCheckpoint();
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());
    }

    TEST_METHOD(TestValidatePoints)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.AddLocalBadgesModel();
        achievement.SetBadge(L"12345.png");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        Assert::IsTrue(achievement.Validate());
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());

        const std::array<int, 10> vValidPoints = {0, 1, 2, 3, 4, 5, 10, 25, 50, 100};

        for (int i = -100; i <= 500; i++)
        {
            achievement.SetPoints(i);

            const auto pIter = std::find(vValidPoints.begin(), vValidPoints.end(), i);
            if (pIter == vValidPoints.end())
            {
                Assert::IsFalse(achievement.Validate());
                Assert::AreEqual(std::wstring(L"Invalid point value"), achievement.GetValidationError());
            }
            else
            {
                Assert::IsTrue(achievement.Validate());
                Assert::AreEqual(std::wstring(), achievement.GetValidationError());
            }
        }
    }
    
    TEST_METHOD(TestValidateLength)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.AddLocalBadgesModel();
        achievement.SetBadge(L"12345.png");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        std::string sTrigger;
        for (int i = 0; i < 65535/12 - 1; i++)
            sTrigger.append(ra::StringPrintf("0xH00%04X=0_", i));
        sTrigger.append("0xX00FFF0=12345");
        Assert::AreEqual({65535U}, sTrigger.length());
        achievement.SetTrigger(sTrigger);

        Assert::IsTrue(achievement.Validate());
        Assert::AreEqual(std::wstring(), achievement.GetValidationError());

        sTrigger.append("6"); // 65536 bytes
        achievement.SetTrigger(sTrigger);

        Assert::IsFalse(achievement.Validate());
        Assert::AreEqual(std::wstring(L"Serialized length exceeds limit: 65536/65535"), achievement.GetValidationError());
    }

    TEST_METHOD(TestDeactivateHidesIndicator)
    {

        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.SetID(53U);
        achievement.SetBadge(L"12345.png");
        achievement.SetTrigger("0xH1234=1_T:0xH2345=2");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        achievement.mockRuntime.MockGame();
        auto* achievement_info = achievement.mockRuntime.MockAchievementWithTrigger(achievement.GetID());
        achievement.ReplaceAttached(*achievement_info);

        g_bEventSeen = false;
        achievement.mockRuntime.GetClient()->callbacks.event_handler =
            [](const rc_client_event_t* pEvent, rc_client_t*)
            {
                Assert::AreEqual({RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE}, pEvent->type);
                Assert::AreEqual({53}, pEvent->achievement->id);
                g_bEventSeen = true;
            };

        achievement.SetState(AssetState::Primed);
        Assert::IsTrue(achievement.IsActive());
        Assert::IsFalse(g_bEventSeen);

        achievement.SetState(AssetState::Inactive);
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        Assert::IsFalse(achievement.IsActive());
        Assert::IsTrue(g_bEventSeen);
    }

    TEST_METHOD(TestDeleteHidesIndicator)
    {
        AchievementModelHarness achievement;
        achievement.mockGameContext.SetGameId(22);
        achievement.SetID(53U);
        achievement.SetBadge(L"12345.png");
        achievement.SetTrigger("0xH1234=1_T:0xH2345=2");
        achievement.CreateServerCheckpoint();
        achievement.CreateLocalCheckpoint();

        achievement.mockRuntime.MockGame();
        auto* achievement_info = achievement.mockRuntime.MockAchievementWithTrigger(achievement.GetID());
        achievement.ReplaceAttached(*achievement_info);

        g_bEventSeen = false;
        achievement.mockRuntime.GetClient()->callbacks.event_handler = [](const rc_client_event_t* pEvent,
                                                                          rc_client_t*) {
            Assert::AreEqual({RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE}, pEvent->type);
            Assert::AreEqual({53}, pEvent->achievement->id);
            g_bEventSeen = true;
        };

        achievement.SetState(AssetState::Primed);
        Assert::IsTrue(achievement.IsActive());
        Assert::IsFalse(g_bEventSeen);

        achievement.SetDeleted();
        Assert::AreEqual(AssetChanges::Deleted, achievement.GetChanges());

        // deleting should deactivate the achievement, which should raise the indicator hide event
        Assert::AreEqual(AssetState::Inactive, achievement.GetState());
        Assert::IsFalse(achievement.IsActive());
        Assert::IsTrue(g_bEventSeen);
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
