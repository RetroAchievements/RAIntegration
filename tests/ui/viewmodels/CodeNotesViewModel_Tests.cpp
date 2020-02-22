#include "CppUnitTest.h"

#include "ui\viewmodels\CodeNotesViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
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
        ra::data::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void PopulateNotes()
        {
            mockGameContext.SetGameId(1U);

            mockGameContext.MockCodeNote(0x0010, L"Score X000");
            mockGameContext.MockCodeNote(0x0011, L"Score 0X00");
            mockGameContext.MockCodeNote(0x0012, L"Score 00X0");
            mockGameContext.MockCodeNote(0x0013, L"Score 000X");
            mockGameContext.MockCodeNote(0x0016, L"[32-bit] Score");

            mockGameContext.MockCodeNote(0x001A, L"Gender\n0=Male\n1=Female");

            mockGameContext.MockCodeNote(0x0020, L"[16-bit] Max HP");
            mockGameContext.MockCodeNote(0x0022, L"[16-bit] Current HP");

            mockGameContext.MockCodeNote(0x0030, L"Item 1 Quantity");
            mockGameContext.MockCodeNote(0x0031, L"Item 2 Quantity");
            mockGameContext.MockCodeNote(0x0032, L"Item 3 Quantity");
            mockGameContext.MockCodeNote(0x0033, L"Item 4 Quantity");
            mockGameContext.MockCodeNote(0x0034, L"Item 5 Quantity");

            mockGameContext.MockCodeNote(0x0040, L"[10 bytes] Inventory");
        }
    };

    void AssertRow(CodeNotesViewModelHarness& notes, gsl::index nRow, ra::ByteAddress nAddress,
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

        notes.mockGameContext.MockCodeNote(0x0024, L"New Note");

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

        notes.mockGameContext.MockCodeNote(0x0024, L"New em Note");
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

        notes.mockGameContext.MockCodeNote(0x0024, L"New Note");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/15"), notes.GetResultCount());
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

        notes.mockGameContext.MockCodeNote(0x0022, L"[8-bit] Current HP");

        Assert::AreEqual({ 14U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"14/14"), notes.GetResultCount());

        AssertRow(notes, 7, 0x0022, L"0x0022", L"[8-bit] Current HP");
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

        notes.mockGameContext.MockCodeNote(0x0030, L"Item 1s");
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

        notes.mockGameContext.MockCodeNote(0x0030, L"Thing 1 Quantity");
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

        notes.mockGameContext.MockCodeNote(0x0020, L"Still hidden");
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

        notes.mockGameContext.MockCodeNote(0x0020, L"Heal them max");
        Assert::AreEqual({ 7U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"7/14"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0020, L"0x0020", L"Heal them max");
        AssertRow(notes, 2, 0x0030, L"0x0030", L"Item 1 Quantity");
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

        notes.mockGameContext.MockCodeNote(0x0022, L"");

        Assert::AreEqual({ 13U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L""), notes.GetFilterValue());
        Assert::AreEqual(std::wstring(L"13/13"), notes.GetResultCount());

        AssertRow(notes, 6, 0x0020, L"0x0020", L"[16-bit] Max HP");
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

        notes.mockGameContext.MockCodeNote(0x0031, L"");
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

        notes.mockGameContext.MockCodeNote(0x0022, L"");
        Assert::AreEqual({ 6U }, notes.Notes().Count());
        Assert::AreEqual(std::wstring(L"6/13"), notes.GetResultCount());
        AssertRow(notes, 1, 0x0030, L"0x0030", L"Item 1 Quantity");
    }

    TEST_METHOD(TestBookmarkSelected)
    {
        CodeNotesViewModelHarness notes;
        notes.PopulateNotes();
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
        Assert::AreEqual({ 4U }, pBookmarks.Count());
        Assert::AreEqual({ 0x0010U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual(MemSize::EightBit, pBookmarks.GetItemAt(0)->GetSize());
        Assert::AreEqual({ 0x0016U }, pBookmarks.GetItemAt(1)->GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pBookmarks.GetItemAt(1)->GetSize());
        Assert::AreEqual({ 0x0022U }, pBookmarks.GetItemAt(2)->GetAddress());
        Assert::AreEqual(MemSize::SixteenBit, pBookmarks.GetItemAt(2)->GetSize());
        Assert::AreEqual({ 0x0040U }, pBookmarks.GetItemAt(3)->GetAddress());
        Assert::AreEqual(MemSize::EightBit, pBookmarks.GetItemAt(3)->GetSize());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
