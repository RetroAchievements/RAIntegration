#ifndef RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#define RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"

#include "services\TextReader.hh"
#include "services\TextWriter.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryBookmarksViewModel : public WindowViewModelBase,
    protected ra::data::context::GameContext::NotifyTarget,
    protected ra::data::context::EmulatorContext::NotifyTarget,
    protected ra::data::context::EmulatorContext::DispatchesReadMemory,
    protected ra::ui::ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryBookmarksViewModel() noexcept;
    ~MemoryBookmarksViewModel() = default;

    MemoryBookmarksViewModel(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(const MemoryBookmarksViewModel&) noexcept = delete;
    MemoryBookmarksViewModel(MemoryBookmarksViewModel&&) noexcept = delete;
    MemoryBookmarksViewModel& operator=(MemoryBookmarksViewModel&&) noexcept = delete;
    
    void InitializeNotifyTargets();

    virtual void DoFrame();

    enum class BookmarkBehavior
    {
        None = 0,
        Frozen,
        PauseOnChange,
    };

    class MemoryBookmarkViewModel : public LookupItemViewModel,
        protected ra::data::context::EmulatorContext::DispatchesReadMemory
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for bookmark summary.
        /// </summary>
        static const StringModelProperty DescriptionProperty;

        /// <summary>
        /// Gets the bookmark summary.
        /// </summary>
        const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

        /// <summary>
        /// Sets the bookmark summary.
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
        /// The <see cref="ModelProperty" /> for the note supporting the bookmark.
        /// </summary>
        static const StringModelProperty RealNoteProperty;

        /// <summary>
        /// Gets the note supporting the bookmark.
        /// </summary>
        const std::wstring& GetRealNote() const { return GetValue(RealNoteProperty); }

        /// <summary>
        /// Sets the note supporting the bookmark.
        /// </summary>
        void SetRealNote(const std::wstring& sValue) { SetValue(RealNoteProperty, sValue); }

        /// <summary>
        /// Sets the <see cref="RealNoteProperty" /> based on the <see cref="CurrentAddressProperty" />.
        /// </summary>
        void UpdateRealNote();

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark address.
        /// </summary>
        static const IntModelProperty AddressProperty;

        /// <summary>
        /// Gets the bookmark address.
        /// </summary>
        ByteAddress GetAddress() const noexcept { return m_nAddress; }

        /// <summary>
        /// Sets the bookmark address.
        /// </summary>
        void SetAddress(ByteAddress value) { SetValue(AddressProperty, value); }

        /// <summary>
        /// Sets an indirect bookmark address.
        /// </summary>
        void SetIndirectAddress(const std::string& sSerialized);

        /// <summary>
        /// Gets the indirect bookmark address.
        /// </summary>
        /// <returns></returns>
        const std::string& GetIndirectAddress() const noexcept { return m_sIndirectAddress; }

        /// <summary>
        /// Gets whether or not the bookmark has an indirect bookmark address.
        /// </summary>
        bool HasIndirectAddress() const noexcept { return !m_sIndirectAddress.empty(); }

        /// <summary>
        /// Updates the current address from the indirect address chain.
        /// </summary>
        /// <returns><c>true</c> if the indirect address chain was valid, <c>false</c> if not.</returns>
        /// <remarks>Address will be updated to whatever the chain evaluates to, regardless of the chain's validity</remarks>
        bool UpdateCurrentAddress();

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark size.
        /// </summary>
        static const IntModelProperty SizeProperty;

        /// <summary>
        /// Gets the bookmark size.
        /// </summary>
        MemSize GetSize() const noexcept { return m_nSize; }

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
        static const StringModelProperty CurrentValueProperty;

        /// <summary>
        /// Gets the current value of the bookmarked address. 
        /// </summary>
        const std::wstring& GetCurrentValue() const { return GetValue(CurrentValueProperty); }

        /// <summary>
        /// Sets the current value of the bookmarked address.
        /// </summary>
        bool SetCurrentValue(const std::wstring& sValue, _Out_ std::wstring& sError);

        /// <summary>
        /// Gets the unformatted current value of the bookmarked address.
        /// </summary>
        uint32_t GetCurrentValueRaw() const noexcept { return m_nValue; }

        /// <summary>
        /// Sets the current value of the bookmarked address.
        /// </summary>
        void SetCurrentValueRaw(unsigned nValue);

        /// <summary>
        /// The <see cref="ModelProperty" /> for the previous value of the bookmarked address.
        /// </summary>
        static const StringModelProperty PreviousValueProperty;

        /// <summary>
        /// Gets the previous value of the bookmarked address.
        /// </summary>
        const std::wstring& GetPreviousValue() const { return GetValue(PreviousValueProperty); }

        /// <summary>
        /// Sets the previous value of the bookmarked address.
        /// </summary>
        void SetPreviousValue(const std::wstring& sValue) { SetValue(PreviousValueProperty, sValue); }

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
        /// The <see cref="ModelProperty" /> for whether or not the <see cref="CurrentValueProperty"/> is read only.
        /// </summary>
        static const BoolModelProperty ReadOnlyProperty;

        /// <summary>
        /// Gets whether or not the <see cref="CurrentValueProperty"/> is read only. 
        /// </summary>
        bool IsReadOnly() const { return GetValue(ReadOnlyProperty); }

        /// <summary>
        /// Sets whether or not the <see cref="CurrentValueProperty"/> is read only.
        /// </summary>
        void SetReadOnly(bool bValue) { SetValue(ReadOnlyProperty, bValue); }

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

        /// <summary>
        /// Gets whether or not the bookmark has been modified.
        /// </summary>
        bool IsModified() const noexcept { return m_bModified; }

        /// <summary>
        /// Clears the modified flag.
        /// </summary>
        void ResetModified() noexcept { m_bModified = false; }

        /// <summary>
        /// Determines if the bookmarked memory has changed since the last time MemoryChanged was called.
        /// </summary>
        /// <returns><c>true</c> if the memory has changed, <c>false</c> if not.</returns>
        bool MemoryChanged();

        /// <summary>
        /// Starts initialization of the bookmark.
        /// </summary>
        void BeginInitialization() noexcept { m_bInitialized = false; }

        /// <summary>
        /// Finishes initialization of the bookmark.
        /// </summary>
        void EndInitialization();

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the <see cref="CurrentValueProperty"/> is dirty.
        /// </summary>
        static const BoolModelProperty IsDirtyProperty;

        /// <summary>
        /// Gets whether or not the <see cref="CurrentValueProperty"/> is dirty. 
        /// </summary>
        bool IsDirty() const { return GetValue(IsDirtyProperty); }

        /// <summary>
        /// Sets whether or not the <see cref="CurrentValueProperty"/> is dirty.
        /// </summary>
        void SetDirty(bool bValue) { SetValue(IsDirtyProperty, bValue); }

        /// <summary>
        /// Updates the <see cref="CurrentValueProperty" /> for the bookmark.
        /// </summary>
        void UpdateCurrentValue();

    protected:
        void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
        void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

        void OnValueChanged();
        void OnSizeChanged();

        unsigned ReadValue() const;

        void SetAddressWithoutUpdatingValue(ra::ByteAddress nNewAddress);

    private:
        std::wstring BuildCurrentValue() const;
        static std::wstring ExtractDescriptionHeader(const std::wstring& sFullNote);

        // keep address/size/value fields directly accessible for speed - also keep in ValueProperty for binding
        ra::ByteAddress m_nAddress = 0;
        uint32_t m_nValue = 0;
        MemSize m_nSize = MemSize::EightBit;
        bool m_bModified = false;
        bool m_bInitialized = false;
        bool m_bSyncingDescriptionHeader = false;

        std::string m_sIndirectAddress;
        std::unique_ptr<uint8_t[]> m_pBuffer;
        rc_value_t* m_pValue = nullptr;
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
    /// Adds a bookmark to the list.
    /// </summary>
    void AddBookmark(const std::string& sSerialized);

    /// <summary>
    /// Removes any bookmarks that are currently selected.
    /// </summary>
    /// <returns>Number of bookmarks that were removed</returns>
    int RemoveSelectedBookmarks();

    static const BoolModelProperty HasSelectionProperty;
    const bool HasSelection() const { return GetValue(HasSelectionProperty); }

    static const BoolModelProperty HasSingleSelectionProperty;
    static const IntModelProperty SingleSelectionIndexProperty;

    static const StringModelProperty FreezeButtonTextProperty;
    const std::wstring& GetFreezeButtonText() const { return GetValue(FreezeButtonTextProperty); }

    /// <summary>
    /// Freezes the selected items. Or, if they're already all frozen, unfreezes them.
    /// </summary>
    void ToggleFreezeSelected();

    static const StringModelProperty PauseButtonTextProperty;
    const std::wstring& GetPauseButtonText() const { return GetValue(PauseButtonTextProperty); }

    /// <summary>
    /// Marks the selected items to pause on change. Or, if they're already all marked, unmarks them.
    /// </summary>
    void TogglePauseSelected();

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
    void SaveBookmarkFile();

    void BeginWritingMemory() noexcept { m_nWritingMemoryCount++; }
    void EndWritingMemory();

protected:
    void LoadBookmarks(ra::services::TextReader& sBookmarksFile);
    void SaveBookmarks(ra::services::TextWriter& sBookmarksFile);

    // ra::data::context::GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

    // ra::data::context::EmulatorContext::NotifyTarget
    void OnByteWritten(ra::ByteAddress, uint8_t) override;

    // ra::ui::ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnEndViewModelCollectionUpdate() override;

    void UpdateBookmark(MemoryBookmarkViewModel& pBookmark);

    bool IsModified() const;
    size_t m_nUnmodifiedBookmarkCount = 0;

private:
    bool ShouldFreeze() const;
    void UpdateFreezeButtonText();

    bool ShouldPause() const;
    void UpdatePauseButtonText();

    static void InitializeBookmark(MemoryBookmarkViewModel& vmBookmark, const std::string& sSerialized);
    static void InitializeBookmark(MemoryBookmarkViewModel& vmBookmark);

    void UpdateHasSelection();

    ViewModelCollection<MemoryBookmarkViewModel> m_vBookmarks;
    LookupItemViewModelCollection m_vSizes;
    LookupItemViewModelCollection m_vFormats;
    LookupItemViewModelCollection m_vBehaviors;

    unsigned int m_nLoadedGameId = 0;
    int m_nWritingMemoryCount = 0;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
