#include "CppUnitTest.h"

#include "data\models\CodeNotesModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(CodeNotesModel_Tests)
{
private:
    class CodeNotesModelHarness : public CodeNotesModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockUserContext mockUser;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;

        std::map<unsigned, std::wstring> mNewNotes;

        void Refresh(unsigned nGameId)
        {
            mNewNotes.clear();

            CodeNotesModel::Refresh(nGameId,
                [this](ra::ByteAddress nAddress, const std::wstring& sNewNote) {
                    mNewNotes[nAddress] = sNewNote;
                },
                []() {});

            mockThreadPool.ExecuteNextTask(); // FetchCodeNotes is async
        }

        void SetGameId(unsigned nGameId)
        {
            m_nGameId = nGameId;
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        CodeNotesModelHarness notes;

        Assert::AreEqual(AssetType::CodeNotes, notes.GetType());
        Assert::AreEqual(0U, notes.GetID());
        Assert::AreEqual(std::wstring(L"Code Notes"), notes.GetName());
        Assert::AreEqual(std::wstring(L""), notes.GetDescription());
        Assert::AreEqual(AssetCategory::Core, notes.GetCategory());
        Assert::AreEqual(AssetState::Inactive, notes.GetState());
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    
    TEST_METHOD(TestLoadCodeNotes)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, L"Note1", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 2345, L"Note2", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 3456, L"Note3", "Author" });
            return true;
        });

        notes.Refresh(1U);
        Assert::AreEqual(3U, notes.mNewNotes.size());

        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);

        const auto* pNote2 = notes.FindCodeNote(2345U);
        Assert::IsNotNull(pNote2);
        Ensures(pNote2 != nullptr);
        Assert::AreEqual(std::wstring(L"Note2"), *pNote2);
        Assert::AreEqual(std::wstring(L"Note2"), notes.mNewNotes[2345U]);

        const auto* pNote3 = notes.FindCodeNote(3456U);
        Assert::IsNotNull(pNote3);
        Ensures(pNote3 != nullptr);
        Assert::AreEqual(std::wstring(L"Note3"), *pNote3);
        Assert::AreEqual(std::wstring(L"Note3"), notes.mNewNotes[3456U]);

        const auto* pNote4 = notes.FindCodeNote(4567U);
        Assert::IsNull(pNote4);

        notes.Refresh(0U);
        const auto* pNote5 = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote5);
        Assert::AreEqual(0U, notes.mNewNotes.size());
    }

    void TestCodeNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, MemSize nExpectedSize)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([&sNote](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, sNote, "Author" });
            return true;
        });

        notes.Refresh(1U);

        Assert::AreEqual(nExpectedBytes, notes.GetCodeNoteBytes(1234U), sNote.c_str());
        Assert::AreEqual(nExpectedSize, notes.GetCodeNoteMemSize(1234U), sNote.c_str());
    }

    TEST_METHOD(TestLoadCodeNotesSized)
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

        TestCodeNoteSize(L"[MBF32] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF40] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 float] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF80] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF320] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF-32] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[32-bit MBF] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[40-bit MBF] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test MBF32", 4U, MemSize::MBF32);

        TestCodeNoteSize(L"42=bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"42-bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit by bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit1=chest", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"Bite count (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Number of bits collected (32 bits)", 4U, MemSize::ThirtyTwoBit);

        TestCodeNoteSize(L"100 32-bit pointers [400 bytes]", 400U, MemSize::Array);
        TestCodeNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, MemSize::Array);
    }

    TEST_METHOD(TestFindCodeNoteSized)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items\r\nMultiline ignored", "Author" });
            return true;
        });

        notes.Refresh(1U);

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(999, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [1/4]"), notes.FindCodeNote(1000, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [2/4]"), notes.FindCodeNote(1001, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [3/4]"), notes.FindCodeNote(1002, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [4/4]"), notes.FindCodeNote(1003, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Level"), notes.FindCodeNote(1100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [1/2]"), notes.FindCodeNote(1110, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [2/2]"), notes.FindCodeNote(1111, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [1/8]"), notes.FindCodeNote(1120, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [2/8]"), notes.FindCodeNote(1121, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [3/8]"), notes.FindCodeNote(1122, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [4/8]"), notes.FindCodeNote(1123, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [5/8]"), notes.FindCodeNote(1124, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [6/8]"), notes.FindCodeNote(1125, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [7/8]"), notes.FindCodeNote(1126, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [8/8]"), notes.FindCodeNote(1127, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1128, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [1/20]"), notes.FindCodeNote(1200, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [10/20]"), notes.FindCodeNote(1209, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [20/20]"), notes.FindCodeNote(1219, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1300, MemSize::EightBit));

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(998, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(999, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1000, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1001, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1002, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1003, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1099, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1109, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength"), notes.FindCodeNote(1110, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1111, MemSize::SixteenBit));

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(996, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(997, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location"), notes.FindCodeNote(1000, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1001, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1002, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1003, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1097, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1107, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1110, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1111, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1112, MemSize::ThirtyTwoBit));
    }

    TEST_METHOD(TestFindCodeNoteStart)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items", "Author" });
            return true;
        });

        notes.Refresh(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(100));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1001));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1002));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1003));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1004));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1099));
        Assert::AreEqual(1100U, notes.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1101));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1109));
        Assert::AreEqual(1110U, notes.FindCodeNoteStart(1110));
        Assert::AreEqual(1110U, notes.FindCodeNoteStart(1111));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1112));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1127));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1128));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1200));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1219));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1220));
    }

    TEST_METHOD(TestFindCodeNoteStartOverlap)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[100 bytes] Outer", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1010, L"[10 bytes] Inner", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1015, L"Individual", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[10 bytes] Secondary", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1125, L"[10 bytes] Overlap", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"Extra", "Author" });
            return true;
        });

        notes.Refresh(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1009));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1010));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1014));
        Assert::AreEqual(1015U, notes.FindCodeNoteStart(1015));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1016));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1019));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1020));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1099));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1124));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1125));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1126));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1130));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1134));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1135));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1200));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1201));
    }

    TEST_METHOD(TestSetCodeNote)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::UpdateCodeNote>([](const ra::api::UpdateCodeNote::Request& request, ra::api::UpdateCodeNote::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::AreEqual(1234U, request.Address);
            Assert::AreEqual(std::wstring(L"Note1"), request.Note);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.Refresh(1U);
        notes.mNewNotes.clear();

        Assert::IsTrue(notes.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);
    }

    TEST_METHOD(TestSetCodeNoteGameNotLoaded)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.ExpectUncalled<ra::api::UpdateCodeNote>();

        notes.Refresh(0U);
        Assert::IsFalse(notes.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote1);
    }

    TEST_METHOD(TestUpdateCodeNote)
    {
        int nCalls = 0;
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            return true;
        });
        notes.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&nCalls](const ra::api::UpdateCodeNote::Request&, ra::api::UpdateCodeNote::Response& response)
        {
            ++nCalls;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.Refresh(1U);
        Assert::IsTrue(notes.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);

        Assert::IsTrue(notes.SetCodeNote(1234, L"Note1b"));
        const auto* pNote1b = notes.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1b);
        Ensures(pNote1b != nullptr);
        Assert::AreEqual(std::wstring(L"Note1b"), *pNote1b);
        Assert::AreEqual(std::wstring(L"Note1b"), notes.mNewNotes[1234U]);

        Assert::AreEqual(2, nCalls);
    }

    TEST_METHOD(TestDeleteCodeNote)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::UpdateCodeNote>([](const ra::api::UpdateCodeNote::Request&, ra::api::UpdateCodeNote::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.mockServer.HandleRequest<ra::api::DeleteCodeNote>([](const ra::api::DeleteCodeNote::Request& request, ra::api::DeleteCodeNote::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::AreEqual(1234U, request.Address);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.SetGameId(1U);
        Assert::IsTrue(notes.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);

        Assert::IsTrue(notes.DeleteCodeNote(1234));
        const auto* pNote1b = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote1b);

        Assert::AreEqual(std::wstring(), notes.mNewNotes[1234U]);
    }

    TEST_METHOD(TestDeleteCodeNoteNonExistant)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.ExpectUncalled<ra::api::DeleteCodeNote>();

        notes.SetGameId(1U);

        Assert::IsTrue(notes.DeleteCodeNote(1234));
        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote1);
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
