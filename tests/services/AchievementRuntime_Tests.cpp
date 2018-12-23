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
    unsigned char sTriggerBuffer[1024];

    rc_trigger_t* ParseTrigger(const char* sTrigger, unsigned char* sBuffer = nullptr, size_t nBufferSize = 0U) noexcept
    {
        if (sBuffer == nullptr)
        {
            sBuffer = sTriggerBuffer;
            nBufferSize = sizeof(sTriggerBuffer);
        }

        const auto nSize = rc_trigger_size(sTrigger);
        assert(static_cast<size_t>(nSize) < nBufferSize);

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
        Assert::AreEqual(6U, vChanges[0].nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges[0].nType);

        // still active achievement should continue to trigger
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(6U, vChanges[0].nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges[0].nType);

        // deactivated achievement should not trigger
        runtime.DeactivateAchievement(6U);
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());
    }

    TEST_METHOD(TestMonitorAchievementReset)
    {
        unsigned char memory[] = {0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory, 5);

        AchievementRuntime runtime;
        auto* pTrigger = ParseTrigger("1=1.3._R:0xH0000=1");
        runtime.ActivateAchievement(4U, pTrigger);

        // HitCount should increase, but no trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // trigger reset, but not watching for reset, no notification
        memory[0] = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // watch for notification, already reset, no notification
        runtime.MonitorAchievementReset(4U, pTrigger);
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset, hitcount should increase, no notification
        memory[0] = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // enable reset, expect notification
        memory[0] = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges[0].nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges[0].nType);
        vChanges.clear();

        // already reset, shouldn't get repeated notification
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset, advance to trigger
        memory[0] = 0;
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges[0].nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges[0].nType);
        vChanges.clear();

        // enable reset, reset prevents trigger, expect reset notification
        memory[0] = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(1U, vChanges.size());
        Assert::AreEqual(4U, vChanges[0].nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges[0].nType);
        vChanges.clear();

        // disable reset, no change expected
        memory[0] = 0;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());

        // disable reset monitor and reset, expect no notification
        runtime.ActivateAchievement(4U, pTrigger);
        memory[0] = 1;
        runtime.Process(vChanges);
        Assert::AreEqual(0U, vChanges.size());
    }

    TEST_METHOD(TestPersistProgress)
    {
        AchievementRuntimeHarness runtime;
        runtime.mockGameContext.AddAchivement(3U, "1=1.10.");
        runtime.mockGameContext.AddAchivement(5U, "1=1.2.");

        const gsl::not_null<Achievement* const> pAchievement3{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(3U))};
        const gsl::not_null<Achievement* const> pAchievement5{
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
};

} // namespace tests
} // namespace services
} // namespace ra
