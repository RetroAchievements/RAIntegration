#ifndef RA_UI_CODENOTESVIEWMODEL_H
#define RA_UI_CODENOTESVIEWMODEL_H
#pragma once

#include "data\context\GameContext.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class CodeNotesViewModel : public WindowViewModelBase,
    protected ra::data::context::GameContext::NotifyTarget,
    protected ra::ui::ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 CodeNotesViewModel() noexcept;
    ~CodeNotesViewModel() = default;

    CodeNotesViewModel(const CodeNotesViewModel&) noexcept = delete;
    CodeNotesViewModel& operator=(const CodeNotesViewModel&) noexcept = delete;
    CodeNotesViewModel(CodeNotesViewModel&&) noexcept = delete;
    CodeNotesViewModel& operator=(CodeNotesViewModel&&) noexcept = delete;
    
    class CodeNoteViewModel : public ViewModelBase
    {
    public:
        CodeNoteViewModel(const std::wstring& sLabel, const std::wstring& sNote) noexcept
        {
            GSL_SUPPRESS_F6 SetValue(LabelProperty, sLabel);
            GSL_SUPPRESS_F6 SetValue(NoteProperty, sNote);
        }

        CodeNoteViewModel(const std::wstring&& sLabel, const std::wstring&& sNote) noexcept
        {
            GSL_SUPPRESS_F6 SetValue(LabelProperty, sLabel);
            GSL_SUPPRESS_F6 SetValue(NoteProperty, sNote);
        }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the label.
        /// </summary>
        static const StringModelProperty LabelProperty;

        /// <summary>
        /// Gets the label to display.
        /// </summary>
        const std::wstring& GetLabel() const { return GetValue(LabelProperty); }

        /// <summary>
        /// Sets the label to display.
        /// </summary>
        void SetLabel(const std::wstring& sValue) { SetValue(LabelProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the note.
        /// </summary>
        static const StringModelProperty NoteProperty;

        /// <summary>
        /// Gets the note to display.
        /// </summary>
        const std::wstring& GetNote() const { return GetValue(NoteProperty); }

        /// <summary>
        /// Sets the note to display.
        /// </summary>
        void SetNote(const std::wstring& sValue) { SetValue(NoteProperty, sValue); }


        /// <summary>
        /// The <see cref="ModelProperty" /> for the whether the bookmark is selected.
        /// </summary>
        static const BoolModelProperty IsSelectedProperty;

        /// <summary>
        /// Gets whether the bookmark is selected.
        /// </summary>
        bool IsSelected() const { return GetValue(IsSelectedProperty); }

        /// <summary>
        /// Sets whether the bookmark is selected.
        /// </summary>
        void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the bookmark color.
        /// </summary>
        static const IntModelProperty BookmarkColorProperty;

        /// <summary>
        /// Gets the bookmark color.
        /// </summary>
        Color GetBookmarkColor() const { return Color(ra::to_unsigned(GetValue(BookmarkColorProperty))); }

        /// <summary>
        /// Sets the bookmark color.
        /// </summary>
        void SetBookmarkColor(Color value) { SetValue(BookmarkColorProperty, ra::to_signed(value.ARGB)); }

        void SetModified(bool bModified);
        bool IsModified() const noexcept { return m_bModified; }

        ra::ByteAddress nAddress = 0;
        unsigned int nBytes = 1;

    private:
        bool m_bModified = false;
    };

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    ViewModelCollection<CodeNoteViewModel>& Notes() noexcept
    {
        return m_vNotes;
    }

    /// <summary>
    /// Gets the list of bookmarks.
    /// </summary>
    const ViewModelCollection<CodeNoteViewModel>& Notes() const noexcept
    {
        return m_vNotes;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the result count.
    /// </summary>
    static const StringModelProperty ResultCountProperty;

    /// <summary>
    /// Gets the result count to display.
    /// </summary>
    const std::wstring& GetResultCount() const { return GetValue(ResultCountProperty); }


    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter value.
    /// </summary>
    static const StringModelProperty FilterValueProperty;

    /// <summary>
    /// Gets the filter value.
    /// </summary>
    const std::wstring& GetFilterValue() const { return GetValue(FilterValueProperty); }

    /// <summary>
    /// Sets the filter value.
    /// </summary>
    void SetFilterValue(const std::wstring& sValue) { SetValue(FilterValueProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not to limit results to unpublished notes.
    /// </summary>
    static const BoolModelProperty OnlyUnpublishedFilterProperty;

    /// <summary>
    /// Gets whether or not to limit results to unpublished notes.
    /// </summary>
    const bool OnlyUnpublishedFilter() const { return GetValue(OnlyUnpublishedFilterProperty); }

    /// <summary>
    /// Sets whether or not to limit results to unpublished notes.
    /// </summary>
    void SetOnlyUnpublishedFilter(bool bValue) { SetValue(OnlyUnpublishedFilterProperty, bValue); }

    void ResetFilter();

    void ApplyFilter();

    void BookmarkSelected() const;

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the current selection includes unpublished items.
    /// </summary>
    static const BoolModelProperty IsSelectionUnpublishedProperty;

    /// <summary>
    /// Gets whether or not the current selection includes unpublished items.
    /// </summary>
    const bool IsSelectionUnpublished() const { return GetValue(IsSelectionUnpublishedProperty); }

    void PublishSelected();
    void RevertSelected();

protected:
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // ra::data::context::GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnEndGameLoad() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

    // ra::ui::ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnEndViewModelCollectionUpdate() override;

private:
    void OnSelectedItemsChanged();
    void GetSelectedModifiedNoteAddresses(std::vector<ra::ByteAddress>& vAddresses);

    ViewModelCollection<CodeNoteViewModel> m_vNotes;
    size_t m_nUnfilteredNotesCount = 0U;

    gsl::index m_nSelectionStart = -1, m_nSelectionEnd = -1;
    unsigned int m_nGameId = 0U;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_CODENOTESVIEWMODEL_H
