#include "CppUnitTest.h"

#include "RA_AchievementSet.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_Achievement_Tests)
{
public:
    TEST_METHOD(TestStateEmpty)
    {
        Achievement ach{ AchievementSet::Type::Local };
        std::string sState = ach.CreateStateString("user1");

        Assert::AreEqual(std::string("0:0:a9bf5b6918bb43ec1d430f09d6606fbd:d41d8cd98f00b204e9800998ecf8427e:"), sState);
    }

    TEST_METHOD(TestStateSimple)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=6");
        ach.GetCondition(0, 0).OverrideCurrentHits(4);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 12);
        ach.GetCondition(0, 0).CompTarget().SetValues(12, 12);

        std::string sState = ach.CreateStateString("user1");
        Assert::IsFalse(sState.empty());

        // incorrect user should cause the achievement to reset (which only affects hit count)
        const char* pIter = sState.c_str();
        pIter = ach.ParseStateString(pIter, "user2");
        Assert::AreEqual('\0', *pIter);
        Assert::AreEqual(0U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompSource().RawValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompSource().RawPreviousValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompTarget().RawValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompTarget().RawPreviousValue());

        // correct user should load
        pIter = sState.c_str();
        pIter = ach.ParseStateString(pIter, "user1");
        Assert::AreEqual('\0', *pIter);
        Assert::AreEqual(4U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompSource().RawValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompSource().RawPreviousValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompTarget().RawValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompTarget().RawPreviousValue());
    }

    TEST_METHOD(TestStateDelta)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=d0x1234");
        ach.GetCondition(0, 0).OverrideCurrentHits(4);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 11);
        ach.GetCondition(0, 0).CompTarget().SetValues(12, 11);

        std::string sState = ach.CreateStateString("user1");
        Assert::IsFalse(sState.empty());

        const char* pIter = sState.c_str();
        pIter = ach.ParseStateString(pIter, "user1");
        Assert::AreEqual('\0', *pIter);
        Assert::AreEqual(4U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompSource().RawValue());
        Assert::AreEqual(11U, ach.GetCondition(0, 0).CompSource().RawPreviousValue());
        Assert::AreEqual(12U, ach.GetCondition(0, 0).CompTarget().RawValue());
        Assert::AreEqual(11U, ach.GetCondition(0, 0).CompTarget().RawPreviousValue());
    }

    TEST_METHOD(TestStateMultipleConditions)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=6.1._0xh2345!=d0xh2345_R:0xh3456=7");
        ach.GetCondition(0, 0).OverrideCurrentHits(1);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 12);
        ach.GetCondition(0, 0).CompTarget().SetValues(6, 6);
        ach.GetCondition(0, 1).OverrideCurrentHits(0);
        ach.GetCondition(0, 1).CompSource().SetValues(32, 32);
        ach.GetCondition(0, 1).CompTarget().SetValues(32, 32);
        ach.GetCondition(0, 2).OverrideCurrentHits(1700);
        ach.GetCondition(0, 2).CompSource().SetValues(0, 0);
        ach.GetCondition(0, 2).CompTarget().SetValues(0, 0);

        std::string sState = ach.CreateStateString("user1");
        Assert::IsFalse(sState.empty());

        ach.Reset();
        Assert::AreEqual(0U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 2).CurrentHits());

        const char* pIter = sState.c_str();
        ach.ParseStateString(pIter, "user1");
        Assert::AreEqual(1U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(1700U, ach.GetCondition(0, 2).CurrentHits());
    }

    TEST_METHOD(TestStateMultipleGroups)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=6.1._0xh2345!=d0xh2345SR:0xh3456=7S0xh4567=0xh5678");
        ach.GetCondition(0, 0).OverrideCurrentHits(1);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 12);
        ach.GetCondition(0, 0).CompTarget().SetValues(6, 6);
        ach.GetCondition(0, 1).OverrideCurrentHits(0);
        ach.GetCondition(0, 1).CompSource().SetValues(32, 32);
        ach.GetCondition(0, 1).CompTarget().SetValues(32, 32);
        ach.GetCondition(1, 0).OverrideCurrentHits(1700);
        ach.GetCondition(1, 0).CompSource().SetValues(0, 0);
        ach.GetCondition(1, 0).CompTarget().SetValues(0, 0);
        ach.GetCondition(2, 0).OverrideCurrentHits(11);
        ach.GetCondition(2, 0).CompSource().SetValues(17, 17);
        ach.GetCondition(2, 0).CompTarget().SetValues(18, 18);

        std::string sState = ach.CreateStateString("user1");
        Assert::IsFalse(sState.empty());

        ach.Reset();
        Assert::AreEqual(0U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(1, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(2, 0).CurrentHits());

        const char* pIter = sState.c_str();
        ach.ParseStateString(pIter, "user1");
        Assert::AreEqual(1U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(1700U, ach.GetCondition(1, 0).CurrentHits());
        Assert::AreEqual(11U, ach.GetCondition(2, 0).CurrentHits());
    }

    TEST_METHOD(TestStateMultipleAchievements)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=6.1._0xh2345!=d0xh2345_R:0xh3456=7");
        ach.GetCondition(0, 0).OverrideCurrentHits(1);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 12);
        ach.GetCondition(0, 0).CompTarget().SetValues(6, 6);
        ach.GetCondition(0, 1).OverrideCurrentHits(0);
        ach.GetCondition(0, 1).CompSource().SetValues(32, 32);
        ach.GetCondition(0, 1).CompTarget().SetValues(32, 32);
        ach.GetCondition(0, 2).OverrideCurrentHits(1700);
        ach.GetCondition(0, 2).CompSource().SetValues(0, 0);
        ach.GetCondition(0, 2).CompTarget().SetValues(0, 0);

        std::string sState = ach.CreateStateString("user1");
        sState += "54321:0:junk:junk";
        Assert::IsFalse(sState.empty());

        ach.Reset();
        Assert::AreEqual(0U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 2).CurrentHits());

        const char* pIter = sState.c_str();
        pIter = ach.ParseStateString(pIter, "user1");
        Assert::AreEqual(1U, ach.GetCondition(0, 0).CurrentHits());
        Assert::AreEqual(0U, ach.GetCondition(0, 1).CurrentHits());
        Assert::AreEqual(1700U, ach.GetCondition(0, 2).CurrentHits());

        // pointer should be ready for next achievement
        Assert::AreEqual("54321:0:junk:junk", pIter);
    }

    TEST_METHOD(TestStateAchievementModified)
    {
        Achievement ach{ AchievementSet::Type::Local };
        ach.SetID(12345);
        ach.ParseLine("12345:0xh1234=6");
        ach.GetCondition(0, 0).OverrideCurrentHits(4);
        ach.GetCondition(0, 0).CompSource().SetValues(12, 12);
        ach.GetCondition(0, 0).CompTarget().SetValues(12, 12);

        std::string sState = ach.CreateStateString("user1");
        Assert::IsFalse(sState.empty());

        // if achievement has changed, achievement checksum will fail and achievement should
        // just be reset (which only affects hitcount)
        ach.GetCondition(0, 0).CompSource().Set(ComparisonVariableSize::EightBit,
            ComparisonVariableType::Address, 0x2345U);
        
        const char* pIter = sState.c_str();
        ach.ParseStateString(pIter, "user1");
        Assert::AreEqual(0U, ach.GetCondition(0, 0).CurrentHits());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
