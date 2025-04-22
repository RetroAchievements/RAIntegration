#include "CppUnitTest.h"

#include "services\AchievementLogicSerializer.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

TEST_CLASS(AchievementLogicSerializer_Tests)
{
public:
    TEST_METHOD(TestBuildMemRefChain)
    {
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;

        ra::data::models::CodeNoteModel note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit] State\n"
            L"-- Increments\n"
            L"+0x438 | Obj2 pointer\n"
            L"++0x08 | Flag\n"
            L"-- b0=quest1 complete\n"
            L"-- b1=quest2 complete\n"
            L"+0x448 | [32-bit BE] Not-nested number";
        note.SetNote(sNote);
        note.SetAddress(0x1234);
        note.UpdateRawPointerValue(0x1234, mockEmulatorContext, nullptr);

        const auto* note2 = note.GetPointerNoteAtOffset(0x438);
        Assert::IsNotNull(note2);
        const auto* note3 = note2->GetPointerNoteAtOffset(0x08);
        Assert::IsNotNull(note3);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(note, *note3);
        Assert::AreEqual(std::string("I:0xX1234_I:0xX0438_M:0xH0008"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain24Bit)
    {
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockConsoleContext.SetId(ConsoleID::PlayStation); // 24-bit read

        ra::data::models::CodeNoteModel note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit] State";
        note.SetNote(sNote);
        note.SetAddress(0x1234);
        note.UpdateRawPointerValue(0x1234, mockEmulatorContext, nullptr);

        const auto* note2 = note.GetPointerNoteAtOffset(0x428);
        Assert::IsNotNull(note2);
        const auto* note3 = note2->GetPointerNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(note, *note3);
        Assert::AreEqual(std::string("I:0xW1234_I:0xW0428_M:0x 024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25Bit)
    {
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockConsoleContext.SetId(ConsoleID::PSP); // 25-bit read

        ra::data::models::CodeNoteModel note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit] State";
        note.SetNote(sNote);
        note.SetAddress(0x1234);
        note.UpdateRawPointerValue(0x1234, mockEmulatorContext, nullptr);

        const auto* note2 = note.GetPointerNoteAtOffset(0x428);
        Assert::IsNotNull(note2);
        const auto* note3 = note2->GetPointerNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(note, *note3);
        Assert::AreEqual(std::string("I:0xX1234&33554431_I:0xX0428&33554431_M:0x 024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25BitBE)
    {
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockConsoleContext.SetId(ConsoleID::GameCube); // 25-bit BE read

        ra::data::models::CodeNoteModel note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit BE] State";
        note.SetNote(sNote);
        note.SetAddress(0x1234);
        note.UpdateRawPointerValue(0x1234, mockEmulatorContext, nullptr);

        const auto* note2 = note.GetPointerNoteAtOffset(0x428);
        Assert::IsNotNull(note2);
        const auto* note3 = note2->GetPointerNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(note, *note3);
        Assert::AreEqual(std::string("I:0xG1234&33554431_I:0xG0428&33554431_M:0xI024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25BitOverflowOffset)
    {
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockConsoleContext.SetId(ConsoleID::GameCube); // 25-bit BE read

        ra::data::models::CodeNoteModel note;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x80000428 | Obj1 pointer\n" // pointer at 80123456 + offset 0x80000428 = address 0012387E 
            L"++0x8000024C | [16-bit BE] State";
        note.SetNote(sNote);
        note.SetAddress(0x1234);
        note.UpdateRawPointerValue(0x1234, mockEmulatorContext, nullptr);

        const auto* note2 = note.GetPointerNoteAtOffset(0x80000428);
        Assert::IsNotNull(note2);
        const auto* note3 = note2->GetPointerNoteAtOffset(0x8000024C);
        Assert::IsNotNull(note3);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(note, *note3);
        Assert::AreEqual(std::string("I:0xG1234_I:0xG80000428_M:0xI8000024c"), sSerialized);
    }
};

} // namespace tests
} // namespace services
} // namespace ra
