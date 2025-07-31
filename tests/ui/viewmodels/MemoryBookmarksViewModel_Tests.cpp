#include "CppUnitTest.h"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockUserContext.hh"

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
            return L"Frozen";
        case ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange:
            return L"PauseOnChange";
        default:
            return std::to_wstring(static_cast<int>(nBehavior));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

ra::ui::viewmodels::MemoryBookmarksViewModel* g_pMemoryBookmarksViewModel = nullptr;

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
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::mocks::MockFrameEventQueue mockFrameEventQueue;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        GSL_SUPPRESS_F6 MemoryBookmarksViewModelHarness()
        {
            InitializeNotifyTargets();
        }

        using MemoryBookmarksViewModel::IsModified;
        void ResetModified() noexcept { m_nUnmodifiedBookmarkCount = Bookmarks().Count(); }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());

        Assert::AreEqual({ 17U }, bookmarks.Sizes().Count());
        Assert::AreEqual((int)MemSize::EightBit, bookmarks.Sizes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L" 8-bit"), bookmarks.Sizes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemSize::SixteenBit, bookmarks.Sizes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), bookmarks.Sizes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemSize::TwentyFourBit, bookmarks.Sizes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit"), bookmarks.Sizes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBit, bookmarks.Sizes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), bookmarks.Sizes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)MemSize::SixteenBitBigEndian, bookmarks.Sizes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit BE"), bookmarks.Sizes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)MemSize::TwentyFourBitBigEndian, bookmarks.Sizes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit BE"), bookmarks.Sizes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBitBigEndian, bookmarks.Sizes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit BE"), bookmarks.Sizes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)MemSize::BitCount, bookmarks.Sizes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"BitCount"), bookmarks.Sizes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)MemSize::Nibble_Lower, bookmarks.Sizes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Lower4"), bookmarks.Sizes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)MemSize::Nibble_Upper, bookmarks.Sizes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"Upper4"), bookmarks.Sizes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)MemSize::Float, bookmarks.Sizes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"Float"), bookmarks.Sizes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)MemSize::FloatBigEndian, bookmarks.Sizes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"Float BE"), bookmarks.Sizes().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)MemSize::Double32, bookmarks.Sizes().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"Double32"), bookmarks.Sizes().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)MemSize::Double32BigEndian, bookmarks.Sizes().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"Double32 BE"), bookmarks.Sizes().GetItemAt(13)->GetLabel());
        Assert::AreEqual((int)MemSize::MBF32, bookmarks.Sizes().GetItemAt(14)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32"), bookmarks.Sizes().GetItemAt(14)->GetLabel());
        Assert::AreEqual((int)MemSize::MBF32LE, bookmarks.Sizes().GetItemAt(15)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32 LE"), bookmarks.Sizes().GetItemAt(15)->GetLabel());
        Assert::AreEqual((int)MemSize::Text, bookmarks.Sizes().GetItemAt(16)->GetId());
        Assert::AreEqual(std::wstring(L"ASCII"), bookmarks.Sizes().GetItemAt(16)->GetLabel());

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

    TEST_METHOD(TestLoadBookmarksOnGameChangedFormat1)
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
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
        Assert::IsTrue(bookmarks.HasBookmark(1234U));
    }

    TEST_METHOD(TestLoadBookmarksOnGameChangedFormat2)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Size\":10,\"Decimal\":true}]}");

        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
        Assert::IsTrue(bookmarks.HasBookmark(1234U));
    }

    TEST_METHOD(TestLoadBookmarksOnGameChangedFormat3)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"MemAddr\":\"0xH04d2\",\"Decimal\":true}]}");

        Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
        Assert::IsTrue(bookmarks.HasBookmark(1234U));
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
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc3"), bookmark.GetDescription());
        Assert::AreEqual(5555U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::SixteenBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0000"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark.GetPreviousValue());
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
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsFalse(bookmark.IsCustomDescription());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromMultilineCodeNotes)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"[8-bit] Selected Character\r\n1=Bob\r\n2=Jane");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
                                                  "{\"Bookmarks\":[{\"MemAddr\":\"0xH4D2\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"[8-bit] Selected Character\r\n1=Bob\r\n2=Jane"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Selected Character"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsFalse(bookmark.IsCustomDescription());

        bookmark.SetDescription(L"Primary Character");

        Assert::AreEqual(std::wstring(L"[8-bit] Selected Character\r\n1=Bob\r\n2=Jane"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Primary Character"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsTrue(bookmark.IsCustomDescription());

        bookmark.SetDescription(L"Selected Character");

        Assert::AreEqual(std::wstring(L"[8-bit] Selected Character\r\n1=Bob\r\n2=Jane"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Selected Character"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsFalse(bookmark.IsCustomDescription());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromFile)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10,\"Description\":\"desc\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsTrue(bookmark.IsCustomDescription());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromFileAndCodeNotes)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10,\"Description\":\"desc\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsTrue(bookmark.IsCustomDescription());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromFileMatchesCodeNote)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10,\"Description\":\"Note description\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsFalse(bookmark.IsCustomDescription());
    }

    TEST_METHOD(TestLoadSaveBookmarkSizes)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");

        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>([](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog)
        {
            vmFileDialog.SetFileName(L"bookmarks.json");
            return DialogResult::OK;
        });

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(bookmarks.Sizes().Count()); ++nIndex)
        {
            const auto nSize = ra::itoe<MemSize>(bookmarks.Sizes().GetItemAt(nIndex)->GetId());
            Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());

            bookmarks.AddBookmark(1234U, nSize);
            Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());

            bookmarks.SaveBookmarkFile();

            bookmarks.Bookmarks().GetItemAt(0)->SetSelected(true);
            bookmarks.RemoveSelectedBookmarks();
            Assert::AreEqual({ 0U }, bookmarks.Bookmarks().Count());

            bookmarks.LoadBookmarkFile();
            Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());

            Assert::AreEqual(nSize, bookmarks.Bookmarks().GetItemAt(0)->GetSize());

            bookmarks.Bookmarks().GetItemAt(0)->SetSelected(true);
            bookmarks.RemoveSelectedBookmarks();
        }
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
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\"},{\"MemAddr\":\"0x 0929\"}]}"), sContents);
    }

    TEST_METHOD(TestSaveBookmarksDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Description\":\"Old\",\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);
        bookmarks.Bookmarks().GetItemAt(0)->SetDescription(L"");        // explicit blank resets to default note
        bookmarks.Bookmarks().GetItemAt(1)->SetDescription(L"Custom2"); // no backing note
        Assert::IsTrue(bookmarks.IsModified());

        bookmarks.mockGameContext.SetGameId(0U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::IsFalse(bookmarks.IsModified());
        const std::string& sContents = bookmarks.mockLocalStorage.GetStoredData(ra::services::StorageItemType::Bookmarks, L"3");
        Assert::AreEqual(std::string("{\"Bookmarks\":["
            "{\"MemAddr\":\"0xH04d2\"},"
            "{\"MemAddr\":\"0x 0929\",\"Description\":\"Custom2\"}]}"), sContents);
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
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\",\"Decimal\":true}]}"), sContents);
    }

    TEST_METHOD(TestCodeNoteChanged)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto* bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"Note description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsFalse(bookmark->IsCustomDescription());

        bookmarks.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"New description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsFalse(bookmark->IsCustomDescription());
    }

    TEST_METHOD(TestCodeNoteChangedCustomDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10,\"Description\":\"My Description\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto* bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsTrue(bookmark->IsCustomDescription());

        bookmarks.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsTrue(bookmark->IsCustomDescription());

        bookmarks.mockGameContext.SetCodeNote(1234U, L"My Description");

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"My Description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsFalse(bookmark->IsCustomDescription());

        bookmarks.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        bookmark = bookmarks.Bookmarks().GetItemAt(0);
        Expects(bookmark != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), bookmark->GetRealNote());
        Assert::AreEqual(std::wstring(L"New description"), bookmark->GetDescription());
        Assert::AreEqual(1234U, bookmark->GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark->GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark->GetFormat());
        Assert::IsFalse(bookmark->IsCustomDescription());
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
        Assert::AreEqual(std::wstring(L"12"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"12"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        memory.at(18) = 12;
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"0c"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"12"), bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"0c"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"12"), bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        memory.at(18) = 15;
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"0f"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0c"), bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());

        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);

        memory.at(18) = 16;
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"0f"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0c"), bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());

        Assert::AreEqual(15, (int)memory.at(18));
    }

    TEST_METHOD(TestAddBookmark)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"00"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark.GetPreviousValue());
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
        Assert::AreEqual(std::wstring(L"NOTE"), bookmark.GetRealNote());
        Assert::AreEqual(2345U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::SixteenBit, bookmark.GetSize());
        Assert::AreEqual(MemFormat::Dec, bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        Assert::IsTrue(bookmarks.IsModified());
    }

    TEST_METHOD(TestAddBookmarkForNoteHex)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.mockGameContext.SetCodeNote(2345U, L"NOTE (BCD)");

        bookmarks.AddBookmark(2345U, MemSize::SixteenBit);

        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"NOTE (BCD)"), bookmark.GetRealNote());
        Assert::AreEqual(2345U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::SixteenBit, bookmark.GetSize());
        Assert::AreEqual(MemFormat::Hex, bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0000"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark.GetPreviousValue());
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
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(5678U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBit, bookmark.GetSize());
        Assert::AreEqual(MemFormat::Dec, bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark.GetPreviousValue());
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
        Assert::AreEqual(std::wstring(L"Freeze"), bookmarks.GetFreezeButtonText());

        // no selected items are frozen - freeze them all
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(3)->SetSelected(true);

        Assert::AreEqual(std::wstring(L"Freeze"), bookmarks.GetFreezeButtonText());
        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Unfreeze"), bookmarks.GetFreezeButtonText());

        // some selected items are frozen - freeze the rest (items 2 and 3 are selected)
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(false);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(true);

        Assert::AreEqual(std::wstring(L"Freeze"), bookmarks.GetFreezeButtonText());
        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Unfreeze"), bookmarks.GetFreezeButtonText());

        // all selected items are frozen - unfreeze them (items 1 and 3 are selected)
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(false);

        Assert::AreEqual(std::wstring(L"Unfreeze"), bookmarks.GetFreezeButtonText());
        bookmarks.ToggleFreezeSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Freeze"), bookmarks.GetFreezeButtonText());
    }

    TEST_METHOD(TestTogglePauseSelected)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(1234U, MemSize::EightBit);
        bookmarks.AddBookmark(2345U, MemSize::EightBit);
        bookmarks.AddBookmark(4567U, MemSize::EightBit);
        bookmarks.AddBookmark(6789U, MemSize::EightBit);
        Assert::AreEqual(std::wstring(L"Pause"), bookmarks.GetPauseButtonText());
        Assert::IsFalse(bookmarks.HasSelection());

        // no selected items are marked to pause - mark them all
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(3)->SetSelected(true);
        Assert::IsTrue(bookmarks.HasSelection());

        Assert::AreEqual(std::wstring(L"Pause"), bookmarks.GetPauseButtonText());
        bookmarks.TogglePauseSelected();
        Assert::IsTrue(bookmarks.HasSelection());

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Stop Pausing"), bookmarks.GetPauseButtonText());

        // some selected items are marked to pause - mark the rest (items 2 and 3 are selected)
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(false);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(true);
        Assert::IsTrue(bookmarks.HasSelection());

        Assert::AreEqual(std::wstring(L"Pause"), bookmarks.GetPauseButtonText());
        bookmarks.TogglePauseSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Stop Pausing"), bookmarks.GetPauseButtonText());

        // all selected items are marked to pause - unmark them (items 1 and 3 are selected)
        bookmarks.Bookmarks().GetItemAt(1)->SetSelected(true);
        bookmarks.Bookmarks().GetItemAt(2)->SetSelected(false);
        Assert::IsTrue(bookmarks.HasSelection());

        Assert::AreEqual(std::wstring(L"Stop Pausing"), bookmarks.GetPauseButtonText());
        bookmarks.TogglePauseSelected();

        Assert::AreEqual({ 4U }, bookmarks.Bookmarks().Count());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(0)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(1)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange, bookmarks.Bookmarks().GetItemAt(2)->GetBehavior());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Bookmarks().GetItemAt(3)->GetBehavior());
        Assert::AreEqual(std::wstring(L"Pause"), bookmarks.GetPauseButtonText());
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
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::EightBit, bookmark.GetSize());
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
            "{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\"}]}");

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
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\"},{\"MemAddr\":\"0x 0929\"}]}"), sContents);
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

    TEST_METHOD(TestPauseOnChange16Bit)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::SixteenBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        bookmark1.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        // no change, dialog should not be shown
        bookmarks.DoFrame();
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        // change second byte of 16-bit value. dialog should be shown and emulator should be paused
        memory.at(5) = 9;
        bookmarks.DoFrame();
        Assert::AreEqual({ 1U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"16-bit 0x0004"));
        bookmarks.mockFrameEventQueue.Reset();

        // row that caused the pause should be highlighted
        Assert::AreEqual(0xFFFFC0C0U, bookmark1.GetRowColor().ARGB);

        // unpause, dialog should not show, emulator should not pause, row should unhighlight
        bookmarks.DoFrame();
        Assert::AreEqual({ 0U }, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::AreEqual(0U, bookmark1.GetRowColor().ARGB);
    }

    TEST_METHOD(TestPauseOnChangeShortDescription)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::EightBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        bookmark1.SetRealNote(L"Short Description");

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
        bookmark1.SetRealNote(L"This description is long enough that it will be truncated at the nearest word");

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
        bookmark1.SetRealNote(L"Level:\n0x0F=1-1\n0x13=1-2\n0x14=1-3");

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
        Assert::AreEqual(std::wstring(L"04030201"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00000000"), bookmark1.GetPreviousValue());
        Assert::AreEqual(0U, bookmark1.GetChanges());

        auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"040302"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"000000"), bookmark2.GetPreviousValue());
        Assert::AreEqual(0U, bookmark2.GetChanges());
        bookmark2.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen); // OnEditMemory should ignore frozen bookmarks

        auto& bookmark3 = *bookmarks.Bookmarks().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0403"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark3.GetPreviousValue());
        Assert::AreEqual(0U, bookmark3.GetChanges());

        auto& bookmark4a = *bookmarks.Bookmarks().GetItemAt(3);
        Assert::AreEqual(std::wstring(L"04"), bookmark4a.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark4a.GetPreviousValue());
        Assert::AreEqual(0U, bookmark4a.GetChanges());
        bookmark4a.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange); // OnEditMemory should not trigger pause
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            Assert::IsFalse(true, L"No dialog should have been shown.");
            return DialogResult::None;
        });

        auto& bookmark4b = *bookmarks.Bookmarks().GetItemAt(4);
        Assert::AreEqual(std::wstring(L"0504"), bookmark4b.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark4b.GetPreviousValue());
        Assert::AreEqual(0U, bookmark4b.GetChanges());

        auto& bookmark5 = *bookmarks.Bookmarks().GetItemAt(5);
        Assert::AreEqual(std::wstring(L"05"), bookmark5.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark5.GetPreviousValue());
        Assert::AreEqual(0U, bookmark5.GetChanges());

        //memory.at(4) = 0x40;
        bookmarks.mockEmulatorContext.WriteMemoryByte(4U, 0x40);

        Assert::AreEqual(std::wstring(L"40030201"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"04030201"), bookmark1.GetPreviousValue());
        Assert::AreEqual(1U, bookmark1.GetChanges());

        Assert::AreEqual(std::wstring(L"400302"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"040302"), bookmark2.GetPreviousValue());
        Assert::AreEqual(1U, bookmark2.GetChanges());

        Assert::AreEqual(std::wstring(L"4003"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0403"), bookmark3.GetPreviousValue());
        Assert::AreEqual(1U, bookmark3.GetChanges());

        Assert::AreEqual(std::wstring(L"40"), bookmark4a.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"04"), bookmark4a.GetPreviousValue());
        Assert::AreEqual(1U, bookmark4a.GetChanges());

        Assert::AreEqual(std::wstring(L"0540"), bookmark4b.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), bookmark4b.GetPreviousValue());
        Assert::AreEqual(1U, bookmark4b.GetChanges());

        Assert::AreEqual(std::wstring(L"05"), bookmark5.GetCurrentValue()); // not affected by the edited memory, should not update
        Assert::AreEqual(std::wstring(L"00"), bookmark5.GetPreviousValue());
        Assert::AreEqual(0U, bookmark5.GetChanges());
    }

    TEST_METHOD(TestUpdateCurrentValueOnSizeChange)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.AddBookmark(4U, MemSize::SixteenBit);
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.DoFrame(); // initialize current and previous values

        Assert::AreEqual(std::wstring(L"0504"), bookmark1.GetCurrentValue());
        bookmark1.SetChanges(0U);

        memory.at(5) = 0x17;
        bookmarks.DoFrame();

        Assert::AreEqual(std::wstring(L"1704"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), bookmark1.GetPreviousValue());
        Assert::AreEqual(1U, bookmark1.GetChanges());

        bookmark1.SetSize(MemSize::EightBit);
        Assert::AreEqual(std::wstring(L"04"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), bookmark1.GetPreviousValue());
        Assert::AreEqual(1U, bookmark1.GetChanges());

        bookmark1.SetSize(MemSize::ThirtyTwoBit);
        Assert::AreEqual(std::wstring(L"07061704"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), bookmark1.GetPreviousValue());
        Assert::AreEqual(1U, bookmark1.GetChanges());
    }

    TEST_METHOD(TestReadOnly)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(1U, MemSize::EightBit);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::IsFalse(bookmark.IsReadOnly());

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(bookmarks.Sizes().Count()); ++nIndex)
        {
            const auto nSize = ra::itoe<MemSize>(bookmarks.Sizes().GetItemAt(nIndex)->GetId());
            bookmark.SetSize(nSize);

            switch (nSize)
            {
                case MemSize::BitCount:
                case MemSize::Text:
                    Assert::IsTrue(bookmark.IsReadOnly());
                    break;

                default:
                    Assert::IsFalse(bookmark.IsReadOnly());
                    break;
            }
        }
    }

    TEST_METHOD(TestAddBookmarkFloat)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(1U, MemSize::Float);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(1U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::Float, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(4U, 0xC0);
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        memory.at(4) = 0x42; // 0x429A4492 => 77.133926
        memory.at(3) = 0x9A;
        memory.at(2) = 0x44;
        memory.at(1) = 0x92;

        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"77.133926"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());
    }

    TEST_METHOD(TestAddBookmarkDouble32)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(4U, MemSize::Double32);

        // without a code note, assume the user is bookmarking the 4 significant bytes
        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(4U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::Double32, bookmark.GetSize());
        Assert::AreEqual(MemFormat::Hex, bookmark.GetFormat()); // default to user preference
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(7U, 0xC0);
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        // with a code note, align the bookmark to the 4 significant bytes
        bookmarks.mockGameContext.SetCodeNote(8U, L"[double] Note description");

        bookmarks.AddBookmark(8U, MemSize::Double32);
        Assert::AreEqual({2U}, bookmarks.Bookmarks().Count());
        const auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"[double] Note description"), bookmark2.GetRealNote());
        Assert::AreEqual(12U, bookmark2.GetAddress()); // adjusted to significant bytes
        Assert::AreEqual(MemSize::Double32, bookmark2.GetSize());
        Assert::AreEqual(MemFormat::Dec, bookmark2.GetFormat()); // implied from note
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark2.GetBehavior());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetPreviousValue());
        Assert::AreEqual(0U, bookmark2.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(15U, 0xC0);
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetPreviousValue());
        Assert::AreEqual(1U, bookmark2.GetChanges());

        // does not exactly match code note address, assume the user is bookmarking the most significant bytes
        bookmarks.AddBookmark(12U, MemSize::Double32);
        Assert::AreEqual({3U}, bookmarks.Bookmarks().Count());
        const auto& bookmark3 = *bookmarks.Bookmarks().GetItemAt(2);
        Assert::AreEqual(std::wstring(L""), bookmark3.GetRealNote());
        Assert::AreEqual(12U, bookmark3.GetAddress()); // adjusted to significant bytes
        Assert::AreEqual(MemSize::Double32, bookmark3.GetSize());
        Assert::AreEqual(MemFormat::Hex, bookmark3.GetFormat()); // default to preference
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark3.GetBehavior());
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark3.GetPreviousValue());
        Assert::AreEqual(0U, bookmark3.GetChanges());
    }

    TEST_METHOD(TestAddBookmarkASCII)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(1U, MemSize::Text);

        Assert::AreEqual({ 1U }, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), bookmark.GetRealNote());
        Assert::AreEqual(1U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::Text, bookmark.GetSize());
        Assert::AreEqual(MemoryBookmarksViewModel::BookmarkBehavior::None, bookmark.GetBehavior());
        Assert::AreEqual(std::wstring(L""), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L""), bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());

        memcpy(&memory.at(1), "Test", 5);
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"Test"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L""), bookmark.GetPreviousValue());
        Assert::AreEqual(1U, bookmark.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(3U, 0x78);
        Assert::AreEqual(std::wstring(L"Text"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Test"), bookmark.GetPreviousValue());
        Assert::AreEqual(2U, bookmark.GetChanges());

        memcpy(&memory.at(1), "EightCharacters", 15);
        bookmarks.DoFrame();
        Assert::AreEqual(std::wstring(L"EightCha"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Text"), bookmark.GetPreviousValue());
        Assert::AreEqual(3U, bookmark.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(9U, 0x73);
        Assert::AreEqual(std::wstring(L"EightCha"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Text"), bookmark.GetPreviousValue());
        Assert::AreEqual(3U, bookmark.GetChanges());

        bookmarks.mockEmulatorContext.WriteMemoryByte(8U, 0x73);
        Assert::AreEqual(std::wstring(L"EightChs"), bookmark.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"EightCha"), bookmark.GetPreviousValue());
        Assert::AreEqual(4U, bookmark.GetChanges());
    }

    TEST_METHOD(TestSetCurrentValue)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        g_pMemoryBookmarksViewModel = &bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(1U, MemSize::EightBit);
        bookmarks.AddBookmark(1U, MemSize::SixteenBit);
        bookmarks.AddBookmark(1U, MemSize::Bit_3);

        Assert::AreEqual({ 3U }, bookmarks.Bookmarks().Count());
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"00"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark1.GetPreviousValue());
        auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"0000"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark2.GetPreviousValue());
        auto& bookmark3 = *bookmarks.Bookmarks().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark3.GetPreviousValue());

        std::wstring sError;
        Assert::IsTrue(bookmark1.SetCurrentValue(L"1C", sError));
        Assert::AreEqual(std::wstring(L"1c"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), bookmark1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"001c"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), bookmark2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark3.GetPreviousValue());

        Assert::IsTrue(bookmark2.SetCurrentValue(L"1234", sError));
        Assert::AreEqual(std::wstring(L"34"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1c"), bookmark1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1234"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"001c"), bookmark2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1"), bookmark3.GetPreviousValue());

        Assert::IsTrue(bookmark3.SetCurrentValue(L"1", sError));
        Assert::AreEqual(std::wstring(L"3c"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"34"), bookmark1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"123c"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1234"), bookmark2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), bookmark3.GetPreviousValue());

        g_pMemoryBookmarksViewModel = nullptr;
    }

    TEST_METHOD(TestSetCurrentValueFloat)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        g_pMemoryBookmarksViewModel = &bookmarks;

        std::array<uint8_t, 64> memory = {};
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark(4U, MemSize::Float);
        bookmarks.AddBookmark(8U, MemSize::MBF32);
        bookmarks.AddBookmark(12U, MemSize::Double32);

        Assert::AreEqual({ 3U }, bookmarks.Bookmarks().Count());
        auto& bookmark1 = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"0.0"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark1.GetPreviousValue());
        auto& bookmark2 = *bookmarks.Bookmarks().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetPreviousValue());
        auto& bookmark3 = *bookmarks.Bookmarks().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0.0"), bookmark3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark3.GetPreviousValue());

        std::wstring sError;
        Assert::IsTrue(bookmark1.SetCurrentValue(L"-2", sError));
        Assert::IsTrue(bookmark2.SetCurrentValue(L"-3", sError));
        Assert::IsTrue(bookmark3.SetCurrentValue(L"68.1234", sError));
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"-3.0"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), bookmark2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"68.123352"), bookmark3.GetCurrentValue()); // conversion through double is inprecise
        Assert::AreEqual(std::wstring(L"0.0"), bookmark3.GetPreviousValue());

        Assert::AreEqual({ 0xC0 }, memory.at(7)); // -2 => 0xC0000000

        Assert::AreEqual({ 0x82 }, memory.at(8)); // -3 => 0x82C00000
        Assert::AreEqual({ 0xC0 }, memory.at(9));

        Assert::AreEqual({ 0x40 }, memory.at(15)); // 68.123 => 0x405107E5
        Assert::AreEqual({ 0x51 }, memory.at(14));
        Assert::AreEqual({ 0x07 }, memory.at(13));
        Assert::AreEqual({ 0xE5 }, memory.at(12));

        Assert::IsTrue(bookmark1.SetCurrentValue(L"3.14", sError));
        Assert::IsTrue(bookmark2.SetCurrentValue(L"6.022", sError));
        Assert::IsTrue(bookmark3.SetCurrentValue(L"-4.56", sError));
        Assert::AreEqual(std::wstring(L"3.14"), bookmark1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-2.0"), bookmark1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"6.022"), bookmark2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-3.0"), bookmark2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"-4.559998"), bookmark3.GetCurrentValue()); // conversion through double is inprecise
        Assert::AreEqual(std::wstring(L"68.123352"), bookmark3.GetPreviousValue());

        g_pMemoryBookmarksViewModel = nullptr;
    }

    TEST_METHOD(TestAddBookmarkIndirect)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); i += 2)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(0x0020U, L"[16-bit pointer]\n+8: data here");

        bookmarks.AddBookmark("I:0x 0020_M:0xW0008");

        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"data here"), bookmark.GetRealNote());
        Assert::AreEqual(0x28U, bookmark.GetAddress()); // $0020=0x20, 0x20+8 = 0x28
        Assert::AreEqual(MemSize::TwentyFourBit, bookmark.GetSize());
        Assert::AreEqual(MemFormat::Dec, bookmark.GetFormat());
        Assert::AreEqual(std::wstring(L"2752552"), bookmark.GetCurrentValue()); // 0x2a0028 = 2752552
        Assert::IsTrue(bookmark.IsIndirectAddress());
        Assert::AreEqual(std::string("I:0x 0020_M:0xW0008"), bookmark.GetIndirectAddress());

        memory.at(0x20) = 0x10;
        memory.at(0x18) = 0xAB;
        bookmarks.DoFrame();

        Assert::AreEqual(std::wstring(L"data here"), bookmark.GetRealNote());
        Assert::AreEqual(0x18U, bookmark.GetAddress()); // $0020=0x10, 0x10+8 = 0x18
        Assert::AreEqual(std::wstring(L"1704107"), bookmark.GetCurrentValue()); // 0x1a00ab = 1704107
    }

    TEST_METHOD(TestLoadBookmarkFileIndirect)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(0x0020U, L"[16-bit pointer]\n+8: data here");
        bookmarks.mockFileSystem.MockFile(L"E:\\Data\\3-Bookmarks.json",
                                          "{\"Bookmarks\":[{\"MemAddr\":\"I:0x 0020_M:0xW0008\"}]}");

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog) {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Import Bookmark File"), vmFileDialog.GetWindowTitle());
                Assert::AreEqual({2U}, vmFileDialog.GetFileTypes().size());
                Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
                Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

                vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

                return DialogResult::OK;
            });

        bookmarks.LoadBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"data here"), bookmark.GetRealNote());
        Assert::AreEqual(std::wstring(L"data here"), bookmark.GetDescription());
        Assert::AreEqual(8U, bookmark.GetAddress());
        Assert::AreEqual(MemSize::TwentyFourBit, bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::IsTrue(bookmark.IsIndirectAddress());
        Assert::AreEqual(std::string("I:0x 0020_M:0xW0008"), bookmark.GetIndirectAddress());
    }

    TEST_METHOD(TestSaveBookmarkFileIndirect)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.SetCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
                                                  "{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\"}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        bookmarks.AddBookmark("I:0x 0020_M:0xW0008");
        Assert::IsTrue(bookmarks.IsModified());

        bool bDialogSeen = false;
        bookmarks.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog) {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Export Bookmark File"), vmFileDialog.GetWindowTitle());
                Assert::AreEqual({1U}, vmFileDialog.GetFileTypes().size());
                Assert::AreEqual(std::wstring(L"json"), vmFileDialog.GetDefaultExtension());
                Assert::AreEqual(std::wstring(L"3-Bookmarks.json"), vmFileDialog.GetFileName());

                vmFileDialog.SetFileName(L"E:\\Data\\3-Bookmarks.json");

                return DialogResult::OK;
            });

        bookmarks.SaveBookmarkFile();

        Assert::IsTrue(bDialogSeen);
        Assert::IsTrue(bookmarks.IsModified()); // export does not update modified flag
        const std::string& sContents = bookmarks.mockFileSystem.GetFileContents(L"E:\\Data\\3-Bookmarks.json");
        Assert::AreEqual(std::string("{\"Bookmarks\":[{\"MemAddr\":\"0xH04d2\"},{\"MemAddr\":\"I:0x 0020_M:0xW0008\"}]}"), sContents);
    }

    TEST_METHOD(TestPauseOnChangeIndirect)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); i += 2)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.AddBookmark("I:0x 0020_M:0xH0008");
        bookmarks.DoFrame(); // initialize current and previous values
        Assert::AreEqual({0U}, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        bookmark.SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);

        // no change, dialog should not be shown
        bookmarks.DoFrame();
        Assert::AreEqual({0U}, bookmarks.mockFrameEventQueue.NumMemoryChanges());

        // value change, dialog should be shown and emulator should be paused
        memory.at(0x28) = 0x10;
        bookmarks.DoFrame();
        Assert::AreEqual({1U}, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0028"));
        bookmarks.mockFrameEventQueue.Reset();

        // pointer change, dialog should be shown
        memory.at(0x20) = 0x10;
        bookmarks.DoFrame();
        Assert::AreEqual({1U}, bookmarks.mockFrameEventQueue.NumMemoryChanges());
        Assert::IsTrue(bookmarks.mockFrameEventQueue.ContainsMemoryChange(L"8-bit 0x0018"));
        bookmarks.mockFrameEventQueue.Reset();

        // pointer change, but value doesn't change, dialog should not be shown
        memory.at(0x20) = 0x00;
        memory.at(0x08) = 0x18;
        bookmarks.DoFrame();
        Assert::AreEqual({0U}, bookmarks.mockFrameEventQueue.NumMemoryChanges());
    }

    TEST_METHOD(TestUpdateCurrentValue)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        std::array<uint8_t, 64> memory = {};
        memory.at(4) = 8;
        // 0x42883EFA => 68.123
        memory.at(0x10) = 0xFA;
        memory.at(0x11) = 0x3E;
        memory.at(0x12) = 0x88;
        memory.at(0x13) = 0x42;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);

        bookmarks.AddBookmark("0xX0010");

        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);

        bookmark.UpdateCurrentValue();
        Assert::AreEqual(std::wstring(L"42883efa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::Float);
        Assert::AreEqual(std::wstring(L"68.123001"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::ThirtyTwoBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e8842"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::TwentyFourBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e88"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::SixteenBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::EightBit);
        Assert::AreEqual(std::wstring(L"fa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::SixteenBit);
        Assert::AreEqual(std::wstring(L"3efa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::TwentyFourBit);
        Assert::AreEqual(std::wstring(L"883efa"), bookmark.GetCurrentValue());
    }

    TEST_METHOD(TestUpdateCurrentValueIndirect)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        std::array<uint8_t, 64> memory = {};
        memory.at(4) = 8;
        // 0x42883EFA => 68.123
        memory.at(0x10) = 0xFA;
        memory.at(0x11) = 0x3E;
        memory.at(0x12) = 0x88;
        memory.at(0x13) = 0x42;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);

        bookmarks.AddBookmark("I:0xX0004_M:0xX0008"); // 4=8 $(8+8)= $16

        Assert::AreEqual({1U}, bookmarks.Bookmarks().Count());
        auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);

        bookmark.UpdateCurrentValue();
        Assert::AreEqual(std::wstring(L"42883efa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::Float);
        Assert::AreEqual(std::wstring(L"68.123001"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::ThirtyTwoBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e8842"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::TwentyFourBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e88"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::SixteenBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::EightBit);
        Assert::AreEqual(std::wstring(L"fa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::SixteenBit);
        Assert::AreEqual(std::wstring(L"3efa"), bookmark.GetCurrentValue());

        bookmark.SetSize(MemSize::TwentyFourBit);
        Assert::AreEqual(std::wstring(L"883efa"), bookmark.GetCurrentValue());
    }

    TEST_METHOD(TestDoFrameFrozenBookmarks)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        std::array<unsigned char, 32> memory{};
        bookmarks.mockEmulatorContext.MockMemory(memory);
        bookmarks.mockConsoleContext.AddMemoryRegion(0U, 16,
                                                     ra::data::context::ConsoleContext::AddressType::SystemRAM);

        bookmarks.AddBookmark("M:0xX0000");
        bookmarks.AddBookmark("I:0xX0004_M:0xH0008");
        auto* pBookmark1 = bookmarks.Bookmarks().GetItemAt(0);
        Expects(pBookmark1 != nullptr);
        auto* pBookmark2 = bookmarks.Bookmarks().GetItemAt(1);
        Expects(pBookmark2 != nullptr);

        memory.at(0) = 6;
        memory.at(1) = 1;
        memory.at(4) = 4;
        memory.at(12) = 7;
        bookmarks.DoFrame();

        Assert::AreEqual(0U, pBookmark1->GetAddress());
        Assert::IsFalse(pBookmark1->IsIndirectAddress());
        Assert::AreEqual(std::wstring(L"00000106"), pBookmark1->GetCurrentValue());

        Assert::AreEqual(12U, pBookmark2->GetAddress());
        Assert::IsTrue(pBookmark2->IsIndirectAddress());
        Assert::AreEqual(std::wstring(L"07"), pBookmark2->GetCurrentValue());

        pBookmark1->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        pBookmark2->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::Frozen);

        memory.at(0) = 3;
        memory.at(12) = 4;

        bookmarks.DoFrame();

        // frozen values should be written back to memory
        Assert::AreEqual(0U, pBookmark1->GetAddress());
        Assert::AreEqual(std::wstring(L"00000106"), pBookmark1->GetCurrentValue());
        Assert::AreEqual({6}, memory.at(0));

        Assert::AreEqual(12U, pBookmark2->GetAddress());
        Assert::AreEqual(std::wstring(L"07"), pBookmark2->GetCurrentValue());
        Assert::AreEqual({7}, memory.at(12));

        // console says only 16 bytes are valid. if pointer points beyond that, it shouldn't write
        memory.at(4) = 20;
        Assert::AreEqual({0}, memory.at(28));

        bookmarks.DoFrame();
        Assert::AreEqual({28}, pBookmark2->GetAddress()); // address updated
        Assert::AreEqual({0}, memory.at(28));             // but not memory

        // null is implicitly invalid
        memory.at(4) = 0;
        Assert::AreEqual({0}, memory.at(8));

        bookmarks.DoFrame();
        Assert::AreEqual({8}, pBookmark2->GetAddress()); // address updated
        Assert::AreEqual({0}, memory.at(8));             // but not memory
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
