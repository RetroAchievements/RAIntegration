#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_CompVariable_Tests)
{
public:
    void AssertCompVariable(const CompVariable& var, ComparisonVariableType nExpectedType, MemSize nExpectedSize, unsigned int nExpectedRawValue, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, var.Type(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSize, var.Size(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRawValue, var.RawValue(), wsSerialized.c_str());
    }

    void AssertParseCompVariable(const char* sSerialized, ComparisonVariableType nExpectedType, MemSize nExpectedSize, unsigned int nExpectedRawValue)
    {
        const char* ptr = sSerialized;
        CompVariable var;
        var.ParseVariable(ptr);
        Assert::AreEqual(*ptr, '\0');

        AssertCompVariable(var, nExpectedType, nExpectedSize, nExpectedRawValue, sSerialized);
    }

    void AssertParseErrorCompVariable(const char* sSerialized, ComparisonVariableType nExpectedType, MemSize nExpectedSize, unsigned int nExpectedRawValue, size_t nExpectedValidChars)
    {
        const char* ptr = sSerialized;
        CompVariable var;
        var.ParseVariable(ptr);
        Assert::AreEqual(*ptr, sSerialized[nExpectedValidChars]);

        AssertCompVariable(var, nExpectedType, nExpectedSize, nExpectedRawValue, sSerialized);
    }

    void AssertSerialize(const char* sSerialized, const char* sExpected = nullptr)
    {
        if (sExpected == nullptr)
            sExpected = sSerialized;

        const char* ptr = sSerialized;
        CompVariable var;
        var.ParseVariable(ptr);

        std::string sReserialized;
        var.SerializeAppend(sReserialized);

        Assert::AreEqual(sExpected, sReserialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestParseVariableAddress)
    {
        // sizes
        AssertParseCompVariable("0xH1234", Address, MemSize::EightBit, 0x1234U);
        AssertParseCompVariable("0x 1234", Address, MemSize::SixteenBit, 0x1234U);
        AssertParseCompVariable("0x1234", Address, MemSize::SixteenBit, 0x1234U);
        AssertParseCompVariable("0xX1234", Address, MemSize::ThirtyTwoBit, 0x1234U);
        AssertParseCompVariable("0xL1234", Address, MemSize::Nibble_Lower, 0x1234U);
        AssertParseCompVariable("0xU1234", Address, MemSize::Nibble_Upper, 0x1234U);
        AssertParseCompVariable("0xM1234", Address, MemSize::Bit_0, 0x1234U);
        AssertParseCompVariable("0xN1234", Address, MemSize::Bit_1, 0x1234U);
        AssertParseCompVariable("0xO1234", Address, MemSize::Bit_2, 0x1234U);
        AssertParseCompVariable("0xP1234", Address, MemSize::Bit_3, 0x1234U);
        AssertParseCompVariable("0xQ1234", Address, MemSize::Bit_4, 0x1234U);
        AssertParseCompVariable("0xR1234", Address, MemSize::Bit_5, 0x1234U);
        AssertParseCompVariable("0xS1234", Address, MemSize::Bit_6, 0x1234U);
        AssertParseCompVariable("0xT1234", Address, MemSize::Bit_7, 0x1234U);

        // sizes (ignore case)
        AssertParseCompVariable("0Xh1234", Address, MemSize::EightBit, 0x1234U);
        AssertParseCompVariable("0xx1234", Address, MemSize::ThirtyTwoBit, 0x1234U);
        AssertParseCompVariable("0xl1234", Address, MemSize::Nibble_Lower, 0x1234U);
        AssertParseCompVariable("0xu1234", Address, MemSize::Nibble_Upper, 0x1234U);
        AssertParseCompVariable("0xm1234", Address, MemSize::Bit_0, 0x1234U);
        AssertParseCompVariable("0xn1234", Address, MemSize::Bit_1, 0x1234U);
        AssertParseCompVariable("0xo1234", Address, MemSize::Bit_2, 0x1234U);
        AssertParseCompVariable("0xp1234", Address, MemSize::Bit_3, 0x1234U);
        AssertParseCompVariable("0xq1234", Address, MemSize::Bit_4, 0x1234U);
        AssertParseCompVariable("0xr1234", Address, MemSize::Bit_5, 0x1234U);
        AssertParseCompVariable("0xs1234", Address, MemSize::Bit_6, 0x1234U);
        AssertParseCompVariable("0xt1234", Address, MemSize::Bit_7, 0x1234U);

        // addresses
        AssertParseCompVariable("0xH0000", Address, MemSize::EightBit, 0x0000U);
        AssertParseCompVariable("0xH12345678", Address, MemSize::EightBit, 0x12345678U);
        AssertParseCompVariable("0xHABCD", Address, MemSize::EightBit, 0xABCDU);
        AssertParseCompVariable("0xhabcd", Address, MemSize::EightBit, 0xABCDU);
    }

    TEST_METHOD(TestParseVariableDeltaMem)
    {
        // sizes
        AssertParseCompVariable("d0xH1234", DeltaMem, MemSize::EightBit, 0x1234U);
        AssertParseCompVariable("d0x 1234", DeltaMem, MemSize::SixteenBit, 0x1234U);
        AssertParseCompVariable("d0x1234", DeltaMem, MemSize::SixteenBit, 0x1234U);
        AssertParseCompVariable("d0xX1234", DeltaMem, MemSize::ThirtyTwoBit, 0x1234U);
        AssertParseCompVariable("d0xL1234", DeltaMem, MemSize::Nibble_Lower, 0x1234U);
        AssertParseCompVariable("d0xU1234", DeltaMem, MemSize::Nibble_Upper, 0x1234U);
        AssertParseCompVariable("d0xM1234", DeltaMem, MemSize::Bit_0, 0x1234U);
        AssertParseCompVariable("d0xN1234", DeltaMem, MemSize::Bit_1, 0x1234U);
        AssertParseCompVariable("d0xO1234", DeltaMem, MemSize::Bit_2, 0x1234U);
        AssertParseCompVariable("d0xP1234", DeltaMem, MemSize::Bit_3, 0x1234U);
        AssertParseCompVariable("d0xQ1234", DeltaMem, MemSize::Bit_4, 0x1234U);
        AssertParseCompVariable("d0xR1234", DeltaMem, MemSize::Bit_5, 0x1234U);
        AssertParseCompVariable("d0xS1234", DeltaMem, MemSize::Bit_6, 0x1234U);
        AssertParseCompVariable("d0xT1234", DeltaMem, MemSize::Bit_7, 0x1234U);

        // ignores case
        AssertParseCompVariable("D0Xh1234", DeltaMem, MemSize::EightBit, 0x1234U);

        // addresses
        AssertParseCompVariable("d0xH0000", DeltaMem, MemSize::EightBit, 0x0000U);
        AssertParseCompVariable("d0xH12345678", DeltaMem, MemSize::EightBit, 0x12345678U);
        AssertParseCompVariable("d0xHABCD", DeltaMem, MemSize::EightBit, 0xABCDU);
        AssertParseCompVariable("d0xhabcd", DeltaMem, MemSize::EightBit, 0xABCDU);
    }

    TEST_METHOD(TestParseVariableValue)
    {
        // decimal - values don't actually have size, default is EightBit
        AssertParseCompVariable("123", ValueComparison, MemSize::EightBit, 123U);
        AssertParseCompVariable("123456", ValueComparison, MemSize::EightBit, 123456U);
        AssertParseCompVariable("0", ValueComparison, MemSize::EightBit, 0U);
        AssertParseCompVariable("0000000000", ValueComparison, MemSize::EightBit, 0U);
        AssertParseCompVariable("4294967295", ValueComparison, MemSize::EightBit, 4294967295U);

        // hex - 'H' prefix, not '0x'!
        AssertParseCompVariable("H123", ValueComparison, MemSize::EightBit, 0x123U);
        AssertParseCompVariable("HABCD", ValueComparison, MemSize::EightBit, 0xABCDU);
        AssertParseCompVariable("h123", ValueComparison, MemSize::EightBit, 0x123U);
        AssertParseCompVariable("habcd", ValueComparison, MemSize::EightBit, 0xABCDU);
        AssertParseCompVariable("HFFFFFFFF", ValueComparison, MemSize::EightBit, 4294967295U);

        // '0x' is an address
        AssertParseCompVariable("0x123", Address, MemSize::SixteenBit, 0x123U);

        // hex without prefix (error)
        AssertParseErrorCompVariable("ABCD", ValueComparison, MemSize::EightBit, 0, 0);

        // more than 32-bits (error), will be constrained to 32-bits
        AssertParseErrorCompVariable("4294967296", ValueComparison, MemSize::EightBit, 4294967295U, 10);

        // negative value (error), will be "wrapped around": -1 = 0x100000000 - 1 = 0xFFFFFFFF = 4294967295
        AssertParseErrorCompVariable("-1", ValueComparison, MemSize::EightBit, 4294967295U, 2);
    }

    TEST_METHOD(TestSerializeVariable)
    {
        // sizes
        AssertSerialize("0xH1234");
        AssertSerialize("0x 1234");
        AssertSerialize("0x1234", "0x 1234");
        AssertSerialize("0xX1234");
        AssertSerialize("0xL1234");
        AssertSerialize("0xU1234");
        AssertSerialize("0xM1234");
        AssertSerialize("0xN1234");
        AssertSerialize("0xO1234");
        AssertSerialize("0xP1234");
        AssertSerialize("0xQ1234");
        AssertSerialize("0xR1234");
        AssertSerialize("0xS1234");
        AssertSerialize("0xT1234");

        // delta
        AssertSerialize("d0xH1234");

        // value
        AssertSerialize("123");
        AssertSerialize("H80", "128");
        AssertSerialize("4294967295");
    }

    TEST_METHOD(TestVariableSet)
    {
        CompVariable var;

        var.Set(MemSize::SixteenBit, DeltaMem, 0x1111);
        AssertCompVariable(var, DeltaMem, MemSize::SixteenBit, 0x1111, "d0x1111");

        var.SetSize(MemSize::EightBit);
        AssertCompVariable(var, DeltaMem, MemSize::EightBit, 0x1111, "d0xH1111");

        var.SetType(Address);
        AssertCompVariable(var, Address, MemSize::EightBit, 0x1111, "0xH1111");

        var.SetValues(0x1234, 0x1111);
        AssertCompVariable(var, Address, MemSize::EightBit, 0x1234, "0xH1234");
    }

    TEST_METHOD(TestVariableGetValue)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        CompVariable var;

        // value
        var.Set(MemSize::EightBit, ValueComparison, 0U);
        Assert::AreEqual(var.GetValue(), 0U);

        // eight-bit
        var.Set(MemSize::EightBit, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::EightBit, Address, 1U);
        Assert::AreEqual(var.GetValue(), 0x12U);

        var.Set(MemSize::EightBit, Address, 4U);
        Assert::AreEqual(var.GetValue(), 0x56U);

        var.Set(MemSize::EightBit, Address, 5U); // out of range
        Assert::AreEqual(var.GetValue(), 0U);

        // sixteen-bit
        var.Set(MemSize::SixteenBit, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0x1200U);

        var.Set(MemSize::SixteenBit, Address, 3U);
        Assert::AreEqual(var.GetValue(), 0x56ABU);

        var.Set(MemSize::SixteenBit, Address, 4U); // out of range
        Assert::AreEqual(var.GetValue(), 0x0056U);

        // thirty-two-bit
        var.Set(MemSize::ThirtyTwoBit, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0xAB341200U);

        var.Set(MemSize::ThirtyTwoBit, Address, 1U);
        Assert::AreEqual(var.GetValue(), 0x56AB3412U);

        var.Set(MemSize::ThirtyTwoBit, Address, 3U); // out of range
        Assert::AreEqual(var.GetValue(), 0x000056ABU);

        // nibbles
        var.Set(MemSize::Nibble_Upper, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Nibble_Upper, Address, 1U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Nibble_Upper, Address, 4U);
        Assert::AreEqual(var.GetValue(), 5U);

        var.Set(MemSize::Nibble_Upper, Address, 5U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Nibble_Lower, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Nibble_Lower, Address, 1U);
        Assert::AreEqual(var.GetValue(), 2U);

        var.Set(MemSize::Nibble_Lower, Address, 4U);
        Assert::AreEqual(var.GetValue(), 6U);

        var.Set(MemSize::Nibble_Lower, Address, 5U);
        Assert::AreEqual(var.GetValue(), 0U);

        // bits
        var.Set(MemSize::Bit_0, Address, 0U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Bit_0, Address, 3U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Bit_1, Address, 3U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Bit_2, Address, 3U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Bit_3, Address, 3U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Bit_4, Address, 3U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Bit_5, Address, 3U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Bit_6, Address, 3U);
        Assert::AreEqual(var.GetValue(), 0U);

        var.Set(MemSize::Bit_7, Address, 3U);
        Assert::AreEqual(var.GetValue(), 1U);

        var.Set(MemSize::Bit_0, Address, 5U);
        Assert::AreEqual(var.GetValue(), 0U);
    }

    TEST_METHOD(TestVariableGetValueDelta)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        CompVariable var;
        var.Set(MemSize::EightBit, DeltaMem, 1U);
        Assert::AreEqual(var.GetValue(), 0U);    // first call gets uninitialized value
        Assert::AreEqual(var.GetValue(), 0x12U); // second gets current value

        // DeltaMem is always one frame behind
        memory[1] = 0x13;
        Assert::AreEqual(var.GetValue(), 0x12U);

        memory[1] = 0x14;
        Assert::AreEqual(var.GetValue(), 0x13U);

        memory[1] = 0x15;
        Assert::AreEqual(var.GetValue(), 0x14U);

        memory[1] = 0x16;
        Assert::AreEqual(var.GetValue(), 0x15U);

        Assert::AreEqual(var.GetValue(), 0x16U);
        Assert::AreEqual(var.GetValue(), 0x16U);

        var.ResetDelta(); // ResetDelta copies the address into the delta value - this doesn't seem right. That's what happens when you overload the meaning of m_nVal!
        Assert::AreEqual(var.GetValue(), 0x1U);
    }
};

} // namespace tests
} // namespace data
} // namespace ra
