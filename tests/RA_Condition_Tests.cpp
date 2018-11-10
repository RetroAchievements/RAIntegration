#include "RA_Condition.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_Condition_Tests)
{
public:
    
    // ===== CompVariable =====

    void AssertCompVariable(const CompVariable& var, CompVariable::Type nExpectedType, MemSize nExpectedSize, unsigned int nExpectedRawValue, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, var.GetType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSize, var.GetSize(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRawValue, var.GetValue(), wsSerialized.c_str());
    }

    void AssertSerialize(CompVariable::Type nType, MemSize nSize, unsigned int nValue, const char* sExpected)
    {
        CompVariable var;
        var.Set(nSize, nType, nValue);

        std::string sSerialized;
        var.SerializeAppend(sSerialized);

        Assert::AreEqual(sExpected, sSerialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestSerializeVariable)
    {
        // sizes
        AssertSerialize(CompVariable::Type::Address, MemSize::EightBit, 0x1234U, "0xH1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::SixteenBit, 0x1234U, "0x 1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::ThirtyTwoBit, 0x1234U, "0xX1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Nibble_Lower, 0x1234U, "0xL1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Nibble_Upper, 0x1234U, "0xU1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_0, 0x1234U, "0xM1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_1, 0x1234U, "0xN1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_2, 0x1234U, "0xO1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_3, 0x1234U, "0xP1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_4, 0x1234U, "0xQ1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_5, 0x1234U, "0xR1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_6, 0x1234U, "0xS1234");
        AssertSerialize(CompVariable::Type::Address, MemSize::Bit_7, 0x1234U, "0xT1234");

        // delta
        AssertSerialize(CompVariable::Type::DeltaMem, MemSize::EightBit, 0x1234U, "d0xH1234");

        // value (size is ignored)
        AssertSerialize(CompVariable::Type::ValueComparison, MemSize::EightBit, 123, "123");
    }

    TEST_METHOD(TestVariableSet)
    {
        CompVariable var;

        var.Set(MemSize::SixteenBit, CompVariable::Type::DeltaMem, 0x1111);
        AssertCompVariable(var, CompVariable::Type::DeltaMem, MemSize::SixteenBit, 0x1111, "d0x1111");

        var.SetSize(MemSize::EightBit);
        AssertCompVariable(var, CompVariable::Type::DeltaMem, MemSize::EightBit, 0x1111, "d0xH1111");

        var.SetType(CompVariable::Type::Address);
        AssertCompVariable(var, CompVariable::Type::Address, MemSize::EightBit, 0x1111, "0xH1111");

        var.SetValue(0x1234);
        AssertCompVariable(var, CompVariable::Type::Address, MemSize::EightBit, 0x1234, "0xH1234");
    }

    // ===== Condition =====

    void AssertCondition(const Condition& cond, Condition::Type nExpectedType, ComparisonType nExpectedComparison, unsigned int nExpectedRequiredHits, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, cond.GetConditionType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedComparison, cond.CompareType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRequiredHits, cond.RequiredHits(), wsSerialized.c_str());
    }

    void AssertSerialize(Condition::Type nCondType, CompVariable::Type nSrcType, MemSize nSrcSize, unsigned int nSrcValue, ComparisonType nComparisonType,
        CompVariable::Type nTgtType, MemSize nTgtSize, unsigned int nTgtValue, const char* sExpected, unsigned int nRequiredHits = 0)
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
        AssertSerialize(Condition::Type::Standard, 
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U, 
            ComparisonType::Equals, 
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234=8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::NotEqualTo,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234!=8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::LessThan,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234<8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::LessThanOrEqual,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234<=8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::GreaterThan,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234>8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::GreaterThanOrEqual,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234>=8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::Address, MemSize::EightBit, 0x4321U, "0xH1234=0xH4321");

        // delta
        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::DeltaMem, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "d0xH1234=8");

        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::DeltaMem, MemSize::EightBit, 0x4321U, "0xH1234=d0xH4321");

        // flags
        AssertSerialize(Condition::Type::ResetIf,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "R:0xH1234=8");

        AssertSerialize(Condition::Type::PauseIf,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "P:0xH1234=8");

        AssertSerialize(Condition::Type::AddSource,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "A:0xH1234=8");

        AssertSerialize(Condition::Type::SubSource,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "B:0xH1234=8");

        AssertSerialize(Condition::Type::AddHits,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "C:0xH1234=8");

        // hit count
        AssertSerialize(Condition::Type::Standard,
            CompVariable::Type::Address, MemSize::EightBit, 0x1234U,
            ComparisonType::Equals,
            CompVariable::Type::ValueComparison, MemSize::EightBit, 8U, "0xH1234=8.1.", 1);
    }

    TEST_METHOD(TestConditionSet)
    {
        Condition cond;
        AssertCondition(cond, Condition::Type::Standard, ComparisonType::Equals, 0, "");

        cond.SetCompareType(ComparisonType::NotEqualTo);
        AssertCondition(cond, Condition::Type::Standard, ComparisonType::NotEqualTo, 0, "NotEqualTo");

        cond.SetConditionType(Condition::Type::ResetIf);
        AssertCondition(cond, Condition::Type::ResetIf, ComparisonType::NotEqualTo, 0, "ResetIf");

        cond.SetRequiredHits(5);
        AssertCondition(cond, Condition::Type::ResetIf, ComparisonType::NotEqualTo, 5, "5");
    }

    TEST_METHOD(TestConditionType)
    {
        Condition::Type types[] = {
            Condition::Type::Standard,
            Condition::Type::ResetIf,
            Condition::Type::PauseIf,
            Condition::Type::AddSource,
            Condition::Type::SubSource,
            Condition::Type::AddHits,
        };

        for (const auto type : types)
        {
            Condition cond;
            cond.SetConditionType(type);

            Assert::AreEqual(cond.IsResetCondition(), (type == Condition::Type::ResetIf));
            Assert::AreEqual(cond.IsPauseCondition(), (type == Condition::Type::PauseIf));
            Assert::AreEqual(cond.IsAddCondition(), (type == Condition::Type::AddSource));
            Assert::AreEqual(cond.IsSubCondition(), (type == Condition::Type::SubSource));
            Assert::AreEqual(cond.IsAddHitsCondition(), (type == Condition::Type::AddHits));
        }
    }
};

} // namespace tests
} // namespace data
} // namespace ra
