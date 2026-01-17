#include "CppUnitTest.h"

#include "data\models\CodeNotesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockUserContext.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\devkit\testutil\MemoryAsserts.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockServer.hh"

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
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
    };

    void TestCodeNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, Memory::Size nExpectedSize)
    {
        CodeNoteModel note;
        note.SetNote(sNote);

        Assert::AreEqual(nExpectedBytes, note.GetBytes(), sNote.c_str());
        Assert::AreEqual(nExpectedSize, note.GetMemSize(), sNote.c_str());
    }

    void TestCodeNoteFormat(const std::wstring& sNote, Memory::Format nExpectedFormat)
    {
        CodeNoteModel note;
        note.SetNote(sNote);

        Assert::AreEqual(nExpectedFormat, note.GetDefaultMemFormat(), sNote.c_str());
    }

    const CodeNoteModel* AssertIndirectNote(const CodeNoteModel& note, unsigned int nOffset,
        const std::wstring& sExpectedNote, Memory::Size nExpectedSize, unsigned int nExpectedBytes)
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
        TestCodeNoteSize(L"", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"Test", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"16-bit Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Test 16-bit", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Test 16-bi", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[16-bit] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[16 bit] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[16 Bit] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[16-bit BCD] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[24-bit] Test", 3U, Memory::Size::TwentyFourBit);
        TestCodeNoteSize(L"[32-bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestCodeNoteSize(L"[32 bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestCodeNoteSize(L"[32bit] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestCodeNoteSize(L"Test [16-bit]", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Test (16-bit)", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Test (16 bits)", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[64-bit] Test", 8U, Memory::Size::Array);
        TestCodeNoteSize(L"[128-bit] Test", 16U, Memory::Size::Array);
        TestCodeNoteSize(L"[17-bit] Test", 3U, Memory::Size::TwentyFourBit);
        TestCodeNoteSize(L"[100-bit] Test", 13U, Memory::Size::Array);
        TestCodeNoteSize(L"[0-bit] Test", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[1-bit] Test", 1U, Memory::Size::EightBit);
        TestCodeNoteSize(L"[4-bit] Test", 1U, Memory::Size::EightBit);
        TestCodeNoteSize(L"[8-bit] Test", 1U, Memory::Size::EightBit);
        TestCodeNoteSize(L"[9-bit] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"bit", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"9bit", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"-bit", 1U, Memory::Size::Unknown);

        TestCodeNoteSize(L"[16-bit BE] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestCodeNoteSize(L"[24-bit BE] Test", 3U, Memory::Size::TwentyFourBitBigEndian);
        TestCodeNoteSize(L"[32-bit BE] Test", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test [32-bit BE]", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test (32-bit BE)", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test 32-bit BE", 4U, Memory::Size::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"[16-bit BigEndian] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestCodeNoteSize(L"[16-bit-BE] Test", 2U, Memory::Size::SixteenBitBigEndian);
        TestCodeNoteSize(L"[4-bit BE] Test", 1U, Memory::Size::EightBit);
        TestCodeNoteSize(L"[US] Test [32-bit BE]", 4U, Memory::Size::ThirtyTwoBitBigEndian);

        TestCodeNoteSize(L"8 BYTE Test", 8U, Memory::Size::Array);
        TestCodeNoteSize(L"Test 8 BYTE", 8U, Memory::Size::Array);
        TestCodeNoteSize(L"Test 8 BYT", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[2 Byte] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[4 Byte] Test", 4U, Memory::Size::ThirtyTwoBit);
        TestCodeNoteSize(L"[4 Byte - Float] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"[Float - 4 Byte] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"[32-bit Float] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"[Float 32-bit] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"[8 Byte] Test", 8U, Memory::Size::Array);
        TestCodeNoteSize(L"[0x80 Bytes] Test", 128U, Memory::Size::Array);
        TestCodeNoteSize(L"[0xa8 bytes] Test", 168U, Memory::Size::Array);
        TestCodeNoteSize(L"Test [0xE Bytes]", 14U, Memory::Size::Array);
        TestCodeNoteSize(L"Test [0xET Bytes]", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"Test [0xE.3 Bytes]", 3U, Memory::Size::TwentyFourBit);
        TestCodeNoteSize(L"[2 byte] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[2-byte] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Test (6 bytes)", 6U, Memory::Size::Array);
        TestCodeNoteSize(L"[2byte] Test", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[100 Bytes] Test", 100U, Memory::Size::Array);

        TestCodeNoteSize(L"[float] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"[float32] Test", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"Test float", 4U, Memory::Size::Float);
        TestCodeNoteSize(L"Test floa", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"is floating", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"has floated", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"16-afloat", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[float be] Test", 4U, Memory::Size::FloatBigEndian);
        TestCodeNoteSize(L"[float bigendian] Test", 4U, Memory::Size::FloatBigEndian);
        TestCodeNoteSize(L"[be float] Test", 4U, Memory::Size::FloatBigEndian);
        TestCodeNoteSize(L"[bigendian float] Test", 4U, Memory::Size::FloatBigEndian);
        TestCodeNoteSize(L"[32-bit] pointer to float", 4U, Memory::Size::ThirtyTwoBit);

        TestCodeNoteSize(L"[64-bit double] Test", 8U, Memory::Size::Double32);
        TestCodeNoteSize(L"[64-bit double BE] Test", 8U, Memory::Size::Double32BigEndian);
        TestCodeNoteSize(L"[double] Test", 8U, Memory::Size::Double32);
        TestCodeNoteSize(L"[double BE] Test", 8U, Memory::Size::Double32BigEndian);
        TestCodeNoteSize(L"[double32] Test", 4U, Memory::Size::Double32);
        TestCodeNoteSize(L"[double32 BE] Test", 4U, Memory::Size::Double32BigEndian);
        TestCodeNoteSize(L"[double64] Test", 8U, Memory::Size::Double32);

        TestCodeNoteSize(L"[MBF32] Test", 4U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[MBF40] Test", 5U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[MBF32 float] Test", 4U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[MBF80] Test", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[MBF320] Test", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"[MBF-32] Test", 4U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[32-bit MBF] Test", 4U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[40-bit MBF] Test", 5U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[MBF] Test", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"Test MBF32", 4U, Memory::Size::MBF32);
        TestCodeNoteSize(L"[MBF32 LE] Test", 4U, Memory::Size::MBF32LE);
        TestCodeNoteSize(L"[MBF40-LE] Test", 5U, Memory::Size::MBF32LE);

        TestCodeNoteSize(L"42=bitten", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"42-bitten", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"bit by bit", 1U, Memory::Size::Unknown);
        TestCodeNoteSize(L"bit1=chest", 1U, Memory::Size::Unknown);

        TestCodeNoteSize(L"Bite count (16-bit)", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"Number of bits collected (32 bits)", 4U, Memory::Size::ThirtyTwoBit);

        TestCodeNoteSize(L"100 32-bit pointers [400 bytes]", 400U, Memory::Size::Array);
        TestCodeNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, Memory::Size::Array);

        TestCodeNoteSize(L"[NTSCU]\r\n[16-bit] Test\r\n", 2U, Memory::Size::SixteenBit);
        TestCodeNoteSize(L"[24-bit]\r\nIt's really 32-bit, but the top byte will never be non-zero\r\n", 3U, Memory::Size::TwentyFourBit);

        TestCodeNoteSize(L"[13-bytes ASCII] Character Name", 13U, Memory::Size::Text);
    }

    TEST_METHOD(TestExtractFormat)
    {
        TestCodeNoteFormat(L"", Memory::Format::Dec);
        TestCodeNoteFormat(L"Test", Memory::Format::Dec);
        TestCodeNoteFormat(L"16-bit Test", Memory::Format::Dec);
        TestCodeNoteFormat(L"Test 16-bit", Memory::Format::Dec);
        TestCodeNoteFormat(L"[16-bit] Test", Memory::Format::Dec);

        TestCodeNoteFormat(L"[16-bit BCD] Test", Memory::Format::Hex);
    }

    TEST_METHOD(TestExtractFormatIndirect)
    {
        CodeNoteModel note;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+03 - Bombs Defused\r\n"
            L"+04 - Bomb Timer (BCD)\r\n"
            L"+08 - [24-bit pointer] Bomb Info\r\n"
            L"++00 - [8-bit] Type\r\n"
            L"++04 - [8-bit] Color (hex)\r\n"
            L"+10 - Bombs Remaining\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat()); // pointer
        Assert::AreEqual(Memory::Format::Dec, note.GetPointerNoteAtOffset(3)->GetDefaultMemFormat()); // bombs defused
        Assert::AreEqual(Memory::Format::Hex, note.GetPointerNoteAtOffset(4)->GetDefaultMemFormat()); // bomb timer
        Assert::AreEqual(Memory::Format::Dec, note.GetPointerNoteAtOffset(10)->GetDefaultMemFormat()); // bombs remaining

        const auto* nestedNote = note.GetPointerNoteAtOffset(8);
        Expects(nestedNote != nullptr);
        Assert::AreEqual(Memory::Format::Hex, nestedNote->GetDefaultMemFormat()); // bomb info pointer
        Assert::AreEqual(Memory::Format::Dec, nestedNote->GetPointerNoteAtOffset(0)->GetDefaultMemFormat()); // type
        Assert::AreEqual(Memory::Format::Hex, nestedNote->GetPointerNoteAtOffset(4)->GetDefaultMemFormat());  // color
    }

    TEST_METHOD(TestExtractFormatImplied)
    {
        CodeNoteModel note;

        // generic decimal values; assume decimal
        const std::wstring sNote =
            L"[16-bit] location\r\n"
            L"0=Unknown\r\n"
            L"10=Level 1\r\n"
            L"68=Credits\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Format::Dec, note.GetDefaultMemFormat());

        // could be decimal values, but padded to size. assume hex
        const std::wstring sNote2 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"0068=Credits\r\n";
        note.SetNote(sNote2);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // definitely hex
        const std::wstring sNote3 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"001A=Level 1-a\r\n"
            L"0068=Credits\r\n";
        note.SetNote(sNote3);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote4 =
            L"[16-bit] location\r\n"
            L"0000: Unknown\r\n"
            L"0010: Level 1\r\n"
            L"001A: Level 1-a\r\n"
            L"0068: Credits\r\n";
        note.SetNote(sNote4);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote5 =
            L"[16-bit] location\r\n"
            L"0000 -> Unknown\r\n"
            L"0010 -> Level 1\r\n"
            L"001A -> Level 1-a\r\n"
            L"0068 -> Credits\r\n";
        note.SetNote(sNote5);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // 0x prefix
        const std::wstring sNote6 =
            L"[16-bit] location\r\n"
            L"0x00=Unknown\r\n"
            L"0x10=Level 1\r\n"
            L"0x68=Credits\r\n";
        note.SetNote(sNote6);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // h prefix
        const std::wstring sNote7 =
            L"[16-bit] location\r\n"
            L"00h=Unknown\r\n"
            L"10h=Level 1\r\n"
            L"68h=Credits\r\n";
        note.SetNote(sNote7);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());
    }

    TEST_METHOD(TestExtractFormatBits)
    {
        CodeNoteModel note;

        // b0 looks like value hex for an 8-bit value
        const std::wstring sNote =
            L"[8-bit] chests in cave\r\n"
            L"b0=first\r\n"
            L"b1=second\r\n"
            L"b2=third\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // bit0 explicitly handled for 8-bit size
        const std::wstring sNote2 =
            L"[8-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        note.SetNote(sNote2);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());

        // bit0 allowed for larger sizes
        const std::wstring sNote3 =
            L"[16-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        note.SetNote(sNote3);

        Assert::AreEqual(Memory::Format::Hex, note.GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetPointerNoteAtOffset)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+03 - Bombs Defused\r\n"
            L"+04 - Bomb Timer";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::TwentyFourBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 3U, L"Bombs Defused", Memory::Size::Unknown, 1);
        AssertIndirectNote(note, 4U, L"Bomb Timer", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestGetPointerNoteAtOffsetMultiline)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\r\n"
            L"---DEFAULT_HEAD = Barry's Head\r\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           Memory::Size::Array, 24);
    }

    TEST_METHOD(TestGetPointerNoteAtOffsetMultilineWithHeader)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[PAL]\r\n"
            L"Pointer [32bit]\r\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\r\n"
            L"---DEFAULT_HEAD = Barry's Head\r\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           Memory::Size::Array, 24);
    }

    TEST_METHOD(TestHeaderedPointer)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\r\n\r\n"
            L"Circuit:\r\n"
            L"+0x1B56E = Current Position\r\n"
            L"+0x1B57E = Total Racers\r\n\r\n"
            L"Free Run:\r\n"
            L"+0x1B5BE = Seconds 0x\r\n"
            L"+0x1B5CE = Lap";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::SixteenBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 0x1B56EU, L"Current Position", Memory::Size::Unknown, 1);
        AssertIndirectNote(note, 0x1B57EU, L"Total Racers\r\n\r\nFree Run:", Memory::Size::Unknown, 1);
        AssertIndirectNote(note, 0x1B5BEU, L"Seconds 0x", Memory::Size::Unknown, 1);
        AssertIndirectNote(note, 0x1B5CEU, L"Lap", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestPointerOverlap)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer\r\r\n"
            L"[OFFSETS]\r\r\n"
            L"+2 = EXP (32-bit)\r\r\n"
            L"+5 = Base Level (8-bit)\r\r\n" // 32-bit value at 2 continues into 5
            L"+6 = Job Level (8-bit)";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 2, L"EXP (32-bit)", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(note, 5, L"Base Level (8-bit)", Memory::Size::EightBit, 1);
        AssertIndirectNote(note, 6, L"Job Level (8-bit)", Memory::Size::EightBit, 1);
    }

    TEST_METHOD(TestNestedPointer)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\r\n"
            L"--- +0x24C | Flag\r\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\r\n"
            L"--- +0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestUnannotatedPointerChain)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | Award - Pretty Woman\r\n"
            L"++0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(
            note, 0x428, L"Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Tee Hee Two"), offsetNote->GetPointerDescription());
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438, L"Award - Pretty Woman\r\n+0x24C | Flag",
                                        Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Pretty Woman"), offsetNote->GetPointerDescription());
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestNestedPointerAlternateFormat)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"++0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestNestedPointerBracketNotSeparator)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 [32bit] Pointer - Award - Tee Hee Two\r\n"
            L"--- +0x24C [8bit] Flag\r\n"
            L"+0x438 [32bit] Pointer - Award - Pretty Woman\r\n"
            L"--- +0x24C [8bit] Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"[32bit] Pointer - Award - Tee Hee Two\r\n+0x24C [8bit] Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"[8bit] Flag", Memory::Size::EightBit, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"[32bit] Pointer - Award - Pretty Woman\r\n+0x24C [8bit] Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"[8bit] Flag", Memory::Size::EightBit, 1);
    }

    TEST_METHOD(TestNestedPointerMultiLine)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Obj1 pointer\r\n"
            L"++0x24C | [16-bit] State\r\n"
            L"-- Increments\r\n"
            L"+0x438 | Obj2 pointer\r\n"
            L"++0x08 | Flag\r\n"
            L"-- b0=quest1 complete\r\n"
            L"-- b1=quest2 complete\r\n"
            L"+0x448 | [32-bit BE] Not-nested number";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428, L"Obj1 pointer\r\n+0x24C | [16-bit] State\r\n-- Increments",
                                                    Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"[16-bit] State\r\n-- Increments", Memory::Size::SixteenBit, 2);

        offsetNote = AssertIndirectNote(note, 0x438, L"Obj2 pointer\r\n+0x08 | Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete",
                                        Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x08, L"Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete", Memory::Size::Unknown, 1);

        AssertIndirectNote(note, 0x448, L"[32-bit BE] Not-nested number", Memory::Size::ThirtyTwoBitBigEndian, 4);
    }

    TEST_METHOD(TestNestedPointerRepeatedNodes)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x308\r\n"
            L"++0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"+++0x24C | Flag\r\n"
            L"+0x308\r\n"
            L"++0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"+++0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // expect individual nodes to be merged together
        const auto* sharedNote = AssertIndirectNote(note, 0x308,
            L"+0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"++0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);

        const auto* offsetNote = AssertIndirectNote(*sharedNote, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        offsetNote = AssertIndirectNote(*sharedNote, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestImpliedPointerChain)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Root Pointer [24 bits]\r\n"
            L"\r\n"
            L"+0x8318\r\n"
            L"++0x8014\r\n"
            L"+++0x8004 = First [32-bits]\r\n"
            L"+++0x8008 = Second [32-bits]";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::TwentyFourBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* nestedNote = AssertIndirectNote(note, 0x8318,
            L"+0x8014\r\n++0x8004 = First [32-bits]\r\n++0x8008 = Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote->GetPointerDescription());
        const auto* nestedNote2 = AssertIndirectNote(*nestedNote, 0x8014,
            L"+0x8004 = First [32-bits]\r\n+0x8008 = Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote2->GetPointerDescription());
        AssertIndirectNote(*nestedNote2, 0x8004, L"First [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*nestedNote2, 0x8008, L"Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
    }

    TEST_METHOD(TestArrayPointer)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit] (80 bytes)\r\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\r\n"
            L"--- +0x24C | Flag\r\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\r\n"
            L"--- +0x24C | Flag";
        note.SetNote(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address
        Assert::AreEqual(std::wstring(L"Pointer [32bit] (80 bytes)"), note.GetPointerDescription());

        const auto* offsetNote = AssertIndirectNote(
            note, 0x428, L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438, L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag",
                                        Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestPointerOverflow)
    {
        CodeNoteModelHarness note;
        note.mockConsoleContext.AddMemoryRegion(0, 0x7F, ra::data::MemoryRegion::Type::SystemRAM, 0x80000000);
        std::array<unsigned char, 256> memory{};
        memory.at(0x04) = 0x10; // pointer@0x04 = 0x80000010
        memory.at(0x07) = 0x80;
        memory.at(0x14) = 0x20; // pointer@0x14 = 0x80000020
        memory.at(0x17) = 0x80;
        note.mockEmulatorMemoryContext.MockMemory(memory);
        const std::wstring sNote =
            L"[32-bit pointer] root\r\n"
            L"+0x80000004 = [32-bit pointer] nested\r\n"
            L"++0x80000006 = data (8-bit)";
        note.SetAddress(0x04);
        note.SetNote(sNote);
        note.UpdateRawPointerValue(0x04, note.mockEmulatorMemoryContext, nullptr);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());

        note.EnumeratePointerNotes([](ra::data::ByteAddress nAddress, const CodeNoteModel& nOffsetNote)
            {
                Assert::AreEqual(0x14U, nAddress);
                Assert::AreEqual(0x80000004U, nOffsetNote.GetAddress());
                Assert::AreEqual(std::wstring(L"[32-bit pointer] nested"), nOffsetNote.GetPointerDescription());

                return true;
            });
    }

    TEST_METHOD(TestPointerNegative)
    {
        CodeNoteModelHarness note;
        note.mockConsoleContext.AddMemoryRegion(0, 0x7F, ra::data::MemoryRegion::Type::SystemRAM, 0x80000000);
        std::array<unsigned char, 256> memory{};
        memory.at(0x04) = 0x10; // pointer@0x04 = 0x80000010
        memory.at(0x07) = 0x80;
        memory.at(0x08) = 0x20; // pointer@0x08 = 0x80000020
        memory.at(0x0B) = 0x80;
        note.mockEmulatorMemoryContext.MockMemory(memory);
        const std::wstring sNote =
            L"[32-bit pointer] root\r\n"
            L"+0xFFFFFFF8 = [32-bit pointer] nested\r\n"
            L"++0x80000006 = data (8-bit)";
        note.SetAddress(0x04);
        note.SetNote(sNote);
        note.UpdateRawPointerValue(0x04, note.mockEmulatorMemoryContext, nullptr);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, note.GetMemSize());

        note.EnumeratePointerNotes([](ra::data::ByteAddress nAddress, const CodeNoteModel& nOffsetNote)
            {
                Assert::AreEqual(0x08U, nAddress);
                Assert::AreEqual(0xFFFFFFF8U, nOffsetNote.GetAddress());
                Assert::AreEqual(std::wstring(L"[32-bit pointer] nested"), nOffsetNote.GetPointerDescription());

                return true;
            });
    }

    TEST_METHOD(TestUpdateRawPointerValue)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0 | Obj1 pointer\r\n"
            L"++0 | [16-bit] State\r\n"
            L"++2 | [16-bit] Visible\r\n"
            L"+4 | Obj2 pointer\r\n"
            L"++0 | [32-bit] ID\r\n"
            L"+8 | Count";
        note.SetAddress(4U);
        note.SetNote(sNote);

        std::array<unsigned char, 32> memory{};
        note.mockEmulatorMemoryContext.MockMemory(memory);

        memory.at(4) = 8; // pointer = 8
        memory.at(8) = 20; // obj1 pointer = 20
        memory.at(12) = 28; // obj2 pointer = 28

        note.UpdateRawPointerValue(4U, note.mockEmulatorMemoryContext, nullptr);
        Assert::AreEqual(8U, note.GetPointerAddress());

        const auto* pObj1Note = note.GetPointerNoteAtOffset(0);
        Expects(pObj1Note != nullptr);
        Assert::IsNotNull(pObj1Note);
        Assert::AreEqual(20U, pObj1Note->GetPointerAddress());

        const auto* pObj2Note = note.GetPointerNoteAtOffset(4);
        Expects(pObj2Note != nullptr);
        Assert::IsNotNull(pObj2Note);
        Assert::AreEqual(28U, pObj2Note->GetPointerAddress());
    }

    TEST_METHOD(TestGetEnumTextSimple)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0=None\r\n"
            L"1=Red\r\n"
            L"2=Green\r\n"
            L"3=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0=None"), note.GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red"), note.GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), note.GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3=Blue"), note.GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(4));
    }

    TEST_METHOD(TestGetEnumTextSingleLine)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote = L"Color (0=None, 1=Red, 2=Green, 3=Blue)";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0=None"), note.GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red"), note.GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), note.GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3=Blue"), note.GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(4));
    }

    TEST_METHOD(TestGetEnumTextSingleLineAlternateFormat)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote = L"Color [0: None, 1: Red, 2: Green, 3: Blue]";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0: None"), note.GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1: Red"), note.GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2: Green"), note.GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3: Blue"), note.GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(4));
    }

    TEST_METHOD(TestGetEnumTextHexPrefix0x)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0x00=None\r\n"
            L"0x10=Red\r\n"
            L"0x4C=Green\r\n"
            L"0xA3=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0x00=None"), note.GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"0x10=Red"), note.GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"0x4C=Green"), note.GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"0xA3=Blue"), note.GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(0x2A));
    }

    TEST_METHOD(TestGetEnumTextHexPrefixH)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"h00=None\r\n"
            L"h10=Red\r\n"
            L"h4c=Green\r\n"
            L"ha3=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"h00=None"), note.GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"h10=Red"), note.GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"h4c=Green"), note.GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"ha3=Blue"), note.GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(0x2A));
    }

    TEST_METHOD(TestGetEnumTextHexPrefixNone)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"00=None\r\n"
            L"10=Red\r\n"
            L"4C=Green\r\n"
            L"A3=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"00=None"), note.GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"10=Red"), note.GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"4C=Green"), note.GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"A3=Blue"), note.GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(0x2A));
    }

    TEST_METHOD(TestGetEnumTextRange)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0-3=None\r\n"
            L"4-7=Red\r\n"
            L"9-12=Green\r\n"
            L"15-18=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0-3=None"), note.GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), note.GetEnumText(4));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), note.GetEnumText(5));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), note.GetEnumText(6));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), note.GetEnumText(7));
        Assert::AreEqual(std::wstring_view(L"9-12=Green"), note.GetEnumText(10));
        Assert::AreEqual(std::wstring_view(L"15-18=Blue"), note.GetEnumText(17));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(8));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(20));
    }

    TEST_METHOD(TestGetEnumTextIndirect)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"[32-bit pointer] Data\r\n"
            L"0=NULL\r\n"
            L"+0x02|Color\r\n"
            L"1=Red\r\n"
            L"2=Green\r\n"
            L"+0x04|Alternate Color\r\n"
            L"3=Blue\r\n";
        note.SetNote(sNote);

        Assert::AreEqual(std::wstring_view(L"0=NULL"), note.GetEnumText(0));
        Assert::AreEqual(std::wstring_view(), note.GetEnumText(2));

        const auto* pSubNote = note.GetPointerNoteAtOffset(0x02);
        Assert::IsNotNull(pSubNote);
        Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red"), pSubNote->GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), pSubNote->GetEnumText(2));
        Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(3));
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
