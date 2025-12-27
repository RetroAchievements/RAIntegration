#include "CppUnitTest.h"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\CodeNotesViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\devkit\testutil\MemoryAsserts.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(CodeNotesViewModel_Tests)
{
private:
    class CodeNotesViewModelHarness : public CodeNotesViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        ra::ui::EditorTheme editorTheme;

        CodeNotesViewModelHarness() noexcept : m_themeOverride(&editorTheme) {}

        void PopulateNotes()
        {
            mockGameContext.SetGameId(1U);

            mockGameContext.SetCodeNote(0x0010, L"Score X000");
            mockGameContext.SetCodeNote(0x0011, L"Score 0X00");
            mockGameContext.SetCodeNote(0x0012, L"Score 00X0");
            mockGameContext.SetCodeNote(0x0013, L"Score 000X");
            mockGameContext.SetCodeNote(0x0016, L"[32-bit] Score");

            mockGameContext.SetCodeNote(0x001A, L"Gender\n0=Male\n1=Female");

            mockGameContext.SetCodeNote(0x0020, L"[16-bit] Max HP");
            mockGameContext.SetCodeNote(0x0022, L"[16-bit] Current HP");

            mockGameContext.SetCodeNote(0x0030, L"Item 1 Quantity");
            mockGameContext.SetCodeNote(0x0031, L"Item 2 Quantity");
            mockGameContext.SetCodeNote(0x0032, L"Item 3 Quantity");
            mockGameContext.SetCodeNote(0x0033, L"Item 4 Quantity");
            mockGameContext.SetCodeNote(0x0034, L"Item 5 Quantity");

            mockGameContext.SetCodeNote(0x0040, L"[10 bytes] Inventory");
        }

        void PopulateMemory()
        {
            std::array<uint8_t, 100> memory = {};
            for (uint8_t i = 0; i < memory.size(); ++i)
                memory.at(i) = i;
            mockEmulatorContext.MockMemory(memory);
        }

        void PreparePublish()
        {
            mockServer.HandleRequest<ra::api::UpdateCodeNote>([this]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
            {
                m_nPublishedAddresses.push_back(pRequest.Address);

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

            mockServer.HandleRequest<ra::api::DeleteCodeNote>([this]
                (const ra::api::DeleteCodeNote::Request& pRequest, ra::api::DeleteCodeNote::Response& pResponse)
            {
                m_nPublishedAddresses.push_back(pRequest.Address);

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

            mockThreadPool.SetSynchronous(true);
        }

        const std::vector<ra::data::ByteAddress>& GetPublishedAddresses() const noexcept { return m_nPublishedAddresses; }

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> m_themeOverride;
        std::vector<ra::data::ByteAddress> m_nPublishedAddresses;
    };

    void AssertRow(CodeNotesViewModelHarness& notes, gsl::index nRow, ra::data::ByteAddress nAddress,
        const wchar_t* sAddress, const wchar_t* sNote)
    {
        auto* pRow = notes.Notes().GetItemAt(nRow);
        Assert::IsNotNull(pRow);
        Ensures(pRow != nullptr);

        Assert::AreEqual(std::wstring(sAddress), pRow->GetLabel());
        Assert::AreEqual(std::wstring(sNote), pRow->GetNote());
        Assert::AreEqual(nAddress, pRow->nAddress);
    }

public:
    TEST_METHOD(TestInitialValues)
    {
        CodeNotesViewModelHarness notes;

        Assert::AreEqual({ 0U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"0/0"), notes.GetResultCount());
    }

    TEST_METHOD(TestActivateGame)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 0, 0x0010, L"0x0010", L"Score X000");
        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        AssertRow(notes, 5, 0x001A, L"0x001a", L"Gender\n0=Male\n1=Female");
        AssertRow(notes, 6, 0x0020, L"0x0020", L"[16-bit] Max HP");
        AssertRow(notes, 13, 0x0040, L"0x0040\n- 0x0049", L"[10 bytes] Inventory");
    }

    TEST_METHOD(TestApplyAndResetFilter)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.ApplyFilter();
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 0, 0x0010, L"0x0010", L"Score X000");
        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        AssertRow(notes, 5, 0x001A, L"0x001a", L"Gender\n0=Male\n1=Female");
        AssertRow(notes, 6, 0x0020, L"0x0020", L"[16-bit] Max HP");
        AssertRow(notes, 13, 0x0040, L"0x0040\n- 0x0049", L"[10 bytes] Inventory");

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"em"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        AssertRow(notes, 0, 0x001A, L"0x001a", L"Gender\n0=Male\n1=Female");
        AssertRow(notes, 5, 0x0034, L"0x0034", L"Item 5 Quantity");

        notes.SetFilterValue(L"en");
        notes.ApplyFilter();
        Assert::AreEqual({ 1U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"en"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"1/14"), notes.GetResultCount());

        AssertRow(notes, 0, 0x001A, L"0x001a", L"Gender\n0=Male\n1=Female");

        notes.ResetFilter();
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"en"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.ApplyFilter();
        Assert::AreEqual({ 3U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"en"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"3/14"), notes.GetResultCount());

        AssertRow(notes, 0, 0x001A, L"0x001a", L"Gender\n0=Male\n1=Female");
        AssertRow(notes, 2, 0x0040, L"0x0040\n- 0x0049", L"[10 bytes] Inventory");
    }

    TEST_METHOD(TestAddNoteUnfiltered)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"New Note");

        Assert::AreEqual({ 15U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"15/15"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[16-bit] Current HP");
        AssertRow(notes, 8, 0x0024, L"0x0024", L"New Note");
        AssertRow(notes, 9, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestAddNoteFilteredMatch)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"New em Note");
        Assert::AreEqual({ 7U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"7/15"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0024, L"0x0024", L"New em Note");
    }

    TEST_METHOD(TestAddNoteFilteredNoMatch)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"New Note");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/15"), notes.GetResultCount());
    }

    TEST_METHOD(TestAddRemoveNote)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"New Note");

        Assert::AreEqual({ 15U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"15/15"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"");

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[16-bit] Current HP");
        AssertRow(notes, 8, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestAddRemoveNoteFiltered)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"em"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"New Note");

        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"em"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"6/15"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0024, L"");

        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"em"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.ResetFilter();

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"em"), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[16-bit] Current HP");
        AssertRow(notes, 8, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestUpdateNoteUnfiltered)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0022, L"[8-bit] Current HP");

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[8-bit] Current HP");
        AssertRow(notes, 8, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestUpdateNoteIndirectUnfiltered)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        const wchar_t* sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"        // no note at $0008
            L"+16 = Small (16-bit)"; // note would exist at $0010

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        // This creates an indirect note at $0008, which would bring the total
        // note count to 15. Make sure it's not added to the list.
        notes.mockGameContext.UpdateCodeNote(0x0022, sPointerNote);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        // indirect 0+8 = 8, would be first element.
        // indirect 0+16 would be $0010.
        // this one assert ensures the first item is not the indirect 8, or the note from the indirect 16
        AssertRow(notes, 0, 0x0010, L"0x0010", L"Score X000");

        // also validate the pointer note itself and that it wasn't duplicated
        AssertRow(notes, 7, 0x0022, L"0x0022", sPointerNote);
        AssertRow(notes, 8, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestUpdateNoteFilterStillApplies)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0030, L"Item 1s");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1s");
    }

    TEST_METHOD(TestUpdateNoteFilterMakesHidden)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0030, L"Thing 1 Quantity");
        Assert::AreEqual({ 5U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"5/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0031, L"0x0031", L"Item 2 Quantity");
    }

    TEST_METHOD(TestUpdateNoteFilterDoesNotApply)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0020, L"Still hidden");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestUpdateNoteFilterMakesVisible)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        notes.mockGameContext.UpdateCodeNote(0x0020, L"Heal them max");
        Assert::AreEqual({ 7U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"7/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0020, L"0x0020", L"Heal them max");
        AssertRow(notes, 2, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestUpdateNoteDialogNotVisible)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        // update before dialog is visible will be picked up when first made visible
        notes.mockGameContext.UpdateCodeNote(0x0030, L"Item 1b");
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1b");

        notes.SetIsVisible(false);

        // change should be picked up even if not visible
        notes.mockGameContext.UpdateCodeNote(0x0030, L"Item 1s");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1s");
    }

    TEST_METHOD(TestRemoveNoteUnfiltered)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        // non-committed deleted note should still be visible in list (as [Deleted])
        auto* pCodeNotes = notes.mockGameContext.Assets().FindCodeNotes();
        Assert::IsNotNull(pCodeNotes);
        Ensures(pCodeNotes != nullptr);
        pCodeNotes->SetCodeNote(0x0022, L"");

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[Deleted]");
        AssertRow(notes, 8, 0x0030, L"0x0030", L"Item 1 Quantity");

        // committed deleted note should be removed from the list
        pCodeNotes->SetServerCodeNote(0x0022, L"");

        Assert::AreEqual({ 13U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"13/13"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestRemoveNoteFilterMatch)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        // non-committed deleted note will no longer matches filter (but still gets counted)
        auto* pCodeNotes = notes.mockGameContext.Assets().FindCodeNotes();
        Assert::IsNotNull(pCodeNotes);
        Ensures(pCodeNotes != nullptr);
        pCodeNotes->SetCodeNote(0x0031, L"");

        Assert::AreEqual({ 5U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"5/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");
        AssertRow(notes, 2, 0x0032, L"0x0032", L"Item 3 Quantity");

        // committed deleted note no longer exists
        pCodeNotes->SetServerCodeNote(0x0031, L"");

        Assert::AreEqual({ 5U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"5/13"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");
        AssertRow(notes, 2, 0x0032, L"0x0032", L"Item 3 Quantity");
    }

    TEST_METHOD(TestRemoveNoteFilterUnmatched)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.SetFilterValue(L"em");
        notes.ApplyFilter();
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());

        // non-committed deleted note not matching filter still gets counted
        auto* pCodeNotes = notes.mockGameContext.Assets().FindCodeNotes();
        Assert::IsNotNull(pCodeNotes);
        Ensures(pCodeNotes != nullptr);
        pCodeNotes->SetCodeNote(0x0022, L"");

        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");

        // committed deleted note no longer exists
        pCodeNotes->SetServerCodeNote(0x0022, L"");

        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/13"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestBookmarkSelected)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.PopulateMemory();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);
        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.Notes().GetItemAt(0)->SetSelected(true);
        notes.Notes().GetItemAt(4)->SetSelected(true);
        notes.Notes().GetItemAt(7)->SetSelected(true);
        notes.Notes().GetItemAt(13)->SetSelected(true);

        notes.BookmarkSelected();

        const auto& pBookmarks = notes.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 4U }, pBookmarks.Items().Count());
        Assert::AreEqual({ 0x0010U }, pBookmarks.Items().GetItemAt(0)->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pBookmarks.Items().GetItemAt(0)->GetSize());
        Assert::AreEqual({ 0x0016U }, pBookmarks.Items().GetItemAt(1)->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBit, pBookmarks.Items().GetItemAt(1)->GetSize());
        Assert::AreEqual({ 0x0022U }, pBookmarks.Items().GetItemAt(2)->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::SixteenBit, pBookmarks.Items().GetItemAt(2)->GetSize());
        Assert::AreEqual({ 0x0040U }, pBookmarks.Items().GetItemAt(3)->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pBookmarks.Items().GetItemAt(3)->GetSize());

        notes.mockGameContext.SetCodeNote(0x0016, L"[32-bit BE] Score");
        notes.Notes().GetItemAt(0)->SetSelected(false);
        notes.Notes().GetItemAt(7)->SetSelected(false);
        notes.Notes().GetItemAt(13)->SetSelected(false);

        notes.BookmarkSelected();
        Assert::AreEqual({ 5U }, pBookmarks.Items().Count());
        Assert::AreEqual({ 0x0016U }, pBookmarks.Items().GetItemAt(4)->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::ThirtyTwoBitBigEndian, pBookmarks.Items().GetItemAt(4)->GetSize());
    }

    TEST_METHOD(TestModifiedIndicator)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        Assert::AreEqual({ 0U }, notes.Notes().Count());

        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        auto* pRow = notes.Notes().GetItemAt(4);
        Assert::IsNotNull(pRow);
        Ensures(pRow != nullptr);
        Assert::AreEqual(0xFF000000, pRow->GetBookmarkColor().ARGB);

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed");
        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed");
        Assert::AreEqual(0xFFC04040, pRow->GetBookmarkColor().ARGB);

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"[32-bit] Score");
        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        Assert::AreEqual(0xFF000000, pRow->GetBookmarkColor().ARGB);
    }
    
    TEST_METHOD(TestModifiedFilter)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0031, L"Changed 49");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0004, L"Changed 4");

        notes.SetOnlyUnpublishedFilter(true);
        notes.ApplyFilter();

        Assert::AreEqual({ 3U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"3/15"), notes.GetResultCount());

        AssertRow(notes, 0, 0x0004, L"0x0004", L"Changed 4");
        AssertRow(notes, 1, 0x0016, L"0x0016", L"Changed 20");
        AssertRow(notes, 2, 0x0031, L"0x0031", L"Changed 49");
    }

    TEST_METHOD(TestRevertSingleApprove)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0031, L"Changed 49");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert note for address 0x0016?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the notes to the last state retrieved from the server."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        notes.RevertSelected();
        Assert::IsTrue(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsFalse(notes.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestRevertSingleReject)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0031, L"Changed 49");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        notes.RevertSelected();
        Assert::IsTrue(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed 20");
        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestRevertMultiple)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0040, L"Changed 64");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert 2 notes?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the notes to the last state retrieved from the server."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        notes.RevertSelected();
        Assert::IsTrue(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"[32-bit] Score");
        AssertRow(notes, 13, 0x0040, L"0x0040\n- 0x0049", L"[10 bytes] Inventory");
        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsFalse(notes.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestPublishSingle)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.PreparePublish();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0031, L"Changed 49");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        notes.PublishSelected();
        Assert::IsFalse(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed 20");
        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsFalse(notes.CanRevertCurrentAddressNote());

        Assert::AreEqual({1}, notes.GetPublishedAddresses().size());
        Assert::AreEqual({0x0016}, notes.GetPublishedAddresses().at(0));
    }

    TEST_METHOD(TestPublishSingleOffline)
    {
        CodeNotesViewModelHarness notes;
        notes.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);
        notes.PopulateNotes();
        notes.PreparePublish();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0031, L"Changed 49");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        notes.PublishSelected();
        Assert::IsFalse(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed 20");
        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestPublishMultipleApprove)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.PreparePublish();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0040, L"Changed 64");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        int nWindowsSeen = 0;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&nWindowsSeen, &notes](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            if (nWindowsSeen == 0)
            {
                Assert::AreEqual(std::wstring(L"Publish 2 notes?"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"The selected modified notes will be uploaded to the server."), vmMessageBox.GetMessage());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

                nWindowsSeen++;
                return ra::ui::DialogResult::Yes;
            }
            else
            {
                Assert::AreEqual(std::wstring(L"Publish succeeded."), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"2 items successfully uploaded."), vmMessageBox.GetMessage());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());

                nWindowsSeen++;
                return ra::ui::DialogResult::OK;
            }
        });

        notes.PublishSelected();
        Assert::AreEqual(nWindowsSeen, 2);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed 20");
        AssertRow(notes, 13, 0x0040, L"0x0040", L"Changed 64");
        Assert::IsFalse(notes.CanPublishCurrentAddressNote());
        Assert::IsFalse(notes.CanRevertCurrentAddressNote());

        Assert::AreEqual({2}, notes.GetPublishedAddresses().size());
        Assert::AreEqual({0x0016}, notes.GetPublishedAddresses().at(0));
        Assert::AreEqual({0x0040}, notes.GetPublishedAddresses().at(1));
    }

    TEST_METHOD(TestPublishMultipleReject)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
        notes.PreparePublish();
        notes.SetIsVisible(true);

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0016, L"Changed 20");
        notes.mockGameContext.Assets().FindCodeNotes()->SetCodeNote(0x0040, L"Changed 64");

        notes.Notes().GetItemAt(0)->SetSelected(true); // 0x10
        notes.Notes().GetItemAt(4)->SetSelected(true); // 0x16
        notes.Notes().GetItemAt(7)->SetSelected(true); // 0x22
        notes.Notes().GetItemAt(13)->SetSelected(true); // 0x40

        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        bool bWindowSeen = false;
        notes.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Publish 2 notes?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The selected modified notes will be uploaded to the server."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        notes.PublishSelected();
        Assert::IsTrue(bWindowSeen);

        AssertRow(notes, 4, 0x0016, L"0x0016", L"Changed 20");
        AssertRow(notes, 13, 0x0040, L"0x0040", L"Changed 64");
        Assert::IsTrue(notes.CanPublishCurrentAddressNote());
        Assert::IsTrue(notes.CanRevertCurrentAddressNote());

        Assert::AreEqual({0}, notes.GetPublishedAddresses().size());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
