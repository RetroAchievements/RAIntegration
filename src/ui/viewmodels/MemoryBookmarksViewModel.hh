#ifndef RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#define RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#pragma once

#include "data\Types.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryBookmarksViewModel : public WindowViewModelBase, protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryBookmarksViewModel() = default;
    ~MemoryBookmarksViewModel() = default;

    MemoryBookmarksViewModel(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel(MemoryBookmarksViewModel&&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(MemoryBookmarksViewModel&&) noexcept = delete;
    
    void InitBookmarks();

    enum BookmarkBehavior
    {
        None = 0,
        Frozen,
    };

    class MemoryBookmarkViewModel : public LookupItemViewModel
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark description.
        /// </summary>
        static const StringModelProperty DescriptionProperty;

        /// <summary>
        /// Gets the bookmark description.
        /// </summary>
        const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

        /// <summary>
        /// Sets the bookmark description.
        /// </summary>
        void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark address.
        /// </summary>
        static const IntModelProperty AddressProperty;

        /// <summary>
        /// Gets the bookmark address.
        /// </summary>
        ByteAddress GetAddress() const { return GetValue(AddressProperty); }

        /// <summary>
        /// Sets the bookmark address.
        /// </summary>
        void SetAddress(ByteAddress value) { SetValue(AddressProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark size.
        /// </summary>
        static const IntModelProperty SizeProperty;

        /// <summary>
        /// Gets the bookmark size.
        /// </summary>
        MemSize GetSize() const { return ra::itoe<MemSize>(GetValue(SizeProperty)); }

        /// <summary>
        /// Sets the bookmark size.
        /// </summary>
        void SetSize(MemSize value) { SetValue(SizeProperty, ra::etoi(value)); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark format.
        /// </summary>
        static const IntModelProperty FormatProperty;

        /// <summary>
        /// Gets the bookmark format.
        /// </summary>
        MemFormat GetFormat() const { return ra::itoe<MemFormat>(GetValue(FormatProperty)); }

        /// <summary>
        /// Sets the bookmark format.
        /// </summary>
        void SetFormat(MemFormat value) { SetValue(FormatProperty, ra::etoi(value)); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the current value of the bookmarked address.
        /// </summary>
        static const IntModelProperty CurrentValueProperty;

        /// <summary>
        /// Gets the current value of the bookmarked address. 
        /// </summary>
        unsigned int GetCurrentValue() const { return static_cast<unsigned int>(GetValue(CurrentValueProperty)); }

        /// <summary>
        /// Sets the current value of the bookmarked address.
        /// </summary>
        void SetCurrentValue(unsigned int value) { SetValue(CurrentValueProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the previous value of the bookmarked address.
        /// </summary>
        static const IntModelProperty PreviousValueProperty;

        /// <summary>
        /// Gets the previous value of the bookmarked address.
        /// </summary>
        unsigned int GetPreviousValue() const { return static_cast<unsigned int>(GetValue(PreviousValueProperty)); }

        /// <summary>
        /// Sets the previous value of the bookmarked address.
        /// </summary>
        void SetPreviousValue(unsigned int value) { SetValue(PreviousValueProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the number of times the value of the bookmarked address changed.
        /// </summary>
        static const IntModelProperty ChangesProperty;

        /// <summary>
        /// Gets the number of times the value of the bookmarked address changed.
        /// </summary>
        unsigned int GetChanges() const { return static_cast<unsigned int>(GetValue(ChangesProperty)); }

        /// <summary>
        /// Sets the number of times the value of the bookmarked address changed.
        /// </summary>
        void SetChanges(unsigned int value) { SetValue(ChangesProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the behavior of the bookmark.
        /// </summary>
        static const IntModelProperty BehaviorProperty;

        /// <summary>
        /// Gets the behavior of the bookmark.
        /// </summary>
        BookmarkBehavior GetBehavior() const { return ra::itoe<BookmarkBehavior>(GetValue(BehaviorProperty)); }

        /// <summary>
        /// Sets the behavior of the bookmark.
        /// </summary>
        void SetBehavior(BookmarkBehavior value) { SetValue(BehaviorProperty, ra::etoi(value)); }
    };

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    ViewModelCollection<MemoryBookmarkViewModel>& Bookmarks() noexcept
    {
        return m_vBookmarks;
    }

    /// <summary>
    /// Gets the list of selectable sizes for each bookmark.
    /// </summary>
    const LookupItemViewModelCollection& Sizes() const noexcept
    {
        return m_vSizes;
    }

    /// <summary>
    /// Gets the list of selectable formats for each bookmark.
    /// </summary>
    const LookupItemViewModelCollection& Formats() const noexcept
    {
        return m_vFormats;
    }

    /// <summary>
    /// Gets the list of selectable behaviors for each bookmark.
    /// </summary>
    const LookupItemViewModelCollection& Behaviors() const noexcept
    {
        return m_vBehaviors;
    }

private:
    ViewModelCollection<MemoryBookmarkViewModel> m_vBookmarks;
    LookupItemViewModelCollection m_vSizes;
    LookupItemViewModelCollection m_vFormats;
    LookupItemViewModelCollection m_vBehaviors;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
