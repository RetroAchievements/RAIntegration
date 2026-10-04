#include "data/util/AchievementLogicSerializer.hh"

#include "context/mocks/MockConsoleContext.hh"
#include "context/mocks/MockEmulatorMemoryContext.hh"

#include "data/models/PointerMemoryNoteModel.hh"

#include "testutil/CppUnitTest.hh"

namespace ra {
namespace data {
namespace util {
namespace tests {

TEST_CLASS(AchievementLogicSerializer_Tests)
{
private:
    static std::unique_ptr<ra::data::models::PointerMemoryNoteModel> Parse(const std::wstring& sNote)
    {
        auto pNote = ra::data::models::MemoryNoteModel::Parse(sNote);
        auto* pPointerNoteRaw = dynamic_cast<ra::data::models::PointerMemoryNoteModel*>(pNote.get());
        if (!pPointerNoteRaw)
            Assert::Fail(L"Parse failed.");

        pNote.release();
        return std::unique_ptr<ra::data::models::PointerMemoryNoteModel>(pPointerNoteRaw);
    }

public:
    TEST_METHOD(TestBuildMemRefChain)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;

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
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x438));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x08);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xX1234_I:0xX0438_M:0xH0008"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain24Bit)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::PlayStation); // 24-bit read

        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x428));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xW1234_I:0xW0428_M:0x 024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25Bit)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::PSP); // 25-bit read

        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x428));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xX1234&33554431_I:0xX0428&33554431_M:0x 024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25BitBE)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::GameCube); // 25-bit BE read

        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Obj1 pointer\n"
            L"++0x24C | [16-bit BE] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x428));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x24C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xG1234&33554431_I:0xG0428&33554431_M:0xI024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChain25BitOverflowOffset)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::GameCube); // 25-bit BE read

        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x80000428 | Obj1 pointer\n" // pointer at 80123456 + offset 0x80000428 = address 0012387E 
            L"++0x8000024C | [16-bit BE] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x80000428));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x8000024C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xG1234_I:0xG80000428_M:0xI8000024c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChainGBA)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::GBA); // 24-bit read with explicit offset in note

        const std::wstring sNote =
            L"Pointer [24bit]\n"
            L"+0x8428 | Obj1 pointer\n"
            L"++0x824C | [16-bit] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x8428));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x824C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xW1234_I:0xW8428_M:0x 824c"), sSerialized);
    }

    TEST_METHOD(TestBuildMemRefChainNegativeOffset)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockConsoleContext.SetId(ConsoleID::GameCube); // 29-bit BE read

        const std::wstring sNote =
            L"Pointer [24bit]\n"
            L"+0xFFFFFFF8 | Obj1 pointer\n"
            L"++0x824C | [16-bit] State";
        auto pNote = Parse(sNote);
        pNote->SetAddress(0x1234);
        pNote->UpdateBaseAddress(0x1234, mockEmulatorMemoryContext, nullptr);

        const auto* note2 = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0xFFFFFFF8));
        Assert::IsNotNull(note2);
        Ensures(note2 != nullptr);
        const auto* note3 = note2->GetNoteAtOffset(0x824C);
        Assert::IsNotNull(note3);
        Ensures(note3 != nullptr);

        std::string sSerialized = AchievementLogicSerializer::BuildMemRefChain(*pNote, *note3);
        Assert::AreEqual(std::string("I:0xG1234&33554431_I:0xGfffffff8&33554431_M:0x 824c"), sSerialized);
    }
};

} // namespace tests
} // namespace util
} // namespace data
} // namespace ra
