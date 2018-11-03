#include "RA_MemValue.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

class MemValueHarness : public MemValue
{
public:
    size_t NumValues() const { return m_vClauses.size(); }
    const Clause& GetMemValue(size_t index) const { return m_vClauses[index]; }
    bool IsAddition(size_t index) const { return m_vClauses[index].GetOperation() == ClauseOperation::Addition; }
    bool IsMaximum(size_t index) const { return m_vClauses[index].GetOperation() == ClauseOperation::Maximum; }

    class ClauseHarness : public Clause
    {
    public:
        ClauseHarness() : Clause(ClauseOperation::None) {}

        unsigned int GetAddress() const { return m_nAddress; }
        MemSize GetVarSize() const { return m_nVarSize; }
        double GetModifier() const { return m_fModifier; }
        bool GetBcdParse() const { return m_bBCDParse; }
        bool GetParseVal() const { return m_bParseVal; }
        bool GetInvertBit() const { return m_bInvertBit; }
        unsigned int GetSecondAddress() const { return m_nSecondAddress; }
        MemSize GetSecondVarSize() const { return m_nSecondVarSize; }
    };
};

TEST_CLASS(RA_MemValue_Tests)
{
    void AssertParseFromString(const char* sSerialized, MemSize nExpectedVarSize, unsigned int nExpectedAddress, bool bExpectedBcdParse = false, bool bExpectedParseVal = false)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        MemValueHarness::ClauseHarness value;
        Assert::AreEqual("", value.ParseFromString(sSerialized));

        Assert::AreEqual(nExpectedVarSize, value.GetVarSize(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedAddress, value.GetAddress(), wsSerialized.c_str());
        Assert::AreEqual(bExpectedBcdParse, value.GetBcdParse(), wsSerialized.c_str());
        Assert::AreEqual(bExpectedParseVal, value.GetParseVal(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetInvertBit(), wsSerialized.c_str());
        Assert::AreEqual(1.0, value.GetModifier(), wsSerialized.c_str());
        Assert::AreEqual(0U, value.GetSecondAddress(), wsSerialized.c_str());
        Assert::AreEqual(MemSize::EightBit, value.GetSecondVarSize(), wsSerialized.c_str());
    }

    void AssertParseFromString(const char* sSerialized, MemSize nExpectedVarSize, unsigned int nExpectedAddress, double fExpectedModifier)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        MemValueHarness::ClauseHarness value;
        Assert::AreEqual("", value.ParseFromString(sSerialized));

        Assert::AreEqual(nExpectedVarSize, value.GetVarSize(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedAddress, value.GetAddress(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetBcdParse(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetParseVal(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetInvertBit(), wsSerialized.c_str());
        Assert::AreEqual(fExpectedModifier, value.GetModifier(), wsSerialized.c_str());
        Assert::AreEqual(0U, value.GetSecondAddress(), wsSerialized.c_str());
        Assert::AreEqual(MemSize::EightBit, value.GetSecondVarSize(), wsSerialized.c_str());
    }

    void AssertParseFromString(const char* sSerialized, MemSize nExpectedVarSize, unsigned int nExpectedAddress, MemSize nExpectedSecondVarSize, unsigned int nExpectedSecondAddress)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        MemValueHarness::ClauseHarness value;
        Assert::AreEqual("", value.ParseFromString(sSerialized));

        Assert::AreEqual(nExpectedVarSize, value.GetVarSize(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedAddress, value.GetAddress(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetBcdParse(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetParseVal(), wsSerialized.c_str());
        Assert::AreEqual(false, value.GetInvertBit(), wsSerialized.c_str());
        Assert::AreEqual(1.0, value.GetModifier(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSecondAddress, value.GetSecondAddress(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSecondVarSize, value.GetSecondVarSize(), wsSerialized.c_str());
    }

    void AssertGetValue(const char* sSerialized, double dExpectedValue)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        MemValueHarness::ClauseHarness value;
        Assert::AreEqual("", value.ParseFromString(sSerialized));
        Assert::AreEqual(dExpectedValue, value.GetValue(), wsSerialized.c_str());
    }

public:
    TEST_METHOD(TestClauseParseFromString)
    {
        // sizes
        AssertParseFromString("0xH1234", MemSize::EightBit, 0x1234U);
        AssertParseFromString("0x 1234", MemSize::SixteenBit, 0x1234U);
        AssertParseFromString("0x1234", MemSize::SixteenBit, 0x1234U);
        AssertParseFromString("0xX1234", MemSize::ThirtyTwoBit, 0x1234U);
        AssertParseFromString("0xL1234", MemSize::Nibble_Lower, 0x1234U);
        AssertParseFromString("0xU1234", MemSize::Nibble_Upper, 0x1234U);
        AssertParseFromString("0xM1234", MemSize::Bit_0, 0x1234U);
        AssertParseFromString("0xN1234", MemSize::Bit_1, 0x1234U);
        AssertParseFromString("0xO1234", MemSize::Bit_2, 0x1234U);
        AssertParseFromString("0xP1234", MemSize::Bit_3, 0x1234U);
        AssertParseFromString("0xQ1234", MemSize::Bit_4, 0x1234U);
        AssertParseFromString("0xR1234", MemSize::Bit_5, 0x1234U);
        AssertParseFromString("0xS1234", MemSize::Bit_6, 0x1234U);
        AssertParseFromString("0xT1234", MemSize::Bit_7, 0x1234U);

        // BCD
        AssertParseFromString("B0xH1234", MemSize::EightBit, 0x1234U, true);
        AssertParseFromString("B0xX1234", MemSize::ThirtyTwoBit, 0x1234U, true);
        AssertParseFromString("b0xH1234", MemSize::EightBit, 0x1234U, true);

        // Value (values don't actually have size, EightBit is the default)
        AssertParseFromString("V0x1234", MemSize::SixteenBit, 0x1234, false, true); // hex is indirectly supported through using CompVariable to parse the value/address
        AssertParseFromString("V1234", MemSize::EightBit, 1234, false, true);
        AssertParseFromString("V+1", MemSize::EightBit, 1, false, true);
        AssertParseFromString("V-1", MemSize::EightBit, 0xFFFFFFFFU, false, true);
        AssertParseFromString("V-2", MemSize::EightBit, 0xFFFFFFFEU, false, true); // twos compliment still works for addition
    }

    TEST_METHOD(TestClauseParseFromStringMultiply)
    {
        AssertParseFromString("0xH1234", MemSize::EightBit, 0x1234U, 1.0);
        AssertParseFromString("0xH1234*1", MemSize::EightBit, 0x1234U, 1.0);
        AssertParseFromString("0xH1234*3", MemSize::EightBit, 0x1234U, 3.0);
        AssertParseFromString("0xH1234*0.5", MemSize::EightBit, 0x1234U, 0.5);
        AssertParseFromString("0xH1234*-1", MemSize::EightBit, 0x1234U, -1.0);
    }

    TEST_METHOD(TestClauseParseFromStringMultiplyAddress)
    {
        AssertParseFromString("0xH1234", MemSize::EightBit, 0x1234U, MemSize::EightBit, 0U);
        AssertParseFromString("0xH1234*0xH3456", MemSize::EightBit, 0x1234U, MemSize::EightBit, 0x3456U);
        AssertParseFromString("0xH1234*0xL2222", MemSize::EightBit, 0x1234U, MemSize::Nibble_Lower, 0x2222U);
        AssertParseFromString("0xH1234*0x1111", MemSize::EightBit, 0x1234U, MemSize::SixteenBit, 0x1111U);
    }

    TEST_METHOD(TestClauseGetValue)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        // value
        AssertGetValue("V0x1234", 0x1234);
        AssertGetValue("V6", 6);
        AssertGetValue("V6*2", 12);
        AssertGetValue("V6*0.5", 3);

        // memory
        AssertGetValue("0xH01", 0x12);
        AssertGetValue("0x0001", 0x3412);

        // BCD encoding
        AssertGetValue("B0xH01", 12);
        AssertGetValue("B0x0001", 12);               // BCD only applies to EightBit values

                                                     // multiplication
        AssertGetValue("0xH01*4", 0x12 * 4);         // multiply by constant
        AssertGetValue("0xH01*0.5", 0x12 / 2);       // multiply by fraction
        AssertGetValue("0xH01*0xH02", 0x12 * 0x34);  // multiply by second address
        AssertGetValue("0xH01*0xT02", 0);            // multiply by bit
        AssertGetValue("0xH01*~0xT02", 0x12);        // multiply by inverse bit
        AssertGetValue("0xH01*~0xH02", 0x12 * 0x34); // ~ only applies to bit sized secondary memory addresses
    }

    TEST_METHOD(TestAdditionSimple)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        MemValueHarness set;
        Assert::AreEqual("", set.ParseFromString("0xH0001_0xH0002"));
        Assert::AreEqual(2U, set.NumValues());
        Assert::AreEqual(false, set.IsAddition(0));
        Assert::AreEqual(false, set.IsMaximum(0));
        Assert::AreEqual(true, set.IsAddition(1));
        Assert::AreEqual(false, set.IsMaximum(1));

        Assert::AreEqual(0x12U + 0x34U, set.GetValue());
    }

    TEST_METHOD(TestAdditionComplex)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        MemValueHarness set;
        Assert::AreEqual("", set.ParseFromString("0xH0001*100_0xH0002*0.5_0xL0003"));
        Assert::AreEqual(3U, set.NumValues());
        Assert::AreEqual(false, set.IsAddition(0));
        Assert::AreEqual(false, set.IsMaximum(0));
        Assert::AreEqual(true, set.IsAddition(1));
        Assert::AreEqual(false, set.IsMaximum(1));
        Assert::AreEqual(true, set.IsAddition(2));
        Assert::AreEqual(false, set.IsMaximum(2));

        Assert::AreEqual(0x12U * 100 + 0x34U / 2 + 0x0B, set.GetValue());
    }

    TEST_METHOD(TestAdditionValue)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        MemValueHarness set;
        Assert::AreEqual("", set.ParseFromString("0xH0001_v1"));
        Assert::AreEqual(2U, set.NumValues());
        Assert::AreEqual(false, set.IsAddition(0));
        Assert::AreEqual(false, set.IsMaximum(0));
        Assert::AreEqual(true, set.IsAddition(1));
        Assert::AreEqual(false, set.IsMaximum(1));

        Assert::AreEqual(0x12U + 1U, set.GetValue());

        MemValueHarness set2;
        Assert::AreEqual("", set2.ParseFromString("0xH0001_v-1"));
        Assert::AreEqual(2U, set2.NumValues());
        Assert::AreEqual(false, set2.IsAddition(0));
        Assert::AreEqual(false, set2.IsMaximum(0));
        Assert::AreEqual(true, set2.IsAddition(1));
        Assert::AreEqual(false, set2.IsMaximum(1));

        Assert::AreEqual(0x12U - 1U, set2.GetValue());
    }

    TEST_METHOD(TestMaximumSimple)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        MemValueHarness set;
        Assert::AreEqual("", set.ParseFromString("0xH0001$0xH0002"));
        Assert::AreEqual(2U, set.NumValues());
        Assert::AreEqual(false, set.IsAddition(0));
        Assert::AreEqual(false, set.IsMaximum(0));
        Assert::AreEqual(false, set.IsAddition(1));
        Assert::AreEqual(true, set.IsMaximum(1));

        Assert::AreEqual(0x34U, set.GetValue());
    }

    TEST_METHOD(TestMaximumComplex)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        MemValueHarness set;
        Assert::AreEqual("", set.ParseFromString("0xH0001_0xH0004*3$0xH0002*0xL0003"));
        Assert::AreEqual(3U, set.NumValues());
        Assert::AreEqual(false, set.IsAddition(0));
        Assert::AreEqual(false, set.IsMaximum(0));
        Assert::AreEqual(true, set.IsAddition(1));
        Assert::AreEqual(false, set.IsMaximum(1));
        Assert::AreEqual(false, set.IsAddition(2));
        Assert::AreEqual(true, set.IsMaximum(2));

        // 0x12 + 0x56 * 3 <> 0x34 * 0xB (0x114 <> 0x23C)
        Assert::AreEqual(0x34U * 0xBU, set.GetValue());
    }

    TEST_METHOD(TestFormatValue)
    {
        Assert::AreEqual("12345", MemValue::FormatValue(12345, MemValue::Format::Value).c_str());
        Assert::AreEqual("012345", MemValue::FormatValue(12345, MemValue::Format::Other).c_str());
        Assert::AreEqual("012345 Points", MemValue::FormatValue(12345, MemValue::Format::Score).c_str());
        Assert::AreEqual("205:45", MemValue::FormatValue(12345, MemValue::Format::TimeSecs).c_str());
        Assert::AreEqual("02:03.45", MemValue::FormatValue(12345, MemValue::Format::TimeMillisecs).c_str());
        Assert::AreEqual("03:25.75", MemValue::FormatValue(12345, MemValue::Format::TimeFrames).c_str());
        Assert::AreEqual("05:45", MemValue::FormatValue(345, MemValue::Format::TimeSecs).c_str());
        Assert::AreEqual("00:03.45", MemValue::FormatValue(345, MemValue::Format::TimeMillisecs).c_str());
        Assert::AreEqual("00:05.75", MemValue::FormatValue(345, MemValue::Format::TimeFrames).c_str());
    }

    TEST_METHOD(TestParseMemValueFormat)
    {
        Assert::AreEqual(MemValue::Format::Value, MemValue::ParseFormat("VALUE"));
        Assert::AreEqual(MemValue::Format::TimeSecs, MemValue::ParseFormat("SECS"));
        Assert::AreEqual(MemValue::Format::TimeSecs, MemValue::ParseFormat("TIMESECS"));
        Assert::AreEqual(MemValue::Format::TimeFrames, MemValue::ParseFormat("TIME"));
        Assert::AreEqual(MemValue::Format::TimeFrames, MemValue::ParseFormat("FRAMES"));
        Assert::AreEqual(MemValue::Format::Score, MemValue::ParseFormat("SCORE"));
        Assert::AreEqual(MemValue::Format::Score, MemValue::ParseFormat("POINTS"));
        Assert::AreEqual(MemValue::Format::TimeMillisecs, MemValue::ParseFormat("MILLISECS"));
        Assert::AreEqual(MemValue::Format::Other, MemValue::ParseFormat("OTHER"));
        Assert::AreEqual(MemValue::Format::Value, MemValue::ParseFormat("INVALID"));
    }

    TEST_METHOD(TestGetMemValueFormatString)
    {
        Assert::AreEqual("OTHER", MemValue::GetFormatString(MemValue::Format::Other));
        Assert::AreEqual("POINTS", MemValue::GetFormatString(MemValue::Format::Score));
        Assert::AreEqual("FRAMES", MemValue::GetFormatString(MemValue::Format::TimeFrames));
        Assert::AreEqual("MILLISECS", MemValue::GetFormatString(MemValue::Format::TimeMillisecs));
        Assert::AreEqual("SECS", MemValue::GetFormatString(MemValue::Format::TimeSecs));
        Assert::AreEqual("VALUE", MemValue::GetFormatString(MemValue::Format::Value));
    }
};

} // namespace tests
} // namespace data
} // namespace ra
