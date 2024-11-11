#include "CppUnitTest.h"

#include "data\models\CodeNotesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(CodeNoteModel_Tests)
{
private:
    class CodeNoteModelHarness : public CodeNoteModel
    {
    public:
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
    };

    void TestCodeNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, MemSize nExpectedSize)
    {
        CodeNoteModel note;
        note.SetNote(sNote);

        Assert::AreEqual(nExpectedBytes, note.GetBytes(), sNote.c_str());
        Assert::AreEqual(nExpectedSize, note.GetMemSize(), sNote.c_str());
    }

    const CodeNoteModel* AssertIndirectNote(const CodeNoteModel& note, unsigned int nOffset,
        const std::wstring& sExpectedNote, MemSize nExpectedSize, unsigned int nExpectedBytes)
    {
        const auto* offsetNote = note.GetPointerNoteAtOffset(nOffset);
        Assert::IsNotNull(offsetNote, ra::StringPrintf(L"No note found at offset 0x%04x", nOffset).c_str());
        Ensures(offsetNote != nullptr);

        const auto sMessage = ra::StringPrintf(L"Offset 0x%04x", nOffset);
        Assert::AreEqual(nExpectedSize, offsetNote->GetMemSize(), sMessage.c_str());
        Assert::AreEqual(nExpectedBytes, offsetNote->GetBytes(), sMessage.c_str());
        Assert::AreEqual(sExpectedNote, offsetNote->GetNote(), sMessage.c_str());

        return offsetNote;
    }

public:
    TEST_METHOD(TestExtractSize)
    {
        TestCodeNoteSize(L"", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-bit Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bi", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[16-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 Bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[24-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[32-bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32 bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"Test [16-bit]", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16 bits)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[64-bit] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[128-bit] Test", 16U, MemSize::Array);
        TestCodeNoteSize(L"[17-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[100-bit] Test", 13U, MemSize::Array);
        TestCodeNoteSize(L"[0-bit] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[1-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[4-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[8-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[9-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"9bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"-bit", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"[16-bit BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[24-bit BE] Test", 3U, MemSize::TwentyFourBitBigEndian);
        TestCodeNoteSize(L"[32-bit BE] Test", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test [32-bit BE]", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test (32-bit BE)", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test 32-bit BE", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"[16-bit BigEndian] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[16-bit-BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[4-bit BE] Test", 1U, MemSize::EightBit);

        TestCodeNoteSize(L"8 BYTE Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYTE", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYT", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[2 Byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[4 Byte] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[4 Byte - Float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[8 Byte] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[100 Bytes] Test", 100U, MemSize::Array);
        TestCodeNoteSize(L"[2 byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[2-byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (6 bytes)", 6U, MemSize::Array);
        TestCodeNoteSize(L"[2byte] Test", 2U, MemSize::SixteenBit);

        TestCodeNoteSize(L"[float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[float32] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test float", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test floa", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"is floating", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"has floated", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-afloat", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[float be] Test", 4U, MemSize::FloatBigEndian);
        TestCodeNoteSize(L"[float bigendian] Test", 4U, MemSize::FloatBigEndian);
        TestCodeNoteSize(L"[be float] Test", 4U, MemSize::FloatBigEndian);
        TestCodeNoteSize(L"[bigendian float] Test", 4U, MemSize::FloatBigEndian);
        TestCodeNoteSize(L"[32-bit] pointer to float", 4U, MemSize::ThirtyTwoBit);

        TestCodeNoteSize(L"[64-bit double] Test", 8U, MemSize::Double32);
        TestCodeNoteSize(L"[64-bit double BE] Test", 8U, MemSize::Double32BigEndian);
        TestCodeNoteSize(L"[double] Test", 8U, MemSize::Double32);
        TestCodeNoteSize(L"[double BE] Test", 8U, MemSize::Double32BigEndian);
        TestCodeNoteSize(L"[double32] Test", 4U, MemSize::Double32);
        TestCodeNoteSize(L"[double32 BE] Test", 4U, MemSize::Double32BigEndian);
        TestCodeNoteSize(L"[double64] Test", 8U, MemSize::Double32);

        TestCodeNoteSize(L"[MBF32] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF40] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 float] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF80] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF320] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF-32] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[32-bit MBF] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[40-bit MBF] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test MBF32", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 LE] Test", 4U, MemSize::MBF32LE);
        TestCodeNoteSize(L"[MBF40-LE] Test", 5U, MemSize::MBF32LE);

        TestCodeNoteSize(L"42=bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"42-bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit by bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit1=chest", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"Bite count (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Number of bits collected (32 bits)", 4U, MemSize::ThirtyTwoBit);

        TestCodeNoteSize(L"100 32-bit pointers [400 bytes]", 400U, MemSize::Array);
        TestCodeNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, MemSize::Array);
    }

    TEST_METHOD(TestGetPointerNoteAtOffset)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\n"
            L"+03 - Bombs Defused\n"
            L"+04 - Bomb Timer";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::TwentyFourBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 3U, L"Bombs Defused", MemSize::Unknown, 1);
        AssertIndirectNote(note, 4U, L"Bomb Timer", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestGetPointerNoteAtOffsetMultiline)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\n"
            L"---DEFAULT_HEAD = Barry's Head\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\n"
                           L"---DEFAULT_HEAD = Barry's Head\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           MemSize::Array, 24);
    }

    TEST_METHOD(TestCodeNoteHeadered)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\n\n"
            L"Circuit:\n"
            L"+0x1B56E = Current Position\n"
            L"+0x1B57E = Total Racers\n\n"
            L"Free Run:\n"
            L"+0x1B5BE = Seconds 0x\n"
            L"+0x1B5CE = Lap";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::SixteenBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 0x1B56EU, L"Current Position", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B57EU, L"Total Racers\n\nFree Run:", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B5BEU, L"Seconds 0x", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B5CEU, L"Lap", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestCodeNotePointerOverlap)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer\r\n"
            L"[OFFSETS]\r\n"
            L"+2 = EXP (32-bit)\r\n"
            L"+5 = Base Level (8-bit)\r\n" // 32-bit value at 2 continues into 5
            L"+6 = Job Level (8-bit)";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 2, L"EXP (32-bit)", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(note, 5, L"Base Level (8-bit)", MemSize::EightBit, 1);
        AssertIndirectNote(note, 6, L"Job Level (8-bit)", MemSize::EightBit, 1);
    }

    TEST_METHOD(TestCodeNoteNested)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\n"
            L"--- +0x24C | Flag\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\n"
            L"--- +0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"Pointer - Award - Tee Hee Two (32bit)\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"Pointer - Award - Pretty Woman (32bit)\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestCodeNoteNestedAlternateFormat)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | (32-bit pointer) Award - Tee Hee Two\n"
            L"++0x24C | Flag\n"
            L"+0x438 | (32-bit pointer) Award - Pretty Woman\n"
            L"++0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
