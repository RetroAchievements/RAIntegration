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

    void TestCodeNoteFormat(const std::wstring& sNote, MemFormat nExpectedFormat)
    {
        CodeNoteModel note;
        note.SetNote(sNote);

        Assert::AreEqual(nExpectedFormat, note.GetDefaultMemFormat(), sNote.c_str());
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
        TestCodeNoteSize(L"[16-bit BCD] Test", 2U, MemSize::SixteenBit);
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

        TestCodeNoteSize(L"[NTSCU]\r\n[16-bit] Test\r\n", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[24-bit]\r\nIt's really 32-bit, but the top byte will never be non-zero\r\n", 3U, MemSize::TwentyFourBit);
    }

    TEST_METHOD(TestExtractFormat)
    {
        TestCodeNoteFormat(L"", MemFormat::Dec);
        TestCodeNoteFormat(L"Test", MemFormat::Dec);
        TestCodeNoteFormat(L"16-bit Test", MemFormat::Dec);
        TestCodeNoteFormat(L"Test 16-bit", MemFormat::Dec);
        TestCodeNoteFormat(L"[16-bit] Test", MemFormat::Dec);

        TestCodeNoteFormat(L"[16-bit BCD] Test", MemFormat::Hex);
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

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat()); // pointer
        Assert::AreEqual(MemFormat::Dec, note.GetPointerNoteAtOffset(3)->GetDefaultMemFormat()); // bombs defused
        Assert::AreEqual(MemFormat::Hex, note.GetPointerNoteAtOffset(4)->GetDefaultMemFormat()); // bomb timer
        Assert::AreEqual(MemFormat::Dec, note.GetPointerNoteAtOffset(10)->GetDefaultMemFormat()); // bombs remaining

        const auto* nestedNote = note.GetPointerNoteAtOffset(8);
        Expects(nestedNote != nullptr);
        Assert::AreEqual(MemFormat::Hex, nestedNote->GetDefaultMemFormat()); // bomb info pointer
        Assert::AreEqual(MemFormat::Dec, nestedNote->GetPointerNoteAtOffset(0)->GetDefaultMemFormat()); // type
        Assert::AreEqual(MemFormat::Hex, nestedNote->GetPointerNoteAtOffset(4)->GetDefaultMemFormat());  // color
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

        Assert::AreEqual(MemFormat::Dec, note.GetDefaultMemFormat());

        // could be decimal values, but padded to size. assume hex
        const std::wstring sNote2 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"0068=Credits\r\n";
        note.SetNote(sNote2);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // definitely hex
        const std::wstring sNote3 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"001A=Level 1-a\r\n"
            L"0068=Credits\r\n";
        note.SetNote(sNote3);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote4 =
            L"[16-bit] location\r\n"
            L"0000: Unknown\r\n"
            L"0010: Level 1\r\n"
            L"001A: Level 1-a\r\n"
            L"0068: Credits\r\n";
        note.SetNote(sNote4);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote5 =
            L"[16-bit] location\r\n"
            L"0000 -> Unknown\r\n"
            L"0010 -> Level 1\r\n"
            L"001A -> Level 1-a\r\n"
            L"0068 -> Credits\r\n";
        note.SetNote(sNote5);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // 0x prefix
        const std::wstring sNote6 =
            L"[16-bit] location\r\n"
            L"0x00=Unknown\r\n"
            L"0x10=Level 1\r\n"
            L"0x68=Credits\r\n";
        note.SetNote(sNote6);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // h prefix
        const std::wstring sNote7 =
            L"[16-bit] location\r\n"
            L"00h=Unknown\r\n"
            L"10h=Level 1\r\n"
            L"68h=Credits\r\n";
        note.SetNote(sNote7);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());
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

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // bit0 explicitly handled for 8-bit size
        const std::wstring sNote2 =
            L"[8-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        note.SetNote(sNote2);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());

        // bit0 allowed for larger sizes
        const std::wstring sNote3 =
            L"[16-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        note.SetNote(sNote3);

        Assert::AreEqual(MemFormat::Hex, note.GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetPointerNoteAtOffset)
    {
        CodeNoteModelHarness note;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+03 - Bombs Defused\r\n"
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
            L"Pointer [32bit]\r\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\r\n"
            L"---DEFAULT_HEAD = Barry's Head\r\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        note.SetNote(sNote);

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           MemSize::Array, 24);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(note, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           MemSize::Array, 24);
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

        Assert::AreEqual(MemSize::SixteenBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 0x1B56EU, L"Current Position", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B57EU, L"Total Racers\r\n\r\nFree Run:", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B5BEU, L"Seconds 0x", MemSize::Unknown, 1);
        AssertIndirectNote(note, 0x1B5CEU, L"Lap", MemSize::Unknown, 1);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(note, 2, L"EXP (32-bit)", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(note, 5, L"Base Level (8-bit)", MemSize::EightBit, 1);
        AssertIndirectNote(note, 6, L"Job Level (8-bit)", MemSize::EightBit, 1);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(
            note, 0x428, L"Award - Tee Hee Two\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Tee Hee Two"), offsetNote->GetPointerDescription());
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438, L"Award - Pretty Woman\r\n+0x24C | Flag",
                                        MemSize::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Pretty Woman"), offsetNote->GetPointerDescription());
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* offsetNote = AssertIndirectNote(note, 0x428, L"Obj1 pointer\r\n+0x24C | [16-bit] State\r\n-- Increments",
                                                    MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"[16-bit] State\r\n-- Increments", MemSize::SixteenBit, 2);

        offsetNote = AssertIndirectNote(note, 0x438, L"Obj2 pointer\r\n+0x08 | Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete",
                                        MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x08, L"Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete", MemSize::Unknown, 1);

        AssertIndirectNote(note, 0x448, L"[32-bit BE] Not-nested number", MemSize::ThirtyTwoBitBigEndian, 4);
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

        Assert::AreEqual(MemSize::TwentyFourBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address

        const auto* nestedNote = AssertIndirectNote(note, 0x8318,
            L"+0x8014\r\n++0x8004 = First [32-bits]\r\n++0x8008 = Second [32-bits]", MemSize::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote->GetPointerDescription());
        const auto* nestedNote2 = AssertIndirectNote(*nestedNote, 0x8014,
            L"+0x8004 = First [32-bits]\r\n+0x8008 = Second [32-bits]", MemSize::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote2->GetPointerDescription());
        AssertIndirectNote(*nestedNote2, 0x8004, L"First [32-bits]", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*nestedNote2, 0x8008, L"Second [32-bits]", MemSize::ThirtyTwoBit, 4);
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

        Assert::AreEqual(MemSize::ThirtyTwoBit, note.GetMemSize());
        Assert::AreEqual(sNote, note.GetNote()); // full note for pointer address
        Assert::AreEqual(std::wstring(L"Pointer [32bit] (80 bytes)"), note.GetPointerDescription());

        const auto* offsetNote = AssertIndirectNote(
            note, 0x428, L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);

        offsetNote = AssertIndirectNote(note, 0x438, L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag",
                                        MemSize::ThirtyTwoBit, 4);
        AssertIndirectNote(*offsetNote, 0x24C, L"Flag", MemSize::Unknown, 1);
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
        note.mockEmulatorContext.MockMemory(memory);

        memory.at(4) = 8; // pointer = 8
        memory.at(8) = 20; // obj1 pointer = 20
        memory.at(12) = 28; // obj2 pointer = 28

        note.UpdateRawPointerValue(4U, note.mockEmulatorContext, nullptr);
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
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
