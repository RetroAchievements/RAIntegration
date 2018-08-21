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
    void AssertCompVariable(const CompVariable& var, ComparisonVariableType nExpectedType, ComparisonVariableSize nExpectedSize, unsigned int nExpectedRawValue, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, var.Type(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSize, var.Size(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRawValue, var.RawValue(), wsSerialized.c_str());
    }

    void AssertCondition(const Condition& cond, Condition::ConditionType nExpectedType, ComparisonType nExpectedComparison, unsigned int nExpectedRequiredHits, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, cond.GetConditionType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedComparison, cond.CompareType(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRequiredHits, cond.RequiredHits(), wsSerialized.c_str());
    }

    void AssertParseCondition(const char* sSerialized, Condition::ConditionType nExpectedConditionType,
        ComparisonVariableType nExpectedLeftType, ComparisonVariableSize nExpectedLeftSize, unsigned int nExpectedLeftValue,
        ComparisonType nExpectedComparison,
        ComparisonVariableType nExpectedRightType, ComparisonVariableSize nExpectedRightSize, unsigned int nExpectedRightValue,
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
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234==8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234!=8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, NotEqualTo, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234<8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, LessThan, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234<=8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, LessThanOrEqual, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234>8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, GreaterThan, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("0xH1234>=8",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, GreaterThanOrEqual, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);

        // delta
        AssertParseCondition("d0xH1234=8",
            Condition::Standard, DeltaMem, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);

        // flags
        AssertParseCondition("R:0xH1234=8",
            Condition::ResetIf, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("P:0xH1234=8",
            Condition::PauseIf, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("A:0xH1234=8",
            Condition::AddSource, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("B:0xH1234=8",
            Condition::SubSource, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);
        AssertParseCondition("C:0xH1234=8",
            Condition::AddHits, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 0);

        // hit count
        AssertParseCondition("0xH1234=8(1)",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 1);
        AssertParseCondition("0xH1234=8.1.", // legacy format
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 1);
        AssertParseCondition("0xH1234=8(100)",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, ValueComparison, ComparisonVariableSize::EightBit, 8U, 100);
    }

    TEST_METHOD(TestParseConditionMemoryComparisonHexValue)
    {
        // hex value is interpreted as a 16-bit memory reference
        AssertParseCondition("0xH1234=0x80",
            Condition::Standard, Address, ComparisonVariableSize::EightBit, 0x1234U, Equals, Address, ComparisonVariableSize::SixteenBit, 0x80U, 0);
    }

    TEST_METHOD(TestParseConditionMemoryComparisonMemory)
    {
        AssertParseCondition("0xL1234!=0xU3456",
            Condition::Standard, Address, ComparisonVariableSize::Nibble_Lower, 0x1234U, NotEqualTo, Address, ComparisonVariableSize::Nibble_Upper, 0x3456U, 0);
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
