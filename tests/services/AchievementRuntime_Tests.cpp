#include "services\AchievementRuntime.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::services::AchievementRuntime::ChangeType>(
    const ra::services::AchievementRuntime::ChangeType& nChangeType)
{
    switch (nChangeType)
    {
        case ra::services::AchievementRuntime::ChangeType::None:
            return L"None";
        case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            return L"AchievementTriggered";
        case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            return L"AchievementReset";
        default:
            return std::to_wstring(static_cast<int>(nChangeType));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace services {
namespace tests {

class AchievementRuntimeHarness : public AchievementRuntime
{
public:
    GSL_SUPPRESS_F6 AchievementRuntimeHarness() : m_Override(this)
    {
        mockUserContext.Initialize("User", "ApiToken");
    }

    ra::data::mocks::MockGameContext mockGameContext;
    ra::data::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockFileSystem mockFileSystem;

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
};

TEST_CLASS(AchievementRuntime_Tests)
{
private:
    std::array<unsigned char, 1024> sTriggerBuffer{};

    rc_trigger_t* ParseTrigger(const char* sTrigger, unsigned char* sBuffer = nullptr, size_t nBufferSize = 0U)
    {
        Expects(sTrigger != nullptr);
        if (sBuffer == nullptr)
        {
            sBuffer = sTriggerBuffer.data();
            nBufferSize = sizeof(sTriggerBuffer);
        }

        const auto nSize = rc_trigger_size(sTrigger);
        Ensures(to_unsigned(nSize) < nBufferSize);

        return rc_parse_trigger(sBuffer, sTrigger, nullptr, 0);
    }

public:
    TEST_METHOD(TestActivateAchievement)
    {
        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("1=1");

        // achievement not active, should not trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // active achievement should trigger
        runtime.ActivateAchievement(6U, pTrigger);
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);

        // still active achievement should continue to trigger
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);

        // deactivated achievement should not trigger
        runtime.DeactivateAchievement(6U);
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());
    }

    TEST_METHOD(TestActivateAchievementPaused)
    {
        std::array<unsigned char, 1> memory{ 0x00 };
        InitializeMemory(memory);

        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("0xH0000=0");

        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());
        runtime.ActivateAchievement(6U, pTrigger);
        runtime.SetPaused(false);
        Assert::IsFalse(runtime.IsPaused());

        // achievement is paused and will be ignored as long as its true
        std::vector<AchievementRuntime::Change> vChanges;
        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual(0U, vChanges.size());
        }

        // achievement is no longer true, so it won't trigger
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // achievement is true again and should trigger
        memory.at(0) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);

        // processing is paused, achievement should not trigger
        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // processing re-enabled, achievement should trigger
        runtime.SetPaused(false);
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
    }

    TEST_METHOD(TestMonitorAchievementReset)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("1=1.3._R:0xH0000=1");
        runtime.ActivateAchievement(4U, pTrigger);

        // HitCount should increase, but no trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // trigger reset, but not watching for reset, no notification
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // watch for notification, already reset, no notification
        runtime.MonitorAchievementReset(4U, pTrigger);
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset, hitcount should increase, no notification
        memory.at(0) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // enable reset, expect notification
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges.front().nType);
        vChanges.clear();

        // already reset, shouldn't get repeated notification
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset, advance to trigger
        memory.at(0) = 0;
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
        vChanges.clear();

        // enable reset, reset prevents trigger, expect reset notification
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges.front().nType);
        vChanges.clear();

        // disable reset, no change expected
        memory.at(0) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset monitor and reset, expect no notification
        runtime.ActivateAchievement(4U, pTrigger);
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());
    }

    TEST_METHOD(TestResetActiveAchievements)
    {
        std::array<unsigned char, 2> memory{ 0x00, 0x00 };
        InitializeMemory(memory);

        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("0xH0000=0_0xH0001=1");

        runtime.ActivateAchievement(6U, pTrigger);

        // rack up some hits
        std::vector<AchievementRuntime::Change> vChanges;
        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual(0U, vChanges.size());
        }
        Assert::AreEqual(10U, pTrigger->requirement->conditions->current_hits);

        // set state to trigger achievement, then call reset. HitCount should go to 0 and trigger should be delayed
        // until inactive.
        memory.at(1) = 1;
        runtime.ResetActiveAchievements();
        Assert::AreEqual(0U, pTrigger->requirement->conditions->current_hits);

        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // achievement is no longer true, so it won't trigger
        memory.at(1) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // achievement is true again and should trigger
        memory.at(1) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
    }

    TEST_METHOD(TestPersistProgress)
    {
        AchievementRuntimeHarness runtime;
        auto& ach1 = runtime.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        ach1.SetID(3U);
        ach1.ParseTrigger("1=1.10.");
        auto& ach2 = runtime.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        ach2.SetID(5U);
        ach2.ParseTrigger("1=1.2.");

        const gsl::not_null<Achievement*> pAchievement3{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(3U))};
        const gsl::not_null<Achievement*> pAchievement5{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(5U))};

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);

        runtime.SaveProgress("test.sav");

        // achievements weren't active, so they weren't persisted
        pAchievement3->SetConditionHitCount(0, 0, 1);
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.LoadProgress("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, pAchievement5->GetConditionHitCount(0, 0));

        // activate one achievement, save, activate other, restore - only first should be restored,
        // second should be reset because it wasn't captured.
        pAchievement3->SetActive(true);
        runtime.SaveProgress("test.sav");
        pAchievement5->SetActive(true);
        pAchievement5->SetPauseOnReset(true); // make sure these get persisted too

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgress("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, pAchievement5->GetConditionHitCount(0, 0));

        // both active, save and restore should reset both
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.SaveProgress("test.sav");

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgress("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, pAchievement5->GetConditionHitCount(0, 0));

        // second no longer active, file contains data that should be ignored
        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetActive(false);
        runtime.LoadProgress("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, pAchievement5->GetConditionHitCount(0, 0));

        // if the definition changes, the hits should be reset instead of remembered
        pAchievement3->GetCondition(0, 0).CompSource().SetValue(2);
        pAchievement3->RebuildTrigger();
        pAchievement3->SetConditionHitCount(0, 0, 2);
        Assert::IsTrue(pAchievement3->Active());
        runtime.LoadProgress("test.sav");
        Assert::AreEqual(0U, pAchievement3->GetConditionHitCount(0, 0));
    }

    TEST_METHOD(TestActivateClonedAchievement)
    {
        AchievementRuntime runtime;
        ra::services::ServiceLocator::ServiceOverride<AchievementRuntime> svcOverride(&runtime, false);

        Achievement pAchievement1, pAchievement2;
        pAchievement1.SetID(1U);
        pAchievement1.ParseTrigger("1=1");
        pAchievement1.SetTitle("Title");
        pAchievement1.SetDescription("Desc");
        pAchievement1.SetAuthor("Auth");
        pAchievement1.SetPoints(5);
        pAchievement1.SetCreatedDate(1234567890);
        pAchievement1.SetModifiedDate(1234567890);
        pAchievement1.SetBadgeImage("12345");

        pAchievement1.SetActive(true);
        pAchievement2.CopyFrom(pAchievement1);
        pAchievement2.SetID(12);
        Assert::IsFalse(pAchievement2.Active()); // clone should not be immediately active
        pAchievement2.SetActive(true);

        // both achievements should trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual(2U, vChanges.size());
        Assert::AreEqual(pAchievement1.ID(), vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
        Assert::AreEqual(pAchievement2.ID(), vChanges.at(1).nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.at(1).nType);
    }

    TEST_METHOD(TestReactivateAchievement)
    {
        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("1=1_1=0");

        std::vector<AchievementRuntime::Change> vChanges;
        runtime.ActivateAchievement(6U, pTrigger);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        // deactivating achievement should not reset hit count
        runtime.DeactivateAchievement(6U);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        runtime.Process(vChanges);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        // reactivating achievement should not reset hit count
        // this behavior is important for GameContext.RefreshUnlocks - when switching from hardcore to non-hardcore,
        // the achievement is deactivated and reactivated (if not unlocked). since the emulator is not reset, the
        // hit count shouldn't be reset either.
        runtime.ActivateAchievement(6U, pTrigger);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        runtime.Process(vChanges);
        Assert::AreEqual(4U, pTrigger->requirement->conditions->current_hits);
    }
};

} // namespace tests
} // namespace services
} // namespace ra
