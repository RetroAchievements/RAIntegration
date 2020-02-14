#include "CodeNotesViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty CodeNotesViewModel::ResultCountProperty("CodeNotesViewModel", "ResultCount", L"0/0");

const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::LabelProperty("CodeNoteViewModel", "Label", L"");
const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::NoteProperty("CodeNoteViewModel", "Note", L"");
const IntModelProperty CodeNotesViewModel::CodeNoteViewModel::RowColorProperty("CodeNoteViewModel", "RowColor", 0);
const BoolModelProperty CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty("CodeNoteViewModel", "IsSelected", false);

CodeNotesViewModel::CodeNotesViewModel() noexcept
{
    SetWindowTitle(L"Code Notes");

    m_vNotes.AddNotifyTarget(*this);
    AddNotifyTarget(*this);
}

void CodeNotesViewModel::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
        if (args.tNewValue)
        {
            pGameContext.AddNotifyTarget(*this);
            ResetFilter();
        }
        else
        {
            pGameContext.RemoveNotifyTarget(*this);
        }
    }
}

void CodeNotesViewModel::OnActiveGameChanged()
{
    ResetFilter();
}

void CodeNotesViewModel::ResetFilter()
{
    m_vNotes.BeginUpdate();

    gsl::index nIndex = 0;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([this, &nIndex](ra::ByteAddress nAddress, unsigned int nBytes, const std::wstring& sNote)
    {
        std::wstring sAddress;
        if (nBytes <= 4)
            sAddress = ra::Widen(ra::ByteAddressToString(nAddress));
        else
            sAddress = ra::StringPrintf(L"%s\n- %s", ra::ByteAddressToString(nAddress), ra::ByteAddressToString(nAddress + nBytes - 1));

        auto* vmNote = m_vNotes.GetItemAt(nIndex);
        if (vmNote)
        {
            vmNote->SetLabel(sAddress);
            vmNote->SetNote(sNote);
        }
        else
        {
            m_vNotes.Add(sAddress, sNote);
        }

        ++nIndex;
        return true;
    });

    for (gsl::index i = ra::to_signed(m_vNotes.Count()) - 1; i >= nIndex; --i)
        m_vNotes.RemoveAt(i);

    m_vNotes.EndUpdate();

    m_nUnfilteredNotesCount = m_vNotes.Count();
    SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_nUnfilteredNotesCount, m_nUnfilteredNotesCount));
}

void CodeNotesViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{

}


GSL_SUPPRESS_F6
void CodeNotesViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
}

void CodeNotesViewModel::ApplyFilter()
{

}

} // namespace viewmodels
} // namespace ui
} // namespace ra
