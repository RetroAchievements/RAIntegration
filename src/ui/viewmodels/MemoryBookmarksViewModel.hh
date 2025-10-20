#ifndef RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#define RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"

#include "services\TextReader.hh"
#include "services\TextWriter.hh"

#include "ui\WindowViewModelBase.hh"
#include "ui\viewmodels\MemoryWatchListViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryBookmarksViewModel : public WindowViewModelBase,
    protected ra::data::context::GameContext::NotifyTarget,
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

    void DoFrame();

    enum class BookmarkBehavior
    {
        None = 0,
        Frozen,
        PauseOnChange,
    };

    class MemoryBookmarkViewModel : public MemoryWatchViewModel
    {
    public:
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

    protected:
        void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

        bool IgnoreValueChange(uint32_t nNewValue) override;
        bool ChangeValue(uint32_t nNewValue) override;

    private:
        void HandlePauseOnChange();
    };

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    MemoryWatchListViewModel& Bookmarks() noexcept
    {
        return m_vmMemoryWatchList;
    }

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    const MemoryWatchListViewModel& Bookmarks() const noexcept
    {
        return m_vmMemoryWatchList;
    }

    /// <summary>
    /// Determines if a bookmark exists for the specified address.
    /// </summary>
    bool HasBookmark(ra::ByteAddress nAddress) const noexcept;

    /// <summary>
    /// Determines if a bookmark that is frozen exists for the specified address.
    /// </summary>
    bool HasFrozenBookmark(ra::ByteAddress nAddress) const;

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
    /// Prompts the user to select a bookmarks file to load.
    /// </summary>
    void LoadBookmarkFile();

    /// <summary>
    /// Prompts the user to select a bookmarks file to save.
    /// </summary>
    void SaveBookmarkFile();

    void ClearBehaviors();

protected:
    void LoadBookmarks(ra::services::TextReader& sBookmarksFile);
    void SaveBookmarks(ra::services::TextWriter& sBookmarksFile);

    // ra::data::context::GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnEndGameLoad() override;

    // ra::ui::ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnEndViewModelCollectionUpdate() override;

    bool IsModified() const noexcept;
    size_t m_nUnmodifiedBookmarkCount = 0;

private:
    bool ShouldFreeze() const;
    void UpdateFreezeButtonText();

    bool ShouldPause() const;
    void UpdatePauseButtonText();

    static void InitializeBookmark(MemoryWatchViewModel& vmBookmark, const std::string& sSerialized);
    static void InitializeBookmark(MemoryWatchViewModel& vmBookmark);

    MemoryWatchListViewModel m_vmMemoryWatchList;
    LookupItemViewModelCollection m_vBehaviors;

    unsigned int m_nLoadedGameId = 0;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYBOOKMARKSVIEWMODEL_H
