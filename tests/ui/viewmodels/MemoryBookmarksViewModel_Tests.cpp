#include "CppUnitTest.h"

#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
        ra::data::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryBookmarksViewModelHarness bookmarks;

        Assert::AreEqual(0U, bookmarks.Bookmarks().Count());

        Assert::AreEqual(3U, bookmarks.Sizes().Count());
        Assert::AreEqual((int)MemSize::EightBit, bookmarks.Sizes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"8-bit"), bookmarks.Sizes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemSize::SixteenBit, bookmarks.Sizes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), bookmarks.Sizes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBit, bookmarks.Sizes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), bookmarks.Sizes().GetItemAt(2)->GetLabel());

        Assert::AreEqual(2U, bookmarks.Formats().Count());
        Assert::AreEqual((int)MemFormat::Hex, bookmarks.Formats().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Hex"), bookmarks.Formats().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemFormat::Dec, bookmarks.Formats().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Dec"), bookmarks.Formats().GetItemAt(1)->GetLabel());

        Assert::AreEqual(2U, bookmarks.Behaviors().Count());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, bookmarks.Behaviors().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), bookmarks.Behaviors().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::Frozen, bookmarks.Behaviors().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Frozen"), bookmarks.Behaviors().GetItemAt(1)->GetLabel());
    }

    TEST_METHOD(TestLoadBookmarksOnGameChanged)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":1,\"Decimal\":true}]}");

        Assert::AreEqual(0U, bookmarks.Bookmarks().Count());
        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"desc"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Dec, (int)bookmark.GetFormat());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, (int)bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
    }

    TEST_METHOD(TestLoadBookmarksOnGameChangedNotVisible)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(false);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(0U, bookmarks.Bookmarks().Count());

        bookmarks.SetIsVisible(true);
        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());
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
        Assert::AreEqual(2U, bookmarks.Bookmarks().Count());

        bookmarks.mockGameContext.SetGameId(4U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"4",
            "{\"Bookmarks\":[{\"Description\":\"desc3\",\"Address\":5555,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"desc3"), bookmark.GetDescription());
        Assert::AreEqual(5555U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::SixteenBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, (int)bookmark.GetBehavior());
        Assert::AreEqual(0U, bookmark.GetCurrentValue());
        Assert::AreEqual(0U, bookmark.GetPreviousValue());
        Assert::AreEqual(0U, bookmark.GetChanges());
    }

    TEST_METHOD(TestLoadBookmarksNoFile)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":1234,\"Type\":2,\"Decimal\":false}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());

        bookmarks.mockGameContext.SetGameId(4U);
        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(0U, bookmarks.Bookmarks().Count());
    }

    TEST_METHOD(TestLoadBookmarksDescriptionFromCodeNotes)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockGameContext.MockCodeNote(1234U, L"Note description");
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Address\":1234,\"Size\":10}]}");

        bookmarks.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());
        const auto& bookmark = *bookmarks.Bookmarks().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"Note description"), bookmark.GetDescription());
        Assert::AreEqual(1234U, bookmark.GetAddress());
        Assert::AreEqual((int)MemSize::EightBit, (int)bookmark.GetSize());
        Assert::AreEqual((int)MemFormat::Hex, (int)bookmark.GetFormat());
    }

    TEST_METHOD(TestDoFrame)
    {
        MemoryBookmarksViewModelHarness bookmarks;
        bookmarks.SetIsVisible(true);
        bookmarks.mockGameContext.SetGameId(3U);
        bookmarks.mockLocalStorage.MockStoredData(ra::services::StorageItemType::Bookmarks, L"3",
            "{\"Bookmarks\":[{\"Description\":\"desc\",\"Address\":18,\"Type\":1,\"Decimal\":false}]}");

        std::array<uint8_t, 64> memory;
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        bookmarks.mockEmulatorContext.MockMemory(memory);

        bookmarks.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(1U, bookmarks.Bookmarks().Count());

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
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
