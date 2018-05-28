#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_ConditionSet_Tests)
{
public:
    void AssertSetTest(ConditionSet& set, bool bExpectedResult, bool bExpectedDirty, bool bExpectedReset)
    {
        bool bDirtyCondition, bWasReset;
        Assert::AreEqual(bExpectedResult, set.Test(bDirtyCondition, bWasReset), L"Test");
        Assert::AreEqual(bExpectedDirty, bDirtyCondition, L"bDirtyCondition");
        Assert::AreEqual(bExpectedReset, bWasReset, L"bWasReset");
    }

    void AssertSerialize(const char* sSerialized, const char* sExpected = nullptr)
    {
        if (sExpected == nullptr)
            sExpected = sSerialized;

        const char* ptr = sSerialized;
        ConditionSet set;
        set.ParseFromString(ptr);

        std::string sReserialized;
        set.Serialize(sReserialized);

        Assert::AreEqual(sExpected, sReserialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestSerialize)
    {
        AssertSerialize("0xH0001=18");
        AssertSerialize("0xH0001=18_0xH0002=52");
        AssertSerialize("0xH0001=18_0xH0002=52_0xL0004=4");

        AssertSerialize("0xH0001=18.2._R:0xH0002=50_P:0xL0004=4");

        AssertSerialize("A:0xH0001=0_B:0xL0002=0_0xL0004=14");

        AssertSerialize("0xH0001=18.1._R:0xH0000=1S0xH0002=52.1.S0xL0004=6.1._P:0xH0000=2");
    }

    TEST_METHOD(TestSimpleSets) // Only standard conditions, no alt groups
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18"); // one condition, true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());

        set.ParseFromString(ptr = "0xH0001!=18"); // one condition, false
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=18_0xH0002=52"); // two conditions, true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=18_0xH0002>52"); // two conditions, false
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=18_0xH0002=52_0xL0004=6"); // three conditions, true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=16_0xH0002=52_0xL0004=6"); // three conditions, first false
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=18_0xH0002=50_0xL0004=6"); // three conditions, first false
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=18_0xH0002=52_0xL0004=4"); // three conditions, first false
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());

        set.ParseFromString(ptr = "0xH0001=16_0xH0002=50_0xL0004=4"); // three conditions, all false
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
    }

    TEST_METHOD(TestPauseIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_P:0xH0002=52_P:0xL0x0004=6");
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // Also true, but processing stops on first PauseIf

        memory[2] = 0;
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // PauseIf goes to 0 when false
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits()); // PauseIf stays at 1 when false

        memory[4] = 0;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // PauseIf goes to 0 when false
    }


    TEST_METHOD(TestPauseIfHitCountOne)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_P:0xH0002=52.1.");
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 0;
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits()); // PauseIf with HitCount doesn't automatically go back to 0
    }

    TEST_METHOD(TestPauseIfHitCountTwo)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_P:0xH0002=52.2.");
        AssertSetTest(set, true, true, false);                        // PauseIf counter hasn't reached HitCount target, non-PauseIf condition still true
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, false, true, false);                        // PauseIf counter has reached HitCount target, non-PauseIf conditions ignored
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 0;
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits()); // PauseIf with HitCount doesn't automatically go back to 0
    }

    TEST_METHOD(TestPauseIfHitReset)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_P:0xH0002=52.1._R:0xH0003=1SR:0xH0003=2");
        AssertSetTest(set, false, true, false);                        // Trigger PauseIf, non-PauseIf conditions ignored
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());

        memory[2] = 0;
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits()); // PauseIf with HitCount doesn't automatically go back to 0
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());

        memory[3] = 1;
        AssertSetTest(set, false, false, false);                      // ResetIf in Paused group is ignored
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());

        memory[3] = 2;
        AssertSetTest(set, false, true, true);                        // ResetIf in alternate group is honored, PauseIf does not retrigger and non-PauseIf condition is true
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // ResetIf causes entire achievement to fail
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());

        memory[3] = 3;
        AssertSetTest(set, true, true, false);                         // ResetIf no longer true, achievement allowed to trigger
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());
    }

    TEST_METHOD(TestResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_R:0xH0002=50_R:0xL0x0004=4");
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[2] = 50;
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // True, but ResetIf also resets true marker
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[4] = 0x54;
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // True, but ResetIf also resets true marker
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // Also true, but processing stop on first ResetIf

        memory[2] = 52;
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // True, but ResetIf also resets true marker

        memory[4] = 0x56;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());
    }

    TEST_METHOD(TestHitCount)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=20(2)_0xH0002=52");

        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[1] = 20;
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits()); // hits stop increment once count it reached
        Assert::AreEqual(4U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    // verifies that ResetIf resets HitCounts
    TEST_METHOD(TestHitCountResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(2)_0xH0002=52_R:0xL0004=4");

        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x54;
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x56;
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    // verifies that ResetIf with HitCount target only resets HitCounts when target is met
    TEST_METHOD(TestHitCountResetIfHitCount)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(2)_0xH0002=52_R:0xL0004=4.2.");

        AssertSetTest(set, false, true, false); // HitCounts on conditions 1 and 2 are incremented
        AssertSetTest(set, true, true, false);  // HitCounts on conditions 1 and 2 are incremented, cond 1 is now true so entire achievement is true
        AssertSetTest(set, true, true, false);  // HitCount on condition 2 is incremented, cond 1 already met its target HitCount
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // ResetIf HitCount should still be 0

        memory[4] = 0x54;

        // first hit on ResetIf should not reset anything
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits()); // condition 1 stopped at it's HitCount target
        Assert::AreEqual(4U, set.GetGroup(0).GetAt(1).CurrentHits()); // condition 2 continues to increment
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits()); // ResetIf HitCount should be 1

        // second hit on ResetIf should reset everything
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // ResetIf HitCount should also be reset
    }

    // verifies that ResetIf works with AddHits
    TEST_METHOD(TestAddHitsResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "C:0xH0001=18_R:0xL0004=6(3)"); // never(repeated(3, byte(1) == 18 || low(4) == 6))
        AssertSetTest(set, true, true, false); // result is true, no non-reset conditions
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, false, true, true); // total hits met (2 for each condition, only needed 3 total) (2 hits on condition 2 is not enough), result is always false if reset
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    // verifies that ResetIf HitCount(1) behaves like ResetIf without a HitCount
    TEST_METHOD(TestHitCountResetIfHitCountOne)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(2)_0xH0002=52_R:0xL0004=4.1.");

        AssertSetTest(set, false, true, false); // HitCounts on conditions 1 and 2 are incremented
        AssertSetTest(set, true, true, false);  // HitCounts on conditions 1 and 2 are incremented, cond 1 is now true so entire achievement is true
        AssertSetTest(set, true, true, false);  // HitCount on condition 2 is incremented, cond 1 already met its target HitCount
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // ResetIf HitCount should still be 0

        memory[4] = 0x54;

        // ResetIf HitCount(1) should behave just like ResetIf without a HitCount - all items, including ResetIf should be reset.
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits()); // ResetIf HitCount should also be reset
    }

    // verifies that PauseIf stops HitCount processing
    TEST_METHOD(TestHitCountPauseIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(2)_0xH0002=52_P:0xL0004=4");

        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x54;
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x56;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x54;
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[4] = 0x56;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    // verifies that PauseIf prevents ResetIf processing
    TEST_METHOD(TestHitCountPauseIfResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(2)_R:0xH0002=50_P:0xL0004=4");

        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());

        memory[4] = 0x54; // pause
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());

        memory[2] = 50; // reset (but still paused)
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());

        memory[4] = 0x56; // unpause (still reset)
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());

        memory[2] = 52; // unreset
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
    }

    TEST_METHOD(TestAddSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "A:0xH0001=0_0xH0002=22");
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 4; // sum is correct
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[1] = 0; // first condition is true, but not sum
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 22; // first condition is true, sum is correct
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    TEST_METHOD(TestSubSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "B:0xH0002=0_0xH0001=14"); // NOTE: SubSource subtracts the first value from the second!
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 4; // difference is correct
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[1] = 0; // first condition is true, but not difference
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[2] = 14; // first condition is true, value is negative inverse of expected value
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[1] = 28; // difference is correct again
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    TEST_METHOD(TestAddSubSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "A:0xH0001=0_B:0xL0002=0_0xL0004=14"); // byte(1) - low(2) + low(4) == 14
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[1] = 12; // total is correct
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[1] = 0; // first condition is true, but not total
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[4] = 18; // byte(4) would make total true, but not low(4)
        AssertSetTest(set, false, false, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(2).CurrentHits());

        memory[2] = 1;
        memory[4] = 15; // difference is correct again
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits()); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(1).CurrentHits()); // SubSource condition does not have hit tracking
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(2).CurrentHits());
    }

    TEST_METHOD(TestAddHits)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "C:0xH0001=18(2)_0xL0004=6(4)"); // repeated(4, byte(1) == 18 || low(4) == 6)
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false); // total hits met (2 for each condition)
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, false, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits()); // threshold met, stop incrementing
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits()); // total met prevents incrementing even though individual tally has not reached total

        set.Reset();
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(1).CurrentHits());

        memory[1] = 16;
        AssertSetTest(set, false, true, false); // 1 + 2 < 4, not met
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(1).CurrentHits());

        AssertSetTest(set, true, true, false); // 1 + 3 = 4, met
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(1).CurrentHits());
    }

    TEST_METHOD(TestAltGroups)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=16S0xH0002=52S0xL0004=6");

        // core not true, both alts are
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[1] = 16; // core and both alts true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[4] = 0; // core and first alt true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[2] = 0; // core true, but neither alt is
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[4] = 6; // core and second alt true
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(4U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(2).GetAt(0).CurrentHits());
    }

    // verifies that a ResetIf resets everything regardless of where it is
    TEST_METHOD(TestResetIfInAltGroup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18(1)_R:0xH0000=1S0xH0002=52(1)S0xL0004=6(1)_R:0xH0000=2");

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 1; // reset in core group resets everything
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 0;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 2; // reset in alt group resets everything
        AssertSetTest(set, false, true, true);
        Assert::AreEqual(0U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(0U, set.GetGroup(2).GetAt(0).CurrentHits());
    }

    // verifies that PauseIf only pauses the group it's in
    TEST_METHOD(TestPauseIfInAltGroup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        ConditionSet set;
        const char* ptr;
        set.ParseFromString(ptr = "0xH0001=18_P:0xH0000=1S0xH0002=52S0xL0004=6_P:0xH0000=2");

        AssertSetTest(set, true, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(1U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 1; // pause in core group only pauses core group
        AssertSetTest(set, false, true, false);
        Assert::AreEqual(1U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(2U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 0;
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(2U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(2).GetAt(0).CurrentHits());

        memory[0] = 2; // reset in alt group only pauses alt group
        AssertSetTest(set, true, true, false);
        Assert::AreEqual(3U, set.GetGroup(0).GetAt(0).CurrentHits());
        Assert::AreEqual(4U, set.GetGroup(1).GetAt(0).CurrentHits());
        Assert::AreEqual(3U, set.GetGroup(2).GetAt(0).CurrentHits());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
