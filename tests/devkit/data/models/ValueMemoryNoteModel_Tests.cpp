#include "data/models/ValueMemoryNoteModel.hh"

#include "tests/devkit/context/mocks/MockConsoleContext.hh"
#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"

#include "testutil/MemoryAsserts.hh"

#include "util/Strings.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(ValueMemoryNoteModel_Tests)
{
private:
    static std::unique_ptr<ValueMemoryNoteModel> Parse(const std::wstring& sNote)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);
        auto* pValueNoteRaw = dynamic_cast<ValueMemoryNoteModel*>(pNote.get());
        if (!pValueNoteRaw)
            return {};

        pNote.release();
        return std::unique_ptr<ValueMemoryNoteModel>(pValueNoteRaw);
    }

public:
    TEST_METHOD(TestDeterminePreferredMemFormat)
    {
        std::unique_ptr<MemoryNoteModel> pNote;

        // generic decimal values; assume decimal
        const std::wstring sNote =
            L"[16-bit] location\r\n"
            L"0=Unknown\r\n"
            L"10=Level 1\r\n"
            L"68=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote);

        Assert::AreEqual(Memory::Format::Dec, pNote->GetDefaultMemFormat());

        // could be decimal values, but padded to size. assume hex
        const std::wstring sNote2 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"0068=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote2);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // definitely hex
        const std::wstring sNote3 =
            L"[16-bit] location\r\n"
            L"0000=Unknown\r\n"
            L"0010=Level 1\r\n"
            L"001A=Level 1-a\r\n"
            L"0068=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote3);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote4 =
            L"[16-bit] location\r\n"
            L"0000: Unknown\r\n"
            L"0010: Level 1\r\n"
            L"001A: Level 1-a\r\n"
            L"0068: Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote4);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // alternate separator
        const std::wstring sNote5 =
            L"[16-bit] location\r\n"
            L"0000 -> Unknown\r\n"
            L"0010 -> Level 1\r\n"
            L"001A -> Level 1-a\r\n"
            L"0068 -> Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote5);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // 0x prefix
        const std::wstring sNote6 =
            L"[16-bit] location\r\n"
            L"0x00=Unknown\r\n"
            L"0x10=Level 1\r\n"
            L"0x68=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote6);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // h prefix
        const std::wstring sNote7 =
            L"[16-bit] location\r\n"
            L"h00=Unknown\r\n"
            L"h10=Level 1\r\n"
            L"h68=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote7);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());
        // h suffix
        const std::wstring sNote8 =
            L"[16-bit] location\r\n"
            L"00h=Unknown\r\n"
            L"10h=Level 1\r\n"
            L"68h=Credits\r\n";
        pNote = MemoryNoteModel::Parse(sNote8);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestDeterminePreferredMemFormatBits)
    {
        std::unique_ptr<MemoryNoteModel> pNote;

        // b0 looks like value hex for an 8-bit value
        const std::wstring sNote =
            L"[8-bit] chests in cave\r\n"
            L"b0=first\r\n"
            L"b1=second\r\n"
            L"b2=third\r\n";
        pNote = MemoryNoteModel::Parse(sNote);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // bit0 explicitly handled for 8-bit size
        const std::wstring sNote2 =
            L"[8-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        pNote = MemoryNoteModel::Parse(sNote2);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());

        // bit0 allowed for larger sizes
        const std::wstring sNote3 =
            L"[16-bit] chests in cave\r\n"
            L"bit0 = first\r\n"
            L"bit1 = second\r\n"
            L"bit2 = third\r\n";
        pNote = MemoryNoteModel::Parse(sNote3);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextSimple)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0=None\r\n"
            L"1=Red\r\n"
            L"2=Green\r\n"
            L"3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0=None"), pNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red"), pNote->GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), pNote->GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3=Blue"), pNote->GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(4));
        Assert::AreEqual(ra::data::Memory::Format::Dec, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextWithCommas)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0=None\r\n"
            L"1=Red, White, or Blue\r\n"
            L"2=Green\r\n"
            L"3=Purple, then Orange\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0=None"), pNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red, White, or Blue"), pNote->GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), pNote->GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3=Purple, then Orange"), pNote->GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(4));
        Assert::AreEqual(ra::data::Memory::Format::Dec, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextSingleLine)
    {
        const std::wstring sNote = L"Color (0=None, 1=Red, 2=Green, 3=Blue)";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0=None"), pNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1=Red"), pNote->GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2=Green"), pNote->GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3=Blue"), pNote->GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(4));
        Assert::AreEqual(ra::data::Memory::Format::Dec, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextSingleLineAlternateFormat)
    {
        const std::wstring sNote = L"Color [0: None, 1: Red, 2: Green, 3: Blue]";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0: None"), pNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"1: Red"), pNote->GetEnumText(1));
        Assert::AreEqual(std::wstring_view(L"2: Green"), pNote->GetEnumText(2));
        Assert::AreEqual(std::wstring_view(L"3: Blue"), pNote->GetEnumText(3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(4));
        Assert::AreEqual(ra::data::Memory::Format::Dec, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextHexPrefix0x)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0x00=None\r\n"
            L"0x10=Red\r\n"
            L"0x4C=Green\r\n"
            L"0xA3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0x00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"0x10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"0x4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"0xA3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextHexPrefixH)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"h00=None\r\n"
            L"h10=Red\r\n"
            L"h4c=Green\r\n"
            L"ha3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"h00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"h10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"h4c=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"ha3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextHexPrefixNone)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"00=None\r\n"
            L"10=Red\r\n"
            L"4C=Green\r\n"
            L"A3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"A3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextIndentSpace)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"  0x00=None\r\n"
            L"  0x10=Red\r\n"
            L"  0x4C=Green\r\n"
            L"  0xA3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0x00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"0x10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"0x4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"0xA3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextIndentNonAlphanumeric)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"..0x00=None\r\n"
            L"..0x10=Red\r\n"
            L"..0x4C=Green\r\n"
            L"..0xA3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0x00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"0x10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"0x4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"0xA3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextIndentBullet)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"* 0x00=None\r\n"
            L"* 0x10=Red\r\n"
            L"* 0x4C=Green\r\n"
            L"* 0xA3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0x00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"0x10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"0x4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"0xA3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(9)); // 8-b could be a range
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    TEST_METHOD(TestGetEnumTextIndentBulletNotPrefixed)
    {
        const std::wstring sNote =
            L"[8-bit] Color:\r\n"
            L"* 00=None\r\n"
            L"* 10=Red\r\n"
            L"* 4C=Green\r\n"
            L"* A3=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color:"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"00=None"), pNote->GetEnumText(0x00));
        Assert::AreEqual(std::wstring_view(L"10=Red"), pNote->GetEnumText(0x10));
        Assert::AreEqual(std::wstring_view(L"4C=Green"), pNote->GetEnumText(0x4C));
        Assert::AreEqual(std::wstring_view(L"A3=Blue"), pNote->GetEnumText(0xA3));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(0x2A));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(9)); // 8-b could be a range
        Assert::AreEqual(ra::data::Memory::Format::Hex, pNote->GetDefaultMemFormat());
    }

    //TEST_METHOD(TestGetEnumTextIndentBulletIndirect)
    //{
    //    const std::wstring sNote =
    //        L"[16-bit pointer] root pointer\r\n"
    //        L"+0x06: [16-bit pointer] next\r\n"
    //        L"+0x0A: [16-bit] Color: \r\n"
    //        L"* 0000=None\r\n"
    //        L"* 3010=Red\r\n"
    //        L"* A04C=Green\r\n"
    //        L"* 08A3=Blue\r\n";
    //    auto pNote = Parse(sNote);

    //    const auto* pSubNote = pNote->GetPointerNoteAtOffset(0x0A);
    //    Assert::IsNotNull(pSubNote);

    //    Assert::AreEqual(std::wstring(L"Color:"), pSubNote->GetSummary());
    //    Assert::AreEqual(std::wstring_view(L"0000=None"), pSubNote->GetEnumText(0x0000));
    //    Assert::AreEqual(std::wstring_view(L"3010=Red"), pSubNote->GetEnumText(0x3010));
    //    Assert::AreEqual(std::wstring_view(L"A04C=Green"), pSubNote->GetEnumText(0xA04C));
    //    Assert::AreEqual(std::wstring_view(L"08A3=Blue"), pSubNote->GetEnumText(0x08A3));
    //    Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(0x2A));
    //    Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(9)); // 8-b could be a range
    //    Assert::AreEqual(ra::data::Memory::Format::Hex, pSubNote->GetDefaultMemFormat());
    //}

    TEST_METHOD(TestGetEnumTextRange)
    {
        const std::wstring sNote =
            L"[8-bit] Color\r\n"
            L"0-3=None\r\n"
            L"4-7=Red\r\n"
            L"9-12=Green\r\n"
            L"15-18=Blue\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Color"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"0-3=None"), pNote->GetEnumText(0));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), pNote->GetEnumText(4));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), pNote->GetEnumText(5));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), pNote->GetEnumText(6));
        Assert::AreEqual(std::wstring_view(L"4-7=Red"), pNote->GetEnumText(7));
        Assert::AreEqual(std::wstring_view(L"9-12=Green"), pNote->GetEnumText(10));
        Assert::AreEqual(std::wstring_view(L"15-18=Blue"), pNote->GetEnumText(17));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(8));
        Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(20));
        Assert::AreEqual(ra::data::Memory::Format::Dec, pNote->GetDefaultMemFormat());
    }

    //TEST_METHOD(TestGetEnumTextIndirect)
    //{
    //    const std::wstring sNote =
    //        L"[32-bit pointer] Data\r\n"
    //        L"0=NULL\r\n"
    //        L"+0x02|Color\r\n"
    //        L"1=Red\r\n"
    //        L"2=Green\r\n"
    //        L"+0x04|Alternate Color\r\n"
    //        L"3=Blue\r\n";
    //    auto pNote = Parse(sNote);

    //    Assert::AreEqual(std::wstring(L"Data"), pNote->GetSummary());
    //    Assert::AreEqual(std::wstring_view(L"0=NULL"), pNote->GetEnumText(0));
    //    Assert::AreEqual(std::wstring_view(), pNote->GetEnumText(2));

    //    const auto* pSubNote = pNote->GetPointerNoteAtOffset(0x02);
    //    Assert::IsNotNull(pSubNote);
    //    Ensures(pSubNote != nullptr);
    //    Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(0));
    //    Assert::AreEqual(std::wstring_view(L"1=Red"), pSubNote->GetEnumText(1));
    //    Assert::AreEqual(std::wstring_view(L"2=Green"), pSubNote->GetEnumText(2));
    //    Assert::AreEqual(std::wstring_view(), pSubNote->GetEnumText(3));
    //    Assert::AreEqual(ra::data::Memory::Format::Dec, pSubNote->GetDefaultMemFormat());
    //}

    TEST_METHOD(TestGetSubNote)
    {
        const std::wstring sNote =
            L"Item flags\r\n"
            L"b0: found\r\n"
            L"bit1 = collected\r\n"
            L"B2-3=color\r\n"
            L"b4 - b7 -> count\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Item flags"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"b0: found"), pNote->GetSubNote(ra::data::Memory::Size::Bit0));
        Assert::AreEqual(std::wstring_view(L"bit1 = collected"), pNote->GetSubNote(ra::data::Memory::Size::Bit1));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit2));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit3));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit4));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit5));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit6));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit7));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::NibbleUpper));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleLower));
    }

    TEST_METHOD(TestGetSubNoteInlineSubClause)
    {
        const std::wstring sNote = L"Item flags [b0: found, bit1 = collected, B2-3=color, b4 - b7 -> count]\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Item flags"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"b0: found"), pNote->GetSubNote(ra::data::Memory::Size::Bit0));
        Assert::AreEqual(std::wstring_view(L"bit1 = collected"), pNote->GetSubNote(ra::data::Memory::Size::Bit1));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit2));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit3));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit4));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit5));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit6));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit7));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::NibbleUpper));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleLower));
    }

    TEST_METHOD(TestGetSubNoteTrailingClause)
    {
        const std::wstring sNote = L"Item flags - b0: found, bit1 = collected, B2-3=color, b4 - b7 -> count\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Item flags"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"b0: found"), pNote->GetSubNote(ra::data::Memory::Size::Bit0));
        Assert::AreEqual(std::wstring_view(L"bit1 = collected"), pNote->GetSubNote(ra::data::Memory::Size::Bit1));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit2));
        Assert::AreEqual(std::wstring_view(L"B2-3=color"), pNote->GetSubNote(ra::data::Memory::Size::Bit3));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit4));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit5));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit6));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::Bit7));
        Assert::AreEqual(std::wstring_view(L"b4 - b7 -> count"), pNote->GetSubNote(ra::data::Memory::Size::NibbleUpper));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleLower));
    }

    TEST_METHOD(TestGetSubNoteWithSetSuffix)
    {
        const std::wstring sNote =
            L"US/EU discriminator\r\n"
            L"bit4 set: US\r\n"
            L"bit5 set: EU\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"US/EU discriminator"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit0));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit1));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit2));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit3));
        Assert::AreEqual(std::wstring_view(L"bit4 set: US"), pNote->GetSubNote(ra::data::Memory::Size::Bit4));
        Assert::AreEqual(std::wstring_view(L"bit5 set: EU"), pNote->GetSubNote(ra::data::Memory::Size::Bit5));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit6));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit7));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleUpper));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleLower));
    }

    TEST_METHOD(TestGetSubNoteMultiLineWithCommas)
    {
        const std::wstring sNote =
            L"[8-bit] Tower of Floo treasures\r\n"
            L"b0: Herb, B1\r\n"
            L"b1: 100g, 1F\r\n"
            L"b2: Sword, 2F\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(std::wstring(L"Tower of Floo treasures"), pNote->GetSummary());
        Assert::AreEqual(std::wstring_view(L"b0: Herb, B1"), pNote->GetSubNote(ra::data::Memory::Size::Bit0));
        Assert::AreEqual(std::wstring_view(L"b1: 100g, 1F"), pNote->GetSubNote(ra::data::Memory::Size::Bit1));
        Assert::AreEqual(std::wstring_view(L"b2: Sword, 2F"), pNote->GetSubNote(ra::data::Memory::Size::Bit2));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit3));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit4));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit5));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit6));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::Bit7));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleUpper));
        Assert::AreEqual(std::wstring_view(), pNote->GetSubNote(ra::data::Memory::Size::NibbleLower));
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
