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
    void AssertCompVariable(const CompVariable& var, ComparisonVariableType nExpectedType, ComparisonVariableSize nExpectedSize, unsigned int nExpectedRawValue, const char* sSerialized)
    {
        std::wstring wsSerialized = Widen(sSerialized);
        Assert::AreEqual(nExpectedType, var.Type(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedSize, var.Size(), wsSerialized.c_str());
        Assert::AreEqual(nExpectedRawValue, var.RawValue(), wsSerialized.c_str());
    }

    void AssertSerialize(ComparisonVariableType nType, ComparisonVariableSize nSize, unsigned int nValue, const char* sExpected)
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
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::EightBit, 0x1234U, "0xH1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::SixteenBit, 0x1234U, "0x 1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::ThirtyTwoBit, 0x1234U, "0xX1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Nibble_Lower, 0x1234U, "0xL1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Nibble_Upper, 0x1234U, "0xU1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_0, 0x1234U, "0xM1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_1, 0x1234U, "0xN1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_2, 0x1234U, "0xO1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_3, 0x1234U, "0xP1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_4, 0x1234U, "0xQ1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_5, 0x1234U, "0xR1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_6, 0x1234U, "0xS1234");
        AssertSerialize(ComparisonVariableType::Address, ComparisonVariableSize::Bit_7, 0x1234U, "0xT1234");

        // delta
        AssertSerialize(ComparisonVariableType::DeltaMem, ComparisonVariableSize::EightBit, 0x1234U, "d0xH1234");

        // value (size is ignored)
        AssertSerialize(ComparisonVariableType::ValueComparison, ComparisonVariableSize::EightBit, 123, "123");
    }

    TEST_METHOD(TestVariableSet)
    {
        CompVariable var;

        var.Set(SixteenBit, DeltaMem, 0x1111);
        AssertCompVariable(var, DeltaMem, SixteenBit, 0x1111, "d0x1111");

        var.SetSize(EightBit);
        AssertCompVariable(var, DeltaMem, EightBit, 0x1111, "d0xH1111");

        var.SetType(Address);
        AssertCompVariable(var, Address, EightBit, 0x1111, "0xH1111");

        var.SetValues(0x1234, 0x1111);
        AssertCompVariable(var, Address, EightBit, 0x1234, "0xH1234");
    }
};

} // namespace tests
} // namespace data
} // namespace ra
