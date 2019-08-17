#include "MemoryBookmarksViewModel.hh"

#include "RA_StringUtils.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty("MemoryBookmarkViewModel", "Description", L"");
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty("MemoryBookmarkViewModel", "Address", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty("MemoryBookmarkViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty("MemoryBookmarkViewModel", "Format", ra::etoi(MemFormat::Hex));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty("MemoryBookmarkViewModel", "CurrentValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty("MemoryBookmarkViewModel", "PreviousValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty("MemoryBookmarkViewModel", "Changes", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty("MemoryBookmarkViewModel", "Behavior", ra::etoi(BookmarkBehavior::None));

void MemoryBookmarksViewModel::InitBookmarks()
{
    if (m_vSizes.Count() == 0)
    {
        m_vSizes.Add(ra::etoi(MemSize::EightBit), L"8-bit");
        m_vSizes.Add(ra::etoi(MemSize::SixteenBit), L"16-bit");
        m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), L"32-bit");
    }

    if (m_vFormats.Count() == 0)
    {
        m_vFormats.Add(ra::etoi(MemFormat::Hex), L"Hex");
        m_vFormats.Add(ra::etoi(MemFormat::Dec), L"Dec");
    }

    if (m_vBehaviors.Count() == 0)
    {
        m_vBehaviors.Add(ra::etoi(BookmarkBehavior::None), L"");
        m_vBehaviors.Add(ra::etoi(BookmarkBehavior::Frozen), L"Frozen");
    }

    if (m_vBookmarks.Count() == 0)
    {
        auto& vmAchievement = m_vBookmarks.Add();
        vmAchievement.SetDescription(L"Test Description");
        vmAchievement.SetAddress(0x1234);
        vmAchievement.SetSize(MemSize::EightBit);
        vmAchievement.SetFormat(MemFormat::Dec);
        vmAchievement.SetCurrentValue(99);
        vmAchievement.SetPreviousValue(66);
        vmAchievement.SetChanges(12);

        auto& vmAchievement2 = m_vBookmarks.Add();
        vmAchievement2.SetDescription(L"Item 2");
        vmAchievement2.SetAddress(0xABCD);
        vmAchievement2.SetSize(MemSize::ThirtyTwoBit);
        vmAchievement2.SetCurrentValue(99);
        vmAchievement2.SetPreviousValue(66);
        vmAchievement2.SetBehavior(BookmarkBehavior::Frozen);

        SetWindowTitle(L"Memory Bookmarks");

        m_vBookmarks.AddNotifyTarget(*this);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
