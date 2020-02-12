#include "CodeNotesViewModel.hh"

#include "RA_StringUtils.h"

#include "data\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::LabelProperty("CodeNotesViewModel", "Label", L"");
const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::NoteProperty("CodeNotesViewModel", "Note", L"");
const IntModelProperty CodeNotesViewModel::CodeNoteViewModel::RowColorProperty("CodeNotesViewModel", "RowColor", 0);
const BoolModelProperty CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty("CodeNotesViewModel", "IsSelected", false);

CodeNotesViewModel::CodeNotesViewModel() noexcept
{
    SetWindowTitle(L"Code Notes");

    m_vNotes.Add(L"0x10038", L"[32-bit] music ID");
    m_vNotes.Add(L"0x1003C", L"[32-bit] in game timer (seconds)");
    m_vNotes.Add(L"0x10069", L"C0 = combat\nE0 = non-combat");
    m_vNotes.Add(L"0x10093", L"bit4: in skystone?");
    m_vNotes.Add(L"0x100a1", L"?? boat facing");
    m_vNotes.Add(L"0x100b4", L"[16-bit] skystone parked location X");
    m_vNotes.Add(L"0x100b6", L"[16-bit] skystone parked location Y");
    m_vNotes.Add(L"0x100b8", L"bits0-5: SE red pillar\nbits6-7: SW red pillar");
    m_vNotes.Add(L"0x100b9", L"bits0-1: SW red pillar\nbits2-6: central red pillar\nbit7: NE red pillar");
    m_vNotes.Add(L"0x100ba", L"bits0-3: NE red pillar\nbits4-7: NW yellow pillar");

    m_vNotes.AddNotifyTarget(*this);
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
