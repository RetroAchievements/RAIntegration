#ifndef RA_UI_CODENOTESVIEWMODEL_H
#define RA_UI_CODENOTESVIEWMODEL_H
#pragma once

#include "data\GameContext.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class CodeNotesViewModel : public WindowViewModelBase,
    protected ViewModelBase::NotifyTarget,
    protected ViewModelCollectionBase::NotifyTarget,
    protected ra::data::GameContext::NotifyTarget
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

    void ResetFilter();

    void ApplyFilter();

protected:
    // ViewModelBase::NotifyTarget
    void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;

    // ra::data::GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

private:
    ViewModelCollection<CodeNoteViewModel> m_vNotes;
    size_t m_nUnfilteredNotesCount = 0U;

    gsl::index m_nSelectionStart = -1, m_nSelectionEnd = -1;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_CODENOTESVIEWMODEL_H
