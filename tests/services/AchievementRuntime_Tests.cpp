#include "services\AchievementRuntime.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

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

    ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
    ra::data::context::mocks::MockGameContext mockGameContext;
    ra::data::context::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockFileSystem mockFileSystem;
    ra::services::mocks::MockThreadPool mockThreadPool;
    ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

    rc_memref_t* GetMemRefs() noexcept
    {
        return m_pRuntime.memrefs;
    }

    size_t GetTriggerCount() const noexcept
    {
        return m_pRuntime.trigger_count;
    }

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
        Expects(to_unsigned(nSize) < nBufferSize);

        return rc_parse_trigger(sBuffer, sTrigger, nullptr, 0);
    }

    rc_lboard_t* ParseLeaderboard(const char* sTrigger, unsigned char* sBuffer = nullptr, size_t nBufferSize = 0U)
    {
        Expects(sTrigger != nullptr);
        if (sBuffer == nullptr)
        {
            sBuffer = sTriggerBuffer.data();
            nBufferSize = sizeof(sTriggerBuffer);
        }

        const auto nSize = rc_lboard_size(sTrigger);
        Expects(to_unsigned(nSize) < nBufferSize);

        return rc_parse_lboard(sBuffer, sTrigger, nullptr, 0);
    }

    rc_richpresence_t* ParseRichPresence(const char* sScript, unsigned char* sBuffer = nullptr, size_t nBufferSize = 0U)
    {
        Expects(sScript != nullptr);
        if (sBuffer == nullptr)
        {
            sBuffer = sTriggerBuffer.data();
            nBufferSize = sizeof(sTriggerBuffer);
        }

        const auto nSize = rc_richpresence_size(sScript);
        Expects(to_unsigned(nSize) < nBufferSize);

        return rc_parse_richpresence(sBuffer, sScript, nullptr, 0);
    }

public:
    TEST_METHOD(TestActivateAchievement)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pTrigger = "0xH0000=1";

        // achievement not active, should not raise events
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // first Process call will switch the state from Waiting to Active - we ignore that
        runtime.ActivateAchievement(6U, pTrigger);
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // now that it's active, we can trigger it
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);

        // triggered achievement should not continue to trigger
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());
    }

    TEST_METHOD(TestActivateAchievementPaused)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pTrigger = "0xH0000=0";

        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());
        runtime.ActivateAchievement(6U, pTrigger);

        // runtime is paused and achievement will be ignored as long as its true
        std::vector<AchievementRuntime::Change> vChanges;
        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual({ 0U }, vChanges.size());
        }

        runtime.SetPaused(false);
        Assert::IsFalse(runtime.IsPaused());

        // achievement will initially be "waiting" and must be false before it can activate
        runtime.Process(vChanges);
        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual({ 0U }, vChanges.size());
        }

        // achievement is no longer true, so it can activate
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // achievement is true again, but should not trigger when paused
        memory.at(0) = 0;
        runtime.SetPaused(true);
        Assert::IsTrue(runtime.IsPaused());
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // processing re-enabled, achievement should trigger
        runtime.SetPaused(false);
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
    }

    TEST_METHOD(TestMonitorAchievementReset)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pTrigger = "1=1.3._R:0xH0000=1";
        runtime.ActivateAchievement(4U, pTrigger);
        runtime.GetAchievementTrigger(4U)->state = RC_TRIGGER_STATE_ACTIVE;

        // HitCount should increase, but no trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // trigger reset, expect notification
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges.front().nType);
        vChanges.clear();

        // watch for notification, already reset, no notification
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // disable reset, hitcount should increase, no notification
        memory.at(0) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // enable reset, expect notification
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges.front().nType);
        vChanges.clear();

        // already reset, shouldn't get repeated notification
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // disable reset, advance to trigger
        memory.at(0) = 0;
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(4U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
        vChanges.clear();
    }

    TEST_METHOD(TestUpdateAchievementId)
    {
        std::array<unsigned char, 1> memory{ 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* sTrigger = "0xH0000=1";

        // achievement not active, should not raise events
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // make sure the trigger is registered correctly
        runtime.ActivateAchievement(110000006U, sTrigger);
        auto* pTrigger = runtime.GetAchievementTrigger(110000006U);
        Assert::IsNotNull(pTrigger);

        // update the ID (this simulates uploading a local achievement)
        runtime.UpdateAchievementId(110000006U, 99001U);
        auto const* pTrigger2 = runtime.GetAchievementTrigger(99001U);
        Assert::IsTrue(pTrigger == pTrigger2);

        pTrigger = runtime.GetAchievementTrigger(110000006U);
        Assert::IsNull(pTrigger);

        // first Process call will switch the state from Waiting to Active - we ignore that
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // now that it's active, we can trigger it
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(99001U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
    }

    TEST_METHOD(TestResetActiveAchievements)
    {
        std::array<unsigned char, 2> memory{ 0x00, 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        runtime.ActivateAchievement(6U, "0xH0000=0_0xH0001=1");
        auto* pTrigger = runtime.GetAchievementTrigger(6U);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;

        runtime.ActivateAchievement(7U, "0xH0000=0.30._R:0xH0001=1");
        auto* pTrigger2 = runtime.GetAchievementTrigger(7U);
        pTrigger2->state = RC_TRIGGER_STATE_ACTIVE;

        // rack up some hits
        std::vector<AchievementRuntime::Change> vChanges;
        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual({ 0U }, vChanges.size());
        }
        Assert::AreEqual(10U, pTrigger->requirement->conditions->current_hits);
        Assert::AreEqual(10U, pTrigger2->requirement->conditions->current_hits);

        // set state to trigger achievement, then call reset. HitCount should go to 0 and trigger should be delayed
        // until inactive.
        memory.at(1) = 1;
        runtime.ResetActiveAchievements();
        Assert::AreEqual(0U, pTrigger->requirement->conditions->current_hits);
        Assert::AreEqual(0U, pTrigger2->requirement->conditions->current_hits);

        for (int i = 0; i < 10; i++)
        {
            runtime.Process(vChanges);
            Assert::AreEqual({ 0U }, vChanges.size());
        }

        // achievement is no longer true, so it will transition from waiting to active
        memory.at(1) = 0;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // second achievement should get a hitcount since the reset condition is no longer true
        Assert::AreEqual(1U, pTrigger2->requirement->conditions->current_hits);

        // achievement is true again and should trigger
        // second achievement is reset - expect to be notified
        memory.at(1) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 2U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
        Assert::AreEqual(7U, vChanges.back().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementReset, vChanges.back().nType);
        Assert::AreEqual(0U, pTrigger2->requirement->conditions->current_hits);
    }

    TEST_METHOD(TestPersistProgressFile)
    {
        AchievementRuntimeHarness runtime;
        auto& ach1 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach1.SetID(3U);
        ach1.SetTrigger("1=1.10.");
        auto& ach2 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach2.SetID(5U);
        ach2.SetTrigger("1=1.2.");

        const gsl::not_null<Achievement*> pAchievement3{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(3U))};
        const gsl::not_null<Achievement*> pAchievement5{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(5U))};

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);

        runtime.SaveProgressToFile("test.sav");

        // achievements weren't active, so they weren't persisted
        pAchievement3->SetConditionHitCount(0, 0, 1);
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(0U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, pAchievement5->GetConditionHitCount(0, 0));

        // activate one achievement, save, activate other, restore - only first should be restored,
        // second should be reset because it wasn't captured.
        pAchievement3->SetActive(true);
        runtime.GetAchievementTrigger(3U)->state = RC_TRIGGER_STATE_ACTIVE;
        pAchievement3->SetConditionHitCount(0, 0, 1);
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.SaveProgressToFile("test.sav");
        pAchievement5->SetActive(true);
        runtime.GetAchievementTrigger(5U)->state = RC_TRIGGER_STATE_ACTIVE;

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, pAchievement5->GetConditionHitCount(0, 0));

        // both active, save and restore should reset both
        runtime.GetAchievementTrigger(5U)->state = RC_TRIGGER_STATE_ACTIVE;
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.SaveProgressToFile("test.sav");

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, pAchievement5->GetConditionHitCount(0, 0));

        // second no longer active, file contains data that should be ignored
        pAchievement3->SetConditionHitCount(0, 0, 6);
        pAchievement5->SetConditionHitCount(0, 0, 6);
        pAchievement5->SetActive(false);
        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(6U, pAchievement5->GetConditionHitCount(0, 0));

        // if the definition changes, the hits should be reset instead of remembered
        pAchievement3->GenerateConditions();
        pAchievement3->GetCondition(0, 0).CompSource().SetValue(2);
        pAchievement3->RebuildTrigger();
        pAchievement3->SetConditionHitCount(0, 0, 2);
        Assert::IsTrue(pAchievement3->Active());
        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(0U, pAchievement3->GetConditionHitCount(0, 0));
    }

    TEST_METHOD(TestPersistProgressBuffer)
    {
        AchievementRuntimeHarness runtime;
        auto& ach1 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach1.SetID(3U);
        ach1.SetTrigger("1=1.10.");
        auto& ach2 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach2.SetID(5U);
        ach2.SetTrigger("1=1.2.");

        const gsl::not_null<Achievement*> pAchievement3{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(3U))};
        const gsl::not_null<Achievement*> pAchievement5{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(5U))};

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);

        std::string sBuffer;
        int nSize = runtime.SaveProgressToBuffer(nullptr, 0);
        sBuffer.resize(nSize);
        runtime.SaveProgressToBuffer(sBuffer.data(), nSize);

        // achievements weren't active, so they weren't persisted
        pAchievement3->SetConditionHitCount(0, 0, 1);
        pAchievement5->SetConditionHitCount(0, 0, 1);
        runtime.LoadProgressFromBuffer(sBuffer.data());
        Assert::AreEqual(0U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, pAchievement5->GetConditionHitCount(0, 0));

        // activate one achievement, save, activate other, restore - only first should be restored,
        // second should be reset because it wasn't captured.
        pAchievement3->SetActive(true);
        runtime.GetAchievementTrigger(3U)->state = RC_TRIGGER_STATE_ACTIVE;
        pAchievement3->SetConditionHitCount(0, 0, 1);
        pAchievement5->SetConditionHitCount(0, 0, 1);
        nSize = runtime.SaveProgressToBuffer(sBuffer.data(), nSize);
        if (nSize > gsl::narrow_cast<int>(sBuffer.size()))
        {
            sBuffer.resize(nSize);
            runtime.SaveProgressToBuffer(sBuffer.data(), nSize);
        }
        pAchievement5->SetActive(true);
        runtime.GetAchievementTrigger(5U)->state = RC_TRIGGER_STATE_ACTIVE;

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgressFromBuffer(sBuffer.data());
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, pAchievement5->GetConditionHitCount(0, 0));

        // both active, save and restore should reset both
        runtime.GetAchievementTrigger(5U)->state = RC_TRIGGER_STATE_ACTIVE;
        pAchievement5->SetConditionHitCount(0, 0, 1);
        nSize = runtime.SaveProgressToBuffer(sBuffer.data(), nSize);
        if (nSize > gsl::narrow_cast<int>(sBuffer.size()))
        {
            sBuffer.resize(nSize);
            runtime.SaveProgressToBuffer(sBuffer.data(), nSize);
        }

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        runtime.LoadProgressFromBuffer(sBuffer.data());
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, pAchievement5->GetConditionHitCount(0, 0));

        // second no longer active, file contains data that should be ignored
        pAchievement3->SetConditionHitCount(0, 0, 6);
        pAchievement5->SetConditionHitCount(0, 0, 6);
        pAchievement5->SetActive(false);
        runtime.LoadProgressFromBuffer(sBuffer.data());
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(6U, pAchievement5->GetConditionHitCount(0, 0));

        // if the definition changes, the hits should be reset instead of remembered
        pAchievement3->GenerateConditions();
        pAchievement3->GetCondition(0, 0).CompSource().SetValue(2);
        pAchievement3->RebuildTrigger();
        pAchievement3->SetConditionHitCount(0, 0, 2);
        Assert::IsTrue(pAchievement3->Active());
        runtime.LoadProgressFromBuffer(sBuffer.data());
        Assert::AreEqual(0U, pAchievement3->GetConditionHitCount(0, 0));
    }

    TEST_METHOD(TestPersistProgressMemory)
    {
        AchievementRuntimeHarness runtime;
        auto& ach = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach.SetID(9U);
        ach.SetTrigger("0xH1234=1_0xX1234>d0xX1234");
        ach.SetActive(true);
        runtime.GetAchievementTrigger(9U)->state = RC_TRIGGER_STATE_ACTIVE;

        ach.SetConditionHitCount(0, 0, 2);
        ach.SetConditionHitCount(0, 1, 0);
        auto pMemRef = runtime.GetMemRefs(); // 0xH1234
        pMemRef->value.value = 0x02;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0x03;
        pMemRef = pMemRef->next; // 0xX1234
        pMemRef->value.value = 0x020000;
        pMemRef->value.changed = 1;
        pMemRef->value.prior = 0x040000;

        runtime.SaveProgressToFile("test.sav");

        // modify data so we can see if the persisted data is restored
        pMemRef = runtime.GetMemRefs();
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        pMemRef = pMemRef->next;
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        ach.SetConditionHitCount(0, 0, 1);
        ach.SetConditionHitCount(0, 1, 1);

        // restore persisted data
        runtime.LoadProgressFromFile("test.sav");

        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));

        pMemRef = runtime.GetMemRefs();
        Assert::AreEqual(0x02U, pMemRef->value.value);
        Assert::AreEqual(0, (int)pMemRef->value.changed);
        Assert::AreEqual(0x03U, pMemRef->value.prior);
        pMemRef = pMemRef->next;
        Assert::AreEqual(0x020000U, pMemRef->value.value);
        Assert::AreEqual(1, (int)pMemRef->value.changed);
        Assert::AreEqual(0x040000U, pMemRef->value.prior);
    }

    TEST_METHOD(TestLoadProgressV1)
    {
        AchievementRuntimeHarness runtime;
        auto& ach1 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach1.SetID(3U);
        ach1.SetTrigger("1=1.10.");
        auto& ach2 = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach2.SetID(5U);
        ach2.SetTrigger("1=1.2.");

        const gsl::not_null<Achievement*> pAchievement3{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(3U)) };
        const gsl::not_null<Achievement*> pAchievement5{
            gsl::make_not_null(runtime.mockGameContext.FindAchievement(5U)) };

        pAchievement3->SetConditionHitCount(0, 0, 2);
        pAchievement5->SetConditionHitCount(0, 0, 2);
        pAchievement3->SetActive(true);
        pAchievement5->SetActive(true);

        runtime.mockFileSystem.MockFile(L"test.sav.rap", "3:1:1:0:0:0:0:6e2301982f40d1a3f311cdb063f57e2f:4f52856e145d7cb05822e8a9675b086b:5:1:1:0:0:0:0:0e9aec1797ad6ba861a4b1e0c7f6d2ab:dd9e5fc6020e728b8c9231d5a5c904d5:");

        runtime.LoadProgressFromFile("test.sav");
        Assert::AreEqual(1U, pAchievement3->GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, pAchievement5->GetConditionHitCount(0, 0));
    }

    TEST_METHOD(TestPersistProgressNoCoreGroup)
    {
        AchievementRuntimeHarness runtime;
        auto& ach = runtime.mockGameContext.NewAchievement(Achievement::Category::Core);
        ach.SetID(9U);
        ach.SetTrigger("S0xH1234=1_0xX1234>d0xX1234");
        ach.SetActive(true);
        runtime.GetAchievementTrigger(9U)->state = RC_TRIGGER_STATE_ACTIVE;

        ach.SetConditionHitCount(1, 0, 2);
        ach.SetConditionHitCount(1, 1, 0);
        auto pMemRef = runtime.GetMemRefs(); // 0xH1234
        pMemRef->value.value = 0x02;
        pMemRef->value.changed = 1;
        pMemRef->value.prior = 0x03;
        pMemRef = pMemRef->next; // 0xX1234
        pMemRef->value.value = 0x020000;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0x040000;

        runtime.SaveProgressToFile("test.sav");

        // modify data so we can see if the persisted data is restored
        pMemRef = runtime.GetMemRefs();
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        pMemRef = pMemRef->next;
        pMemRef->value.value = 0;
        pMemRef->value.changed = 0;
        pMemRef->value.prior = 0;
        ach.SetConditionHitCount(1, 0, 1);
        ach.SetConditionHitCount(1, 1, 1);

        // restore persisted data
        runtime.LoadProgressFromFile("test.sav");

        Assert::AreEqual(2U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 1));

        pMemRef = runtime.GetMemRefs();
        Assert::AreEqual(0x02U, pMemRef->value.value);
        Assert::AreEqual(1, (int)pMemRef->value.changed);
        Assert::AreEqual(0x03U, pMemRef->value.prior);
        pMemRef = pMemRef->next;
        Assert::AreEqual(0x020000U, pMemRef->value.value);
        Assert::AreEqual(0, (int)pMemRef->value.changed);
        Assert::AreEqual(0x040000U, pMemRef->value.prior);
    }

    TEST_METHOD(TestActivateClonedAchievement)
    {
        AchievementRuntime runtime;
        ra::services::ServiceLocator::ServiceOverride<AchievementRuntime> svcOverride(&runtime, false);
        ra::data::models::AchievementModel vmAchievement1, vmAchievement2;

        Achievement pAchievement1, pAchievement2;
        pAchievement1.m_pAchievementModel = &vmAchievement1;
        pAchievement2.m_pAchievementModel = &vmAchievement2;

        pAchievement1.SetID(1U);
        pAchievement1.SetTrigger("1=1");
        pAchievement1.SetTitle("Title");
        pAchievement1.SetDescription("Desc");
        pAchievement1.SetAuthor("Auth");
        pAchievement1.SetPoints(5);
        pAchievement1.SetCreatedDate(1234567890);
        pAchievement1.SetModifiedDate(1234567890);
        pAchievement1.SetBadgeImage("12345");
        pAchievement1.SetActive(true);
        runtime.GetAchievementTrigger(1U)->state = RC_TRIGGER_STATE_ACTIVE;

        pAchievement2.CopyFrom(pAchievement1);
        pAchievement2.SetID(12U);
        Assert::IsFalse(pAchievement2.Active()); // clone should not be immediately active
        pAchievement2.SetActive(true);
        runtime.GetAchievementTrigger(12U)->state = RC_TRIGGER_STATE_ACTIVE;

        // both achievements should trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 2U }, vChanges.size());
        Assert::AreEqual(pAchievement1.ID(), vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.front().nType);
        Assert::AreEqual(pAchievement2.ID(), vChanges.at(1).nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::AchievementTriggered, vChanges.at(1).nType);
    }

    TEST_METHOD(TestReactivateAchievement)
    {
        std::array<unsigned char, 2> memory{ 0x00, 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);

        auto* sTrigger = "0xH0001=0_0xH0000=1";
        runtime.ActivateAchievement(6U, sTrigger);
        auto* pTrigger = runtime.GetAchievementTrigger(6U);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;

        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        runtime.Process(vChanges);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        // deactivating achievement should not reset hit count
        // so user can examine hit counts after achievement has triggered
        runtime.DeactivateAchievement(6U);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        runtime.Process(vChanges);
        Assert::AreEqual(3U, pTrigger->requirement->conditions->current_hits);

        // reactivating achievement will reset hit count and allow it to increment again
        runtime.ActivateAchievement(6U, sTrigger);
        Assert::AreEqual(0U, pTrigger->requirement->conditions->current_hits);

        runtime.Process(vChanges);
        Assert::AreEqual(1U, pTrigger->requirement->conditions->current_hits);
    }

    TEST_METHOD(TestReleaseAchievementTrigger)
    {
        std::array<unsigned char, 2> memory{ 0x00, 0x00 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);

        const auto* sTrigger = "0xH0001=0_0xH0000=1";
        runtime.ActivateAchievement(6U, sTrigger);
        auto* pTrigger = runtime.GetAchievementTrigger(6U);

        // deactivated achievement shouldn't be found without passing in the trigger
        runtime.DeactivateAchievement(6U);
        auto* pTrigger2 = runtime.GetAchievementTrigger(6U);
        Assert::IsNull(pTrigger2);
        pTrigger2 = runtime.GetAchievementTrigger(6U, sTrigger);
        Assert::IsNotNull(pTrigger2);
        Assert::IsTrue(pTrigger == pTrigger2);

        // cannot release trigger that has memrefs
        runtime.ReleaseAchievementTrigger(6U, pTrigger);
        pTrigger2 = runtime.GetAchievementTrigger(6U);
        Assert::IsNull(pTrigger2);
        pTrigger2 = runtime.GetAchievementTrigger(6U, sTrigger);
        Assert::IsNotNull(pTrigger2);
        Assert::IsTrue(pTrigger == pTrigger2);

        // register second trigger that will not have memrefs
        runtime.ActivateAchievement(7U, sTrigger);
        pTrigger = runtime.GetAchievementTrigger(7U);
        runtime.DeactivateAchievement(7U);
        pTrigger2 = runtime.GetAchievementTrigger(7U);
        Assert::IsNull(pTrigger2);
        pTrigger2 = runtime.GetAchievementTrigger(7U, sTrigger);
        Assert::IsNotNull(pTrigger2);
        Assert::IsTrue(pTrigger == pTrigger2);

        // can release trigger that does not have memrefs
        Assert::AreEqual({ 2U }, runtime.GetTriggerCount());
        runtime.ReleaseAchievementTrigger(7U, pTrigger);
        Assert::AreEqual({ 1U }, runtime.GetTriggerCount());
        pTrigger2 = runtime.GetAchievementTrigger(7U);
        Assert::IsNull(pTrigger2);
        pTrigger2 = runtime.GetAchievementTrigger(7U, sTrigger);
        Assert::IsNull(pTrigger2);

        // first trigger should still be findable
        pTrigger2 = runtime.GetAchievementTrigger(6U, sTrigger);
        Assert::IsNotNull(pTrigger2);
    }

    TEST_METHOD(TestActivateLeaderboard)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);
        auto* pLeaderboard = "STA:0xH00=1::CAN:0xH00=2::SUB:0xH00=3::VAL:0xH02";

        // leaderboard not active, should not trigger
        std::vector<AchievementRuntime::Change> vChanges;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // leaderboard active, but not started
        runtime.ActivateLeaderboard(6U, pLeaderboard);
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // leaderboard cancel condition should not be captured until it's been started
        memory.at(0) = 2;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // leaderboard start condition should notify the start with initial value
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardStarted, vChanges.front().nType);
        Assert::AreEqual(52, vChanges.front().nValue);

        // still active leaderboard should not notify unless the value changes
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        memory.at(2) = 33;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardUpdated, vChanges.front().nType);
        Assert::AreEqual(33, vChanges.front().nValue);
        vChanges.clear();

        // leaderboard cancel condition should notify
        memory.at(0) = 2;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardCanceled, vChanges.front().nType);
        vChanges.clear();

        // leaderboard submit condition should not trigger if leaderboard not started
        memory.at(0) = 3;
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // restart the leaderboard
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardStarted, vChanges.front().nType);
        Assert::AreEqual(33, vChanges.front().nValue);
        vChanges.clear();

        // leaderboard submit should trigger if leaderboard started
        memory.at(0) = 3;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardTriggered, vChanges.front().nType);
        Assert::AreEqual(33, vChanges.front().nValue);
        vChanges.clear();

        // leaderboard cancel condition should not trigger after submission
        memory.at(0) = 2;
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        // restart the leaderboard
        memory.at(0) = 1;
        runtime.Process(vChanges);
        Assert::AreEqual({ 1U }, vChanges.size());
        Assert::AreEqual(6U, vChanges.front().nId);
        Assert::AreEqual(AchievementRuntime::ChangeType::LeaderboardStarted, vChanges.front().nType);
        Assert::AreEqual(33, vChanges.front().nValue);
        vChanges.clear();

        // deactivated leaderboard should not trigger
        runtime.DeactivateLeaderboard(6U);
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());

        memory.at(0) = 2;
        vChanges.clear();
        runtime.Process(vChanges);
        Assert::AreEqual({ 0U }, vChanges.size());
    }

    TEST_METHOD(TestActivateRichPresence)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        std::vector<ra::services::AchievementRuntime::Change> changes;

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);

        auto* pRichPresence = "Format:Num\nFormatType:Value\n\nDisplay:\n@Num(0xH01) @Num(d0xH01)\n";
        runtime.ActivateRichPresence(pRichPresence);

        // string is evaluated with current memrefs (which will be 0)
        Assert::AreEqual(std::wstring(L"0 0"), runtime.GetRichPresenceDisplayString());

        // do_frame should immediately process the rich presence
        runtime.Process(changes);
        Assert::AreEqual(std::wstring(L"18 0"), runtime.GetRichPresenceDisplayString());

        // first update - updates value and delta
        runtime.Process(changes);
        Assert::AreEqual(std::wstring(L"18 18"), runtime.GetRichPresenceDisplayString());

        // second update - updates delta
        runtime.Process(changes);
        Assert::AreEqual(std::wstring(L"18 18"), runtime.GetRichPresenceDisplayString());

        // third update - updates value after change
        memory.at(1) = 11;
        runtime.Process(changes);
        Assert::AreEqual(std::wstring(L"11 18"), runtime.GetRichPresenceDisplayString());

        // fourth update - updates delta and value after change
        memory.at(1) = 13;
        runtime.Process(changes);
        Assert::AreEqual(std::wstring(L"13 11"), runtime.GetRichPresenceDisplayString());
    }

    TEST_METHOD(TestActivateRichPresenceChange)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        std::vector<ra::services::AchievementRuntime::Change> changes;

        AchievementRuntimeHarness runtime;
        runtime.mockEmulatorContext.MockMemory(memory);

        runtime.ActivateRichPresence("Display:\nHello, World\n");
        Assert::AreEqual(std::wstring(L"Hello, World"), runtime.GetRichPresenceDisplayString());

        runtime.ActivateRichPresence("Display:\nNew String\n");
        Assert::AreEqual(std::wstring(L"New String"), runtime.GetRichPresenceDisplayString());

        runtime.ActivateRichPresence("");
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), runtime.GetRichPresenceDisplayString());
    }
};

} // namespace tests
} // namespace services
} // namespace ra
