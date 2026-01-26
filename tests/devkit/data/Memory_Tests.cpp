#include "data/Memory.hh"

#include "testutil/CppUnitTest.hh"

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(Memory_Tests)
{
public:
    TEST_METHOD(TestParseAddress)
    {
        Assert::AreEqual({ 0 }, Memory::ParseAddress(""));
        Assert::AreEqual({ 0 }, Memory::ParseAddress("0"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress("0x0000"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress("$0000"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress("0x1234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress("$1234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress("0x00001234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress("1234"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("0xFEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("$FEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("FEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("0xfeed"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("$feed"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress("feed"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress("feel"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress("$0xfeed"));

        Assert::AreEqual({ 0 }, Memory::ParseAddress(L""));
        Assert::AreEqual({ 0 }, Memory::ParseAddress(L"0"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress(L"0x0000"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress(L"$0000"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress(L"0x1234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress(L"$1234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress(L"0x00001234"));
        Assert::AreEqual({ 0x1234 }, Memory::ParseAddress(L"1234"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"0xFEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"$FEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"FEED"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"0xfeed"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"$feed"));
        Assert::AreEqual({ 0xFEED }, Memory::ParseAddress(L"feed"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress(L"feel"));
        Assert::AreEqual({ 0 }, Memory::ParseAddress(L"$0xfeed"));
    }

    TEST_METHOD(TestParseRangeEmpty)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0 }, nStartAddress);
        Assert::AreEqual({ 0xFFFFFFFF }, nEndAddress);
    }

    TEST_METHOD(TestParseRangeSingleAddress)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"0x1234", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0x1234 }, nEndAddress);
    }

    TEST_METHOD(TestParseRangeHexPrefix)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"0xbeef-0xfeed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeDollarPrefix)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"$beef-$feed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoPrefix)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"1234-face", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0xFACE }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeWhitespace)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"0xbeef - 0xfeed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoStart)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(Memory::TryParseAddressRange(L"-8765", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0 }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoEnd)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(Memory::TryParseAddressRange(L"8765-", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x8765 }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNotHex)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(Memory::TryParseAddressRange(L"banana", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBA }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeStartAfterEnd)
    {
        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(Memory::TryParseAddressRange(L"0x9876-0x1234", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0x9876 }, nEndAddress);
    }
};

} // namespace tests
} // namespace data
} // namespace ra
