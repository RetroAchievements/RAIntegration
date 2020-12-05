#include "CppUnitTest.h"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior>(
    const ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior& nBehavior)
{
    switch (nBehavior)
    {
        case ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::None:
            return L"None";
        case ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::Frozen:
            return L"Fozen";
        case ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange:
            return L"PauseOnChange";
        default:
            return std::to_wstring(static_cast<int>(nBehavior));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemoryBookmarksViewModel_Tests)
{
private:
    class MemoryBookmarksViewModelHarness : public MemoryBookmarksViewModel
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::mocks::MockFrameEventQueue mockFrameEventQueue;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        bool IsModified() const noexcept { return m_bModified; }
        void ResetModified() noexcept { m_bModified = false; }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());

        Assert::AreEqual({ 4U }, bookmarks.Sizes().Count());
        Assert::AreEqual((int)MemSize::EightBit, bookmarks.Sizes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L" 8-bit"), bookmarks.Sizes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemSize::SixteenBit, bookmarks.Sizes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), bookmarks.Sizes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemSize::TwentyFourBit, bookmarks.Sizes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit"), bookmarks.Sizes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBit, bookmarks.Sizes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), bookmarks.Sizes().GetItemAt(3)->GetLabel());

        Assert::AreEqual({ 2U }, bookmarks.Formats().Count());
        Assert::AreEqual((int)MemFormat::Hex, bookmarks.Formats().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Hex"), bookmarks.Formats().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemFormat::Dec, bookmarks.Formats().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Dec"), bookmarks.Formats().GetItemAt(1)->GetLabel());

        Assert::AreEqual({ 3U }, bookmarks.Behaviors().Count());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Behaviors().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), bookmarks.Behaviors().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Behaviors().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Frozen"), bookmarks.Behaviors().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Behaviors().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Pause"), bookmarks.Behaviors().GetItemAt(2)->GetLabel());
    }

    TEST_METHOD(TestLoadBookmarksOnGameChanged)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":1,\"Decimal\":true}]}");

        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
        Assert::IsTrue(bookmarks.HasBookmark(1234U));
    }

    TEST_METHOD(TestLoadBookmarksOnGameChangedNotVisible)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(false);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());

        bookmarks.SetIsVisible(true);
        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::SixteenBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
    }

    TEST_METHOD(TestLoadBookmarksFewer)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":1,\"Decimal\":true},"
                            "{\"Description\":\"desc2\",\"Address\":1235,\"Type\":3,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 2U }, bookmarks.Bookmarks().Count());

        Assert::IsTrue(bookmarks.HasBookmark(1234U));
        Assert::IsTrue(bookmarks.HasBookmark(1235U));

        bookmarks.mockGameContext.SetGameId(4U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"4",
            "{\"Bookmarks\":[{\"Description\":\"desc3\",\"Address\":5555,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"desc3"), bookmark.GetDescription());
        Assert::AreEqual(5555U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::SixteenBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        Assert::IsFalse(bookmarks.HasBookmark(1234U));
        Assert::IsFalse(bookmarks.HasBookmark(1235U));
        Assert::IsTrue(bookmarks.HasBookmark(5555U));
    }

    TEST_METHOD(TestLoadBookmarksNoFile)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());

        bookmarks.mockGameContext.SetGameId(4U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromCodeNotes)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
    }

    TEST_METHOD(TestSaveBookmarks)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);
        Assert::IsTrue(bookmarks.IsModified());

        bookmarks.mockGameContext.SetGameId(0U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::IsFalse(bookmarks.IsModified());
        const std::string& sContents = bookmarks.mockLocalStorage.GetStoredData(ra::services::StorageItemType::Bookmarks, L"3");
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"Address\":1234,\"Size\":10},{\"Address\":2345,\"Size\":11}]}"), sContents);
    }

    TEST_METHOD(TestSaveBookmarksDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);
        bookmarks.Bookmarks().GetItemAt(0)->SetDescription(L""); // explicit blank hides existing note
        bookmarks.Bookmarks().GetItemAt(1)->SetDescription(L"Custom2"); // no backing note
        Assert::IsTrue(bookmarks.IsModified());

        bookmarks.mockGameContext.SetGameId(0U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::IsFalse(bookmarks.IsModified());
        const std::string& sContents = bookmarks.mockLocalStorage.GetStoredData(ra::services::StorageItemType::Bookmarks, L"3");
        Assert::AreEqual(std::string("{\"Bookmarks\":["
            "{\"Address\":1234,\"Size\":10,\"Description\":\"\"},"
            "{\"Address\":2345,\"Size\":11,\"Description\":\"Custom2\"}]}"), sContents);
    }

    TEST_METHOD(TestSaveBookmarksDecimal)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.Bookmarks().GetItemAt(0)->SetFormat(MemFormat::Dec);
        Assert::IsTrue(bookmarks.IsModified());

        bookmarks.mockGameContext.SetGameId(0U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::IsFalse(bookmarks.IsModified());
        const std::string& sContents = bookmarks.mockLocalStorage.GetStoredData(ra::services::StorageItemType::Bookmarks, L"3");
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"Address\":1234,\"Size\":10,\"Decimal\":true}]}"), sContents);
    }

    TEST_METHOD(TestDoFrame)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":18,\"Type\":1,\"Decimal\":false}]}");

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());

        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(18U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        bookmarks.DoFrame();
        Assert::AreEqual(18U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        memory.at(18) = 12;
        bookmarks.DoFrame();
        Assert::AreEqual(12U, bookmark.GetCurrentValue());
        Assert::AreEqual(18U, bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        bookmarks.DoFrame();
        Assert::AreEqual(12U, bookmark.GetCurrentValue());
        Assert::AreEqual(18U, bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        memory.at(18) = 15;
        bookmarks.DoFrame();
        Assert::AreEqual(15U, bookmark.GetCurrentValue());
        Assert::AreEqual(12U, bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);

        memory.at(18) = 16;
        bookmarks.DoFrame();
        Assert::AreEqual(15U, bookmark.GetCurrentValue());
        Assert::AreEqual(12U, bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());

        Assert::AreEqual(15, (int)memory.at(18));
    }

    TEST_METHOD(TestAddBookmark)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestAddBookmarkForNote)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.mockGameContext.SetCodeNote(2345U, L"NOTE");

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"NOTE"), bookmark.GetDescription());
        Assert::AreEqual(2345U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::SixteenBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestAddBookmarkPreferDecimal)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        bookmarks.AddBookmark(5678U, MemSize::ThirtyTwoBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetDescription());
        Assert::AreEqual(5678U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::ThirtyTwoBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestRemoveBookmark)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.ResetModified();

        Assert::AreEqual(1, bookmarks.RemoveSelectedBookmarks());

        Assert::AreEqual({ 2U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(1234U, bookmarks.Bookmarks().GetItemAt(0)->GetAddress());
        Assert::AreEqual(4567U, bookmarks.Bookmarks().GetItemAt(1)->GetAddress());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestRemoveBookmarkMultiple)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.AddBookmark(6789U, MemSize::EightBit);
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(3)->SetSelected(true);
        bookmarks.ResetModified();

        Assert::AreEqual(2, bookmarks.RemoveSelectedBookmarks());

        Assert::AreEqual({ 2U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(1234U, bookmarks.Bookmarks().GetItemAt(0)->GetAddress());
        Assert::AreEqual(4567U, bookmarks.Bookmarks().GetItemAt(1)->GetAddress());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestRemoveBookmarkNone)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.ResetModified();

        Assert::AreEqual(0, bookmarks.RemoveSelectedBookmarks());

        Assert::AreEqual({ 3U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(1234U, bookmarks.Bookmarks().GetItemAt(0)->GetAddress());
        Assert::AreEqual(2345U, bookmarks.Bookmarks().GetItemAt(1)->GetAddress());
        Assert::AreEqual(4567U, bookmarks.Bookmarks().GetItemAt(2)->GetAddress());

        Assert::IsFalse(bookmarks.IsModified());
    }

    TEST_METHOD(TestToggleFreezeSelected)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.AddBookmark(6789U, MemSize::EightBit);

        // no selected items are frozen - freeze them all
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(3)->SetSelected(true);

        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());

        // some selected items are frozen - freeze the rest
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(false);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(true);

        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());

        // all selected items are frozen - unfreeze them
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(false);

        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
    }

    TEST_METHOD(TestClearAllChanges)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.ResetModified();

        bookmarks.Bookmarks().GetItemAt(0)->SetChanges(6U);
        bookmarks.Bookmarks().GetItemAt(2)->SetChanges(123U);

        bookmarks.ClearAllChanges();

        Assert::AreEqual({ 3U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(0U, bookmarks.Bookmarks().GetItemAt(0)->GetChanges());
        Assert::AreEqual(0U, bookmarks.Bookmarks().GetItemAt(1)->GetChanges());
        Assert::AreEqual(0U, bookmarks.Bookmarks().GetItemAt(2)->GetChanges());

        Assert::IsFalse(bookmarks.IsModified());
    }

    TEST_METHOD(TestLoadBookmarkFile)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockFileSystem.MockFile(L"E:\\Data\\3-Bookmarks.json",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Import Bookmark File"), vmFileDialog.GetWindowTitle());
            Assert::AreEqual({ 2U }, vmFileDialog.GetFileTypes().size());
            Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
            Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

            vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

            return DialogResult::OK;
        });

        bookmarks.LoadBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
    }

    TEST_METHOD(TestLoadBookmarkFileCancel)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockFileSystem.MockFile(L"E:\\Data\\3-Bookmarks.json",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Import Bookmark File"), vmFileDialog.GetWindowTitle());
            Assert::AreEqual({ 2U }, vmFileDialog.GetFileTypes().size());
            Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
            Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

            vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

            return DialogResult::Cancel;
        });

        bookmarks.LoadBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());
    }

    TEST_METHOD(TestSaveBookmarkFile)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);
        Assert::IsTrue(bookmarks.IsModified());

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Export Bookmark File"), vmFileDialog.GetWindowTitle());
            Assert::AreEqual({ 1U }, vmFileDialog.GetFileTypes().size());
            Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
            Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

            vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

            return DialogResult::OK;
        });

        bookmarks.SaveBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::IsTrue(bookmarks.IsModified()); // export does not update modified flag
        const std::string& sContents = bookmarks.mockFileSystem.GetFileContents(L"E:\\Data\\3-Bookmarks.json");
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"Address\":1234,\"Size\":10},{\"Address\":2345,\"Size\":11}]}"), sContents);
    }

    TEST_METHOD(TestSaveBookmarkFileCancel)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);
        Assert::IsTrue(bookmarks.IsModified());

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Export Bookmark File"), vmFileDialog.GetWindowTitle());
            Assert::AreEqual({ 1U }, vmFileDialog.GetFileTypes().size());
            Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
            Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

            vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

            return DialogResult::Cancel;
        });

        bookmarks.SaveBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::IsTrue(bookmarks.IsModified()); // export does not update modified flag
        const std::string& sContents = bookmarks.mockFileSystem.GetFileContents(L"E:\\Data\\3-Bookmarks.json");
        Assert::AreEqual(std::string(), sContents);
    }

    TEST_METHOD(TestBehaviorColoring)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetRowColor().ARGB);

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmark.GetBehavior());
        Assert::AreEqual(0xFFFFFFC0U, bookmark.GetRowColor().ARGB);

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetRowColor().ARGB);

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmark.GetBehavior());
        Assert::AreEqual(0xFFFFFFC0U, bookmark.GetRowColor().ARGB);

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::None);
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetRowColor().ARGB);
    }

    TEST_METHOD(TestPauseOnChange)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::EightBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        bookmarks.AddBookmark(5U, MemSize::EightBit);
        auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        bookmark1.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);
        bookmark2.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        // no change, dialog should not be shown
        bookmarks.DoFrame();
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        // change, dialog should be shown and emulator should be paused
        memory.at(4) = 9;
        bookmarks.DoFrame();
        Assert::AreEqual({ 1U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0004"));
        bookmarks.mockFrameEventQueue.Reset();

        // only row that caused the pause should be highlighted
        Assert::AreEqual(0xFFFFC0C0U, bookmark1.GetRowColor().ARGB);
        Assert::AreEqual(0U, bookmark2.GetRowColor().ARGB);

        // unpause, dialog should not show, emulator should not pause, row should unhighlight
        bookmarks.DoFrame();
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::AreEqual(0U, bookmark1.GetRowColor().ARGB);
        Assert::AreEqual(0U, bookmark2.GetRowColor().ARGB);

        // both change in one frame, expect a single dialog
        memory.at(4) = 77;
        memory.at(5) = 77;
        bookmarks.DoFrame();
        Assert::AreEqual({ 2U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0004"));
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0005"));

        // both rows that caused the pause should be highlighted
        Assert::AreEqual(0xFFFFC0C0U, bookmark1.GetRowColor().ARGB);
        Assert::AreEqual(0xFFFFC0C0U, bookmark2.GetRowColor().ARGB);
    }

    TEST_METHOD(TestPauseOnChangeShortDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::EightBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        bookmark1.SetDescription(L"Short Description");

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        bookmark1.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        memory.at(4) = 9;
        bookmarks.DoFrame();
        Assert::AreEqual({ 1U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0004: Short Description"));
    }

    TEST_METHOD(TestPauseOnChangeLongDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::EightBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        bookmark1.SetDescription(L"This description is long enough that it will be truncated at the nearest word");

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        bookmark1.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        memory.at(4) = 9;
        bookmarks.DoFrame();
        Assert::AreEqual({ 1U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0004: This description is long enough that it"));
    }

    TEST_METHOD(TestPauseOnChangeMultilineDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::EightBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        bookmark1.SetDescription(L"Level:\n0x0F=1-1\n0x13=1-2\n0x14=1-3");

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        bookmark1.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        memory.at(4) = 9;
        bookmarks.DoFrame();
        Assert::AreEqual({ 1U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0004: Level:"));
    }

    TEST_METHOD(TestOnEditMemory)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        bookmarks.AddBookmark(1, MemSize::ThirtyTwoBit);
        bookmarks.AddBookmark(2, MemSize::TwentyFourBit);
        bookmarks.AddBookmark(3, MemSize::SixteenBit);
        bookmarks.AddBookmark(4, MemSize::EightBit);
        bookmarks.AddBookmark(4, MemSize::SixteenBit);
        bookmarks.AddBookmark(5, MemSize::EightBit);

        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(0x04030201U, bookmark1.GetCurrentValue());
        Assert::AreEqual(0U, bookmark1.GetPreviousValue());
        Assert::AreEqual(0U, bookmark1.GetChanges());

        auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);
        Assert::AreEqual(0x040302U, bookmark2.GetCurrentValue());
        Assert::AreEqual(0U, bookmark2.GetPreviousValue());
        Assert::AreEqual(0U, bookmark2.GetChanges());
        bookmark2.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen); // OnEditMemory should ignore frozen bookmarks

        auto& bookmark3 = *bookmarks.Bookmarks().GetItemAt(2);
        Assert::AreEqual(0x0403U, bookmark3.GetCurrentValue());
        Assert::AreEqual(0U, bookmark3.GetPreviousValue());
        Assert::AreEqual(0U, bookmark3.GetChanges());

        auto& bookmark4a = *bookmarks.Bookmarks().GetItemAt(3);
        Assert::AreEqual(0x04U, bookmark4a.GetCurrentValue());
        Assert::AreEqual(0U, bookmark4a.GetPreviousValue());
        Assert::AreEqual(0U, bookmark4a.GetChanges());
        bookmark4a.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange); // OnEditMemory should not trigger pause
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            Assert::IsFalse(true, L"No dialog should have been shown.");
            return DialogResult::None;
        });

        auto& bookmark4b = *bookmarks.Bookmarks().GetItemAt(4);
        Assert::AreEqual(0x0504U, bookmark4b.GetCurrentValue());
        Assert::AreEqual(0U, bookmark4b.GetPreviousValue());
        Assert::AreEqual(0U, bookmark4b.GetChanges());

        auto& bookmark5 = *bookmarks.Bookmarks().GetItemAt(5);
        Assert::AreEqual(0x05U, bookmark5.GetCurrentValue());
        Assert::AreEqual(0U, bookmark5.GetPreviousValue());
        Assert::AreEqual(0U, bookmark5.GetChanges());

        memory.at(4) = 0x40;
        bookmarks.OnEditMemory(4U);

        Assert::AreEqual(0x40030201U, bookmark1.GetCurrentValue());
        Assert::AreEqual(0x04030201U, bookmark1.GetPreviousValue());
        Assert::AreEqual(1U, bookmark1.GetChanges());

        Assert::AreEqual(0x400302U, bookmark2.GetCurrentValue());
        Assert::AreEqual(0x040302U, bookmark2.GetPreviousValue());
        Assert::AreEqual(1U, bookmark2.GetChanges());

        Assert::AreEqual(0x4003U, bookmark3.GetCurrentValue());
        Assert::AreEqual(0x0403U, bookmark3.GetPreviousValue());
        Assert::AreEqual(1U, bookmark3.GetChanges());

        Assert::AreEqual(0x40U, bookmark4a.GetCurrentValue());
        Assert::AreEqual(0x04U, bookmark4a.GetPreviousValue());
        Assert::AreEqual(1U, bookmark4a.GetChanges());

        Assert::AreEqual(0x0540U, bookmark4b.GetCurrentValue());
        Assert::AreEqual(0x0504U, bookmark4b.GetPreviousValue());
        Assert::AreEqual(1U, bookmark4b.GetChanges());

        Assert::AreEqual(0x05U, bookmark5.GetCurrentValue()); // not affected by the edited memory, should not update
        Assert::AreEqual(0U, bookmark5.GetPreviousValue());
        Assert::AreEqual(0U, bookmark5.GetChanges());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
