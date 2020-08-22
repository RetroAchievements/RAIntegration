#ifndef RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#define RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#pragma once

#include "data\GameContext.hh"
#include "data\Types.hh"

#include "services\TextReader.hh"
#include "services\TextWriter.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryBookmarksViewModel : public WindowViewModelBase,
    protected ra::data::GameContext::NotifyTarget,
    protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryBookmarksViewModel() noexcept;
    ~MemoryBookmarksViewModel() = default;

    MemoryBookmarksViewModel(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel(MemoryBookmarksViewModel&&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(MemoryBookmarksViewModel&&) noexcept = delete;
    
    void DoFrame();

    enum class BookmarkBehavior
    {
        None = 0,
        Frozen,
        PauseOnChange,
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
        /// The <see cref="ModelProperty" /> for the whether the bookmark description is custom.
        /// </summary>
        static const BoolModelProperty IsCustomDescriptionProperty;

        /// <summary>
        /// Gets whether the bookmark description is custom.
        /// </summary>
        bool IsCustomDescription() const { return GetValue(IsCustomDescriptionProperty); }

        /// <summary>
        /// Sets whether the bookmark description is custom.
        /// </summary>
        void SetIsCustomDescription(bool bValue) { SetValue(IsCustomDescriptionProperty, bValue); }

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
        unsigned int GetCurrentValue() const { return gsl::narrow_cast<unsigned int>(GetValue(CurrentValueProperty)); }

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
        unsigned int GetPreviousValue() const { return gsl::narrow_cast<unsigned int>(GetValue(PreviousValueProperty)); }

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
        unsigned int GetChanges() const { return gsl::narrow_cast<unsigned int>(GetValue(ChangesProperty)); }

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

        /// <summary>
        /// The <see cref="ModelProperty" /> for the row color.
        /// </summary>
        static const IntModelProperty RowColorProperty;

        /// <summary>
        /// Gets the row color.
        /// </summary>
        Color GetRowColor() const { return Color(ra::to_unsigned(GetValue(RowColorProperty))); }

        /// <summary>
        /// Sets the row color.
        /// </summary>
        void SetRowColor(Color value) { SetValue(RowColorProperty, ra::to_signed(value.ARGB)); }
    };

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    ViewModelCollection<MemoryBookmarkViewModel>& Bookmarks() noexcept
    {
        return m_vBookmarks;
    }

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    const ViewModelCollection<MemoryBookmarkViewModel>& Bookmarks() const noexcept
    {
        return m_vBookmarks;
    }

    /// <summary>
    /// Determines if a bookmark exists for the specified address.
    /// </summary>
    bool HasBookmark(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Determines if a bookmark that is frozen exists for the specified address.
    /// </summary>
    bool HasFrozenBookmark(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Tells the control that the user has modified memory and any associated bookmarks should be updated
    /// </summary>
    void OnEditMemory(ra::ByteAddress nAddress);

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

    /// <summary>
    /// Adds a bookmark to the list.
    /// </summary>
    void AddBookmark(ra::ByteAddress nAddress, MemSize nSize);

    /// <summary>
    /// Removes any bookmarks that are currently selected.
    /// </summary>
    /// <returns>Number of bookmarks that were removed</returns>
    int RemoveSelectedBookmarks();

    /// <summary>
    /// Freezes the selected items. Or, if they're already all frozen, unfreezes them.
    /// </summary>
    void ToggleFreezeSelected();

    /// <summary>
    /// Moves the selected bookmarks higher in the list.
    /// </summary>
    void MoveSelectedBookmarksUp();

    /// <summary>
    /// Moves the selected bookmarks higher in the list.
    /// </summary>
    void MoveSelectedBookmarksDown();

    /// <summary>
    /// Resets the Changes counter on all bookmarks.
    /// </summary>
    void ClearAllChanges();

    /// <summary>
    /// Prompts the user to select a bookmarks file to load.
    /// </summary>
    void LoadBookmarkFile();

    /// <summary>
    /// Prompts the user to select a bookmarks file to save.
    /// </summary>
    void SaveBookmarkFile() const;

protected:
    void LoadBookmarks(ra::services::TextReader& sBookmarksFile);
    void SaveBookmarks(ra::services::TextWriter& sBookmarksFile) const;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;

    // ra::data::GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

    bool m_bModified = false;

private:
    ViewModelCollection<MemoryBookmarkViewModel> m_vBookmarks;
    LookupItemViewModelCollection m_vSizes;
    LookupItemViewModelCollection m_vFormats;
    LookupItemViewModelCollection m_vBehaviors;

    unsigned int m_nLoadedGameId = 0;
    bool m_bIgnoreValueChanged = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
