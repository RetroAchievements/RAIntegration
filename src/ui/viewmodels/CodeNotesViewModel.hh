#ifndef RA_UI_CODENOTESVIEWMODEL_H
#define RA_UI_CODENOTESVIEWMODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class CodeNotesViewModel : public WindowViewModelBase,
    protected ViewModelBase::NotifyTarget,
    protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 CodeNotesViewModel() noexcept;
    ~CodeNotesViewModel() = default;

    CodeNotesViewModel(const CodeNotesViewModel&) noexcept = delete;
    CodeNotesViewModel& operator=(const CodeNotesViewModel&) noexcept = delete;
    CodeNotesViewModel(CodeNotesViewModel&&) noexcept = delete;
    CodeNotesViewModel& operator=(CodeNotesViewModel&&) noexcept = delete;
    
    class CodeNoteViewModel : public LookupItemViewModel
    {
    public:
        CodeNoteViewModel(int nId, const std::wstring& sLabel) noexcept
            : LookupItemViewModel(nId, sLabel)
        {
        }

        CodeNoteViewModel(int nId, const std::wstring&& sLabel) noexcept
            : LookupItemViewModel(nId, sLabel)
        {
        }
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

    void ApplyFilter();

protected:

private:
    ViewModelCollection<CodeNoteViewModel> m_vNotes;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_CODENOTESVIEWMODEL_H
