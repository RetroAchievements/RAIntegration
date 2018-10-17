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
    void AssertCompVariable(const CompVariable& var, CompVariable::Type nExpectedType, ComparisonVariableSize nExpectedSize, unsigned int nExpectedRawValue, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, var.GetType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSize, var.Size(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRawValue, var.RawValue(), wsSerialized.c_str());
    }

    void AssertCondition(const Condition& cond, Condition::Type nExpectedType, ComparisonType nExpectedComparison, unsigned int nExpectedRequiredHits, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, cond.GetConditionType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedComparison, cond.CompareType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRequiredHits, cond.RequiredHits(), wsSerialized.c_str());
    }

    void AssertParseCondition(const char* sSerialized, Condition::Type nExpectedConditionType,
        CompVariable::Type nExpectedLeftType, ComparisonVariableSize nExpectedLeftSize, unsigned int nExpectedLeftValue,
        ComparisonType nExpectedComparison,
        CompVariable::Type nExpectedRightType, ComparisonVariableSize nExpectedRightSize, unsigned int nExpectedRightValue,
        unsigned int nExpectedRequiredHits)
    {
        const char *ptr = sSerialized;
        Condition cond;
        cond.ParseFromString(ptr);
        Assert::AreEqual(*ptr, '\0');

        AssertCondition(cond, nExpectedConditionType, nExpectedComparison, nExpectedRequiredHits, sSerialized);
        AssertCompVariable(cond.CompSource(), nExpectedLeftType, nExpectedLeftSize, nExpectedLeftValue, sSerialized);
        AssertCompVariable(cond.CompTarget(), nExpectedRightType, nExpectedRightSize, nExpectedRightValue, sSerialized);
    }

    void AssertSerialize(const char* sSerialized, const char* sExpected = nullptr)
    {
        if (sExpected == nullptr)
            sExpected = sSerialized;

        const char* ptr = sSerialized;
        Condition condition;
        condition.ParseFromString(ptr);

        std::string sReserialized;
        condition.SerializeAppend(sReserialized);

        Assert::AreEqual(sExpected, sReserialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestParseConditionMemoryComparisonValue)
    {
        // different comparisons
        AssertParseCondition("0xH1234=8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234==8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234!=8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, NotEqualTo,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234<8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, LessThan,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234<=8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, LessThanOrEqual,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234>8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, GreaterThan,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("0xH1234>=8",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, GreaterThanOrEqual,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);

        // delta
        AssertParseCondition("d0xH1234=8",
            Condition::Type::Standard, CompVariable::Type::DeltaMem, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);

        // flags
        AssertParseCondition("R:0xH1234=8",
            Condition::Type::ResetIf, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("P:0xH1234=8",
            Condition::Type::PauseIf, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("A:0xH1234=8",
            Condition::Type::AddSource, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("B:0xH1234=8",
            Condition::Type::SubSource, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);
        AssertParseCondition("C:0xH1234=8",
            Condition::Type::AddHits, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 0);

        // hit count
        AssertParseCondition("0xH1234=8(1)",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 1);
        AssertParseCondition("0xH1234=8.1.", // legacy format
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 1);
        AssertParseCondition("0xH1234=8(100)",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 8U, 100);
    }

    TEST_METHOD(TestParseConditionMemoryComparisonHexValue)
    {
        // hex value is interpreted as a 16-bit memory reference
        AssertParseCondition("0xH1234=0x80",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::Address, SixteenBit, 0x80U, 0);

        // h prefix indicates hex value
        AssertParseCondition("0xH1234=h80",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 0x80U, 0);

        AssertParseCondition("0xH1234=h0x80",
            Condition::Type::Standard, CompVariable::Type::Address, EightBit, 0x1234U, Equals,
            CompVariable::Type::ValueComparison, EightBit, 0x80U, 0);
    }

    TEST_METHOD(TestParseConditionMemoryComparisonMemory)
    {
        AssertParseCondition("0xL1234!=0xU3456",
            Condition::Type::Standard, CompVariable::Type::Address, Nibble_Lower, 0x1234U, NotEqualTo,
            CompVariable::Type::Address, Nibble_Upper, 0x3456U, 0);
    }

    TEST_METHOD(TestSerialize)
    {
        // comparisons
        AssertSerialize("0xH1234=8");
        AssertSerialize("0xH1234!=8");
        AssertSerialize("0xH1234<8");
        AssertSerialize("0xH1234<=8");
        AssertSerialize("0xH1234>8");
        AssertSerialize("0xH1234>=8");
        AssertSerialize("0xH1234=0xH4321");

        // delta
        AssertSerialize("d0xH1234=8");

        // flags
        AssertSerialize("R:0xH1234=8");
        AssertSerialize("P:0xH1234=8");
        AssertSerialize("A:0xH1234=8");
        AssertSerialize("B:0xH1234=8");
        AssertSerialize("C:0xH1234=8");

        // hit count
        AssertSerialize("0xH1234=8.1.");
        AssertSerialize("0xH1234=8(1)", "0xH1234=8.1.");
    }

    TEST_METHOD(TestConditionSet)
    {
        Condition cond;
        AssertCondition(cond, Condition::Type::Standard, Equals, 0, "");

        cond.SetCompareType(NotEqualTo);
        AssertCondition(cond, Condition::Type::Standard, NotEqualTo, 0, "NotEqualTo");

        cond.SetConditionType(Condition::Type::ResetIf);
        AssertCondition(cond, Condition::Type::ResetIf, NotEqualTo, 0, "ResetIf");

        cond.SetRequiredHits(5);
        AssertCondition(cond, Condition::Type::ResetIf, NotEqualTo, 5, "5");
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

        for (auto type : types)
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

    TEST_METHOD(TestConditionHits)
    {
        Condition cond;
        cond.SetRequiredHits(5);
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(0U, cond.CurrentHits());

        cond.IncrHits();
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(1U, cond.CurrentHits());

        cond.IncrHits();
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(2U, cond.CurrentHits());

        cond.IncrHits();
        cond.IncrHits();
        cond.IncrHits();
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(5U, cond.CurrentHits());

        cond.IncrHits();
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(6U, cond.CurrentHits());

        cond.OverrideCurrentHits(4U);
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(4U, cond.CurrentHits());

        cond.IncrHits();
        Assert::AreEqual(5U, cond.RequiredHits());
        Assert::AreEqual(5U, cond.CurrentHits());
    }

    void AssertConditionCompare(const char* def, bool expectedResult)
    {
        const char* ptr = def;
        Condition cond;
        cond.ParseFromString(ptr);

        Assert::AreEqual(cond.Compare(), expectedResult);
    }

    TEST_METHOD(TestConditionCompare)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        // values
        AssertConditionCompare("0xH0001=18", true);
        AssertConditionCompare("0xH0001!=18", false);
        AssertConditionCompare("0xH0001<=18", true);
        AssertConditionCompare("0xH0001>=18", true);
        AssertConditionCompare("0xH0001<18", false);
        AssertConditionCompare("0xH0001>18", false);
        AssertConditionCompare("0xH0001>0", true);
        AssertConditionCompare("0xH0001!=0", true);

        // memory
        AssertConditionCompare("0xH0001<0xH0002", true);
        AssertConditionCompare("0xH0001>0xH0002", false);
        AssertConditionCompare("0xH0001=0xH0001", true);
        AssertConditionCompare("0xH0001!=0xH0002", true);
    }

    TEST_METHOD(TestConditionCompareDelta)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        const char *ptr = "0xH0001>d0xH0001";
        Condition cond;
        cond.ParseFromString(ptr);

        Assert::AreEqual(cond.Compare(), true); // initial delta value is 0, 0x12 > 0

        Assert::AreEqual(cond.Compare(), false); // delta value is now 0x12, 0x12 = 0x12

        memory[1] = 0x11;
        Assert::AreEqual(cond.Compare(), false); // delta value is now 0x12, 0x11 < 0x12

        memory[1] = 0x12;
        Assert::AreEqual(cond.Compare(), true); // delta value is now 0x13, 0x12 > 0x11
    }

    TEST_METHOD(TestConditionCompare32Bits)
    {
        unsigned char memory[] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
        InitializeMemory(memory, 6);

        // values
        AssertConditionCompare("0xX0000=4294967295", false);
        AssertConditionCompare("0xX0001=4294967295", true);
        AssertConditionCompare("0xX0002=4294967295", false);

        // memory
        AssertConditionCompare("0xX0000<0xX0001", true);
        AssertConditionCompare("0xX0001>0xX0002", true);
        AssertConditionCompare("0xX0001=0xX0002", false);
        AssertConditionCompare("0xX0000!=0xH0002", true);
    }
};

} // namespace tests
} // namespace data
} // namespace ra
