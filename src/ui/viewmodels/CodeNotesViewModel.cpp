#include "CodeNotesViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty CodeNotesViewModel::ResultCountProperty("CodeNotesViewModel", "ResultCount", L"0/0");
const StringModelProperty CodeNotesViewModel::FilterValueProperty("CodeNotesViewModel", "FilterValue", L"");

const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::LabelProperty("CodeNoteViewModel", "Label", L"");
const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::NoteProperty("CodeNoteViewModel", "Note", L"");
const BoolModelProperty CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty("CodeNoteViewModel", "IsSelected", false);
const IntModelProperty CodeNotesViewModel::CodeNoteViewModel::BookmarkColorProperty("CodeNoteViewModel", "BookmarkColor", 0);

void CodeNotesViewModel::CodeNoteViewModel::SetModified(bool bValue)
{
    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    SetBookmarkColor(bValue ? pEditorTheme.ColorModified() : pEditorTheme.ColorNormal());
}

CodeNotesViewModel::CodeNotesViewModel() noexcept
{
    SetWindowTitle(L"Code Notes");
}

void CodeNotesViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        if (args.tNewValue)
        {
            // don't subscribe until the first time the dialog is shown, but we have to keep
            // the subscription while its closed to make sure the notes stay up-to-date
            // call Remove before Add to ensure we're only subscribed once.
            pGameContext.RemoveNotifyTarget(*this);
            pGameContext.AddNotifyTarget(*this);

            if (m_nGameId != pGameContext.GameId())
                OnActiveGameChanged();
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void CodeNotesViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    m_nGameId = pGameContext.GameId();

    SetFilterValue(L"");

    if (!pGameContext.IsGameLoading())
        ResetFilter();
}

void CodeNotesViewModel::OnEndGameLoad()
{
    ResetFilter();
}

void CodeNotesViewModel::ResetFilter()
{
    m_vNotes.BeginUpdate();

    gsl::index nIndex = 0;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        pCodeNotes->EnumerateCodeNotes([this, &nIndex, pCodeNotes](ra::ByteAddress nAddress, unsigned int nBytes, const std::wstring& sNote)
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
                vmNote->SetSelected(false);
            }
            else
            {
                vmNote = &m_vNotes.Add(sAddress, sNote);
            }
            vmNote->nAddress = nAddress;
            vmNote->nBytes = nBytes;
            vmNote->SetModified(pCodeNotes->IsNoteModified(nAddress));

            ++nIndex;
            return true;
        });
    }

    for (gsl::index i = ra::to_signed(m_vNotes.Count()) - 1; i >= nIndex; --i)
        m_vNotes.RemoveAt(i);

    m_vNotes.EndUpdate();

    m_nUnfilteredNotesCount = m_vNotes.Count();
    SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_nUnfilteredNotesCount, m_nUnfilteredNotesCount));
}

void CodeNotesViewModel::ApplyFilter()
{
    const std::wstring& sFilter = GetFilterValue();
    if (sFilter.empty())
        return;

    std::wstring sFilterLower = sFilter;
    ra::StringMakeLowercase(sFilterLower);

    m_vNotes.BeginUpdate();

    for (gsl::index i = ra::to_signed(m_vNotes.Count()) - 1; i >= 0; --i)
    {
        const auto& sNote = m_vNotes.GetItemValue(i, CodeNoteViewModel::NoteProperty);
        if (!ra::StringContainsCaseInsensitive(sNote, sFilterLower, true))
            m_vNotes.RemoveAt(i);
    }

    m_vNotes.EndUpdate();

    SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_vNotes.Count(), m_nUnfilteredNotesCount));
}

void CodeNotesViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.IsGameLoading())
        return;

    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return;

    m_nUnfilteredNotesCount = pCodeNotes->CodeNoteCount();

    bool bMatchesFilter = false;
    if (!sNewNote.empty())
    {
        const std::wstring& sFilter = GetFilterValue();
        if (sFilter.empty())
            bMatchesFilter = true;
        else if (sNewNote.length() > sFilter.length())
            bMatchesFilter = ra::StringContainsCaseInsensitive(sNewNote, sFilter);
    }

    gsl::index nIndex = 0;
    for (; nIndex < ra::to_signed(m_vNotes.Count()); ++nIndex)
    {
        auto* pNote = m_vNotes.GetItemAt(nIndex);
        Ensures(pNote != nullptr);
        if (pNote->nAddress == nAddress)
        {
            if (bMatchesFilter)
            {
                /* existing note was updated */
                pNote->SetNote(sNewNote);
                pNote->SetModified(pCodeNotes->IsNoteModified(nAddress));
            }
            else if (pCodeNotes->IsNoteModified(nAddress) && GetFilterValue().empty())
            {
                /* note was deleted */
                pNote->SetNote(L"[Deleted]");
                pNote->SetModified(true);
            }
            else
            {
                /* existing note was deleted or does not match filter */
                m_vNotes.RemoveAt(nIndex);
            }

            SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_vNotes.Count(), m_nUnfilteredNotesCount));
            return;
        }
        else if (pNote->nAddress > nAddress)
        {
            break;
        }
    }

    /* non existing note - may be new, or modified to match the filter */
    if (bMatchesFilter)
    {
        m_vNotes.BeginUpdate();
        auto& pNote = m_vNotes.Add(ra::Widen(ra::ByteAddressToString(nAddress)), sNewNote);
        pNote.SetModified(pCodeNotes->IsNoteModified(nAddress));
        pNote.nAddress = nAddress;
        pNote.nBytes = 1; // this will be updated when the filter is reset
        m_vNotes.MoveItem(m_vNotes.Count() - 1, nIndex);
        m_vNotes.EndUpdate();
    }

    SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_vNotes.Count(), m_nUnfilteredNotesCount));
}

void CodeNotesViewModel::BookmarkSelected() const
{
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    vmBookmarks.Bookmarks().BeginUpdate();

    int nCount = 0;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vNotes.Count()); ++nIndex)
    {
        const auto* pNote = m_vNotes.GetItemAt(nIndex);
        Ensures(pNote != nullptr);
        if (pNote->IsSelected())
        {
            switch (pNote->nBytes)
            {
                default:
                case 1:
                    vmBookmarks.AddBookmark(pNote->nAddress, MemSize::EightBit);
                    break;

                case 2:
                    vmBookmarks.AddBookmark(pNote->nAddress, MemSize::SixteenBit);
                    break;

                case 4:
                    vmBookmarks.AddBookmark(pNote->nAddress, MemSize::ThirtyTwoBit);
                    break;
            }

            if (++nCount == 100)
                break;
        }
    }

    vmBookmarks.Bookmarks().EndUpdate();

    if (nCount == 100)
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"Can only create 100 new bookmarks at a time.");
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
