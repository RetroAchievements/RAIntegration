#include "data/models/MemoryNoteModel.hh"

#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"

#include "testutil/MemoryAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(MemoryNoteModel_Tests)
{
private:
    void TestNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, Memory::Size nExpectedSize)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);

        Assert::AreEqual(nExpectedBytes, pNote->GetBytes(), sNote.c_str());
        Assert::AreEqual(nExpectedSize, pNote->GetMemSize(), sNote.c_str());
    }

    void TestNoteFormat(const std::wstring& sNote, Memory::Format nExpectedFormat)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);

        Assert::AreEqual(nExpectedFormat, pNote->GetDefaultMemFormat(), sNote.c_str());
    }

public:
    TEST_METHOD(TestExtractSize)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;

        TestNoteSize(L"", 1U, Memory::Size::Unknown);
        TestNoteSize(L"Test", 1U, Memory::Size::Unknown);
        TestNoteSize(L"16-bit Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Test 16-bit", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Test 16-bi", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[16-bit] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[16 bit] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[16 Bit] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[16-bit BCD] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[24-bit] Test", 3U, Memory::Size::TwentyFourBit);
        TestNoteSize(L"[32-bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestNoteSize(L"[32 bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestNoteSize(L"[32bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestNoteSize(L"Test [16-bit]", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Test (16-bit)", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Test (16 bits)", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[64-bit] Test", 8U, Memory::Size::Array);
        TestNoteSize(L"[128-bit] Test", 16U, Memory::Size::Array);
        TestNoteSize(L"[17-bit] Test", 3U, Memory::Size::TwentyFourBit);
        TestNoteSize(L"[100-bit] Test", 13U, Memory::Size::Array);
        TestNoteSize(L"[0-bit] Test", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[1-bit] Test", 1U, Memory::Size::EightBit);
        TestNoteSize(L"[4-bit] Test", 1U, Memory::Size::EightBit);
        TestNoteSize(L"[8-bit] Test", 1U, Memory::Size::EightBit);
        TestNoteSize(L"[9-bit] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"bit", 1U, Memory::Size::Unknown);
        TestNoteSize(L"9bit", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"-bit", 1U, Memory::Size::Unknown);

        TestNoteSize(L"[16-bit BE] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestNoteSize(L"[24-bit BE] Test", 3U, Memory::Size::TwentyFourBitBigEndian);
        TestNoteSize(L"[32-bit BE] Test", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestNoteSize(L"Test [32-bit BE]", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestNoteSize(L"Test (32-bit BE)", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestNoteSize(L"Test 32-bit BE", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestNoteSize(L"[16-bit BigEndian] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestNoteSize(L"[16-bit-BE] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestNoteSize(L"[4-bit BE] Test", 1U, Memory::Size::EightBit);
        TestNoteSize(L"[US] Test [32-bit BE]", 4U, Memory::Size::ThirtyTwoBitBigEndian);

        TestNoteSize(L"8 BYTE Test", 8U, Memory::Size::Array);
        TestNoteSize(L"Test 8 BYTE", 8U, Memory::Size::Array);
        TestNoteSize(L"Test 8 BYT", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[2 Byte] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[4 Byte] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestNoteSize(L"[4 Byte - Float] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"[Float - 4 Byte] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"[32-bit Float] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"[Float 32-bit] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"[8 Byte] Test", 8U, Memory::Size::Array);
        TestNoteSize(L"[0x80 Bytes] Test", 128U, Memory::Size::Array);
        TestNoteSize(L"[0xa8 bytes] Test", 168U, Memory::Size::Array);
        TestNoteSize(L"Test [0xE Bytes]", 14U, Memory::Size::Array);
        TestNoteSize(L"Test [0xET Bytes]", 1U, Memory::Size::Unknown);
        TestNoteSize(L"Test [0xE.3 Bytes]", 3U, Memory::Size::TwentyFourBit);
        TestNoteSize(L"[2 byte] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[2-byte] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Test (6 bytes)", 6U, Memory::Size::Array);
        TestNoteSize(L"[2byte] Test", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[100 Bytes] Test", 100U, Memory::Size::Array);

        TestNoteSize(L"[float] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"[float32] Test", 4U, Memory::Size::Float);
        TestNoteSize(L"Test float", 4U, Memory::Size::Float);
        TestNoteSize(L"Test floa", 1U, Memory::Size::Unknown);
        TestNoteSize(L"is floating", 1U, Memory::Size::Unknown);
        TestNoteSize(L"has floated", 1U, Memory::Size::Unknown);
        TestNoteSize(L"16-afloat", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[float be] Test", 4U, Memory::Size::FloatBigEndian);
        TestNoteSize(L"[float bigendian] Test", 4U, Memory::Size::FloatBigEndian);
        TestNoteSize(L"[be float] Test", 4U, Memory::Size::FloatBigEndian);
        TestNoteSize(L"[bigendian float] Test", 4U, Memory::Size::FloatBigEndian);
        TestNoteSize(L"[32-bit] pointer to float", 4U, Memory::Size::ThirtyTwoBit);
        TestNoteSize(L"[8-bit] can double down", 1U, Memory::Size::EightBit);

        TestNoteSize(L"[64-bit double] Test", 8U, Memory::Size::Double32);
        TestNoteSize(L"[64-bit double BE] Test", 8U, Memory::Size::Double32BigEndian);
        TestNoteSize(L"[double] Test", 8U, Memory::Size::Double32);
        TestNoteSize(L"[double BE] Test", 8U, Memory::Size::Double32BigEndian);
        TestNoteSize(L"[double32] Test", 4U, Memory::Size::Double32);
        TestNoteSize(L"[double32 BE] Test", 4U, Memory::Size::Double32BigEndian);
        TestNoteSize(L"[double64] Test", 8U, Memory::Size::Double32);

        TestNoteSize(L"[MBF32] Test", 4U, Memory::Size::MBF32);
        TestNoteSize(L"[MBF40] Test", 5U, Memory::Size::MBF32);
        TestNoteSize(L"[MBF32 float] Test", 4U, Memory::Size::MBF32);
        TestNoteSize(L"[MBF80] Test", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[MBF320] Test", 1U, Memory::Size::Unknown);
        TestNoteSize(L"[MBF-32] Test", 4U, Memory::Size::MBF32);
        TestNoteSize(L"[32-bit MBF] Test", 4U, Memory::Size::MBF32);
        TestNoteSize(L"[40-bit MBF] Test", 5U, Memory::Size::MBF32);
        TestNoteSize(L"[MBF] Test", 1U, Memory::Size::Unknown);
        TestNoteSize(L"Test MBF32", 4U, Memory::Size::MBF32);
        TestNoteSize(L"[MBF32 LE] Test", 4U, Memory::Size::MBF32LE);
        TestNoteSize(L"[MBF40-LE] Test", 5U, Memory::Size::MBF32LE);

        TestNoteSize(L"42=bitten", 1U, Memory::Size::Unknown);
        TestNoteSize(L"42-bitten", 1U, Memory::Size::Unknown);
        TestNoteSize(L"bit by bit", 1U, Memory::Size::Unknown);
        TestNoteSize(L"bit1=chest", 1U, Memory::Size::Unknown);

        TestNoteSize(L"Bite count (16-bit)", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"Number of bits collected (32 bits)", 4U, Memory::Size::ThirtyTwoBit);

        TestNoteSize(L"100 32-bit pointers [400 bytes]", 400U, Memory::Size::Array);
        TestNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, Memory::Size::Array);

        TestNoteSize(L"[NTSCU]\r\n[16-bit] Test\r\n", 2U, Memory::Size::SixteenBit);
        TestNoteSize(L"[24-bit]\r\nIt's really 32-bit, but the top byte will never be non-zero\r\n", 3U, Memory::Size::TwentyFourBit);

        TestNoteSize(L"[13-bytes ASCII] Character Name", 13U, Memory::Size::Text);
    }

    TEST_METHOD(TestExtractFormat)
    {
        TestNoteFormat(L"", Memory::Format::Dec);
        TestNoteFormat(L"Test", Memory::Format::Dec);
        TestNoteFormat(L"16-bit Test", Memory::Format::Dec);
        TestNoteFormat(L"Test 16-bit", Memory::Format::Dec);
        TestNoteFormat(L"[16-bit] Test", Memory::Format::Dec);

        TestNoteFormat(L"[16-bit BCD] Test", Memory::Format::Hex);
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
