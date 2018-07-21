#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_Condition_Tests)
{
public:
    void AssertCondition(const Condition& cond, Condition::ConditionType nExpectedType, ComparisonType nExpectedComparison, unsigned int nExpectedRequiredHits, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, cond.GetConditionType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedComparison, cond.CompareType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRequiredHits, cond.RequiredHits(), wsSerialized.c_str());
    }

    void AssertSerialize(Condition::ConditionType nCondType, ComparisonVariableType nSrcType, ComparisonVariableSize nSrcSize, unsigned int nSrcValue, ComparisonType nComparisonType,
        ComparisonVariableType nTgtType, ComparisonVariableSize nTgtSize, unsigned int nTgtValue, const char* sExpected, unsigned int nRequiredHits = 0)
    {
        Condition condition;
        condition.CompSource().Set(nSrcSize, nSrcType, nSrcValue);
        condition.CompTarget().Set(nTgtSize, nTgtType, nTgtValue);
        condition.SetConditionType(nCondType);
        condition.SetCompareType(nComparisonType);
        condition.SetRequiredHits(nRequiredHits);

        std::string sSerialized;
        condition.SerializeAppend(sSerialized);

        Assert::AreEqual(sExpected, sSerialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestSerialize)
    {
        // comparisons
        AssertSerialize(Condition::ConditionType::Standard, 
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U, 
            ComparisonType::Equals, 
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234=8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::NotEqualTo,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234!=8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::LessThan,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234<8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::LessThanOrEqual,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234<=8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::GreaterThan,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234>8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::GreaterThanOrEqual,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234>=8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x4321U, "0xH1234=0xH4321");

        // delta
        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::DeltaMem, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "d0xH1234=8");

        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::DeltaMem, ComparisonVariableSize::EightBit, 0x4321U, "0xH1234=d0xH4321");

        // flags
        AssertSerialize(Condition::ConditionType::ResetIf,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "R:0xH1234=8");

        AssertSerialize(Condition::ConditionType::PauseIf,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "P:0xH1234=8");

        AssertSerialize(Condition::ConditionType::AddSource,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "A:0xH1234=8");

        AssertSerialize(Condition::ConditionType::SubSource,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "B:0xH1234=8");

        AssertSerialize(Condition::ConditionType::AddHits,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "C:0xH1234=8");

        // hit count
        AssertSerialize(Condition::ConditionType::Standard,
            ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 8U, "0xH1234=8.1.", 1);
    }

    TEST_METHOD(TestConditionSet)
    {
        Condition cond;
        AssertCondition(cond, Condition::Standard, Equals, 0, "");

        cond.SetCompareType(NotEqualTo);
        AssertCondition(cond, Condition::Standard, NotEqualTo, 0, "NotEqualTo");

        cond.SetConditionType(Condition::ResetIf);
        AssertCondition(cond, Condition::ResetIf, NotEqualTo, 0, "ResetIf");

        cond.SetRequiredHits(5);
        AssertCondition(cond, Condition::ResetIf, NotEqualTo, 5, "5");
    }

    TEST_METHOD(TestConditionType)
    {
        Condition::ConditionType types[] = {
            Condition::Standard,
            Condition::ResetIf,
            Condition::PauseIf,
            Condition::AddSource,
            Condition::SubSource,
            Condition::AddHits,
        };

        for (auto type : types)
        {
            Condition cond;
            cond.SetConditionType(type);

            Assert::AreEqual(cond.IsResetCondition(), (type == Condition::ResetIf));
            Assert::AreEqual(cond.IsPauseCondition(), (type == Condition::PauseIf));
            Assert::AreEqual(cond.IsAddCondition(), (type == Condition::AddSource));
            Assert::AreEqual(cond.IsSubCondition(), (type == Condition::SubSource));
            Assert::AreEqual(cond.IsAddHitsCondition(), (type == Condition::AddHits));
        }
    }
};

} // namespace tests
} // namespace data
} // namespace ra
