#include "CodeNotesViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\AssetUploadViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty CodeNotesViewModel::ResultCountProperty("CodeNotesViewModel", "ResultCount", L"0/0");
const StringModelProperty CodeNotesViewModel::FilterValueProperty("CodeNotesViewModel", "FilterValue", L"");
const BoolModelProperty CodeNotesViewModel::OnlyUnpublishedFilterProperty("CodeNotesViewModel", "OnlyUnpublishedFilter", false);
const BoolModelProperty CodeNotesViewModel::CanRevertCurrentAddressNoteProperty("CodeNotesViewModel", "CanRevertCurrentAddressNote", false);
const BoolModelProperty CodeNotesViewModel::CanPublishCurrentAddressNoteProperty("CodeNotesViewModel", "CanPublishCurrentAddressNote", false);

const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::LabelProperty("CodeNoteViewModel", "Label", L"");
const StringModelProperty CodeNotesViewModel::CodeNoteViewModel::NoteProperty("CodeNoteViewModel", "Note", L"");
const BoolModelProperty CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty("CodeNoteViewModel", "IsSelected", false);
const IntModelProperty CodeNotesViewModel::CodeNoteViewModel::BookmarkColorProperty("CodeNoteViewModel", "BookmarkColor", 0);

void CodeNotesViewModel::CodeNoteViewModel::SetModified(bool bValue)
{
    if (m_bModified != bValue || GetBookmarkColor().ARGB == 0)
    {
        m_bModified = bValue;

        const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
        SetBookmarkColor(bValue ? pEditorTheme.ColorNoteModified() : pEditorTheme.ColorNoteNormal());
    }
}

CodeNotesViewModel::CodeNotesViewModel() noexcept
{
    SetWindowTitle(L"Code Notes");

    m_vNotes.AddNotifyTarget(*this);
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
            const auto bNoteModified = pCodeNotes->IsNoteModified(nAddress);

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
            vmNote->SetModified(bNoteModified);

            if (bNoteModified && sNote.empty())
                vmNote->SetNote(L"[Deleted]");

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
    const bool bOnlyUnpublished = OnlyUnpublishedFilter();

    const std::wstring& sFilter = GetFilterValue();
    if (sFilter.empty() && !bOnlyUnpublished)
        return;

    std::wstring sFilterLower = sFilter;
    ra::StringMakeLowercase(sFilterLower);

    m_vNotes.BeginUpdate();

    for (gsl::index i = ra::to_signed(m_vNotes.Count()) - 1; i >= 0; --i)
    {
        const auto* pNote = m_vNotes.GetItemAt(i);
        Expects(pNote != nullptr);

        if (bOnlyUnpublished && !pNote->IsModified())
            m_vNotes.RemoveAt(i);
        else if (!sFilterLower.empty() && !ra::StringContainsCaseInsensitive(pNote->GetNote(), sFilterLower, true))
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
    if (pCodeNotes == nullptr || pCodeNotes->GetIndirectSource(nAddress) != 0xFFFFFFFF)
        return;

    m_nUnfilteredNotesCount = pCodeNotes->CodeNoteCount();
    const bool bNoteModified = pCodeNotes->IsNoteModified(nAddress);

    bool bMatchesFilter = false;
    if (bNoteModified || !OnlyUnpublishedFilter())
    {
        if (!sNewNote.empty())
        {
            const std::wstring& sFilter = GetFilterValue();
            if (sFilter.empty())
                bMatchesFilter = true;
            else if (sNewNote.length() > sFilter.length())
                bMatchesFilter = ra::StringContainsCaseInsensitive(sNewNote, sFilter);
        }
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
            }
            else if (bNoteModified && m_nUnfilteredNotesCount == m_vNotes.Count())
            {
                /* note was deleted and no filter applied */
                pNote->SetNote(L"[Deleted]");
            }
            else
            {
                /* existing note was deleted or does not match filter */
                m_vNotes.RemoveAt(nIndex);
                pNote = nullptr;
            }

            if (pNote != nullptr)
            {
                pNote->SetModified(bNoteModified);

                pNote->nBytes = pCodeNotes->GetCodeNoteBytes(nAddress);
                std::wstring sAddress;
                if (pNote->nBytes <= 4)
                    sAddress = ra::Widen(ra::ByteAddressToString(nAddress));
                else
                    sAddress = ra::StringPrintf(L"%s\n- %s",
                        ra::ByteAddressToString(nAddress), ra::ByteAddressToString(nAddress + pNote->nBytes - 1));

                pNote->SetLabel(sAddress);
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
        pNote.SetModified(bNoteModified);
        pNote.nAddress = nAddress;
        pNote.nBytes = 1; // this will be updated when the filter is reset
        m_vNotes.MoveItem(m_vNotes.Count() - 1, nIndex);
        m_vNotes.EndUpdate();
    }

    SetValue(ResultCountProperty, ra::StringPrintf(L"%u/%u", m_vNotes.Count(), m_nUnfilteredNotesCount));
}

void CodeNotesViewModel::OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == CodeNoteViewModel::IsSelectedProperty)
    {
        if (!m_vNotes.IsUpdating())
            OnSelectedItemsChanged();
    }
}

void CodeNotesViewModel::OnEndViewModelCollectionUpdate()
{
    OnSelectedItemsChanged();
}

void CodeNotesViewModel::OnSelectedItemsChanged()
{
    bool bHasModifiedSelectedItem = false;
    for (const auto& pNote : m_vNotes)
    {
        if (pNote.IsSelected())
        {
            if (pNote.IsModified())
            {
                bHasModifiedSelectedItem = true;
                break;
            }
        }
    }

    const bool bOffline = ra::services::ServiceLocator::Get<ra::services::IConfiguration>()
        .IsFeatureEnabled(ra::services::Feature::Offline);

    SetValue(CanPublishCurrentAddressNoteProperty, bHasModifiedSelectedItem && !bOffline);
    SetValue(CanRevertCurrentAddressNoteProperty, bHasModifiedSelectedItem);
}

void CodeNotesViewModel::BookmarkSelected() const
{
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    vmBookmarks.Bookmarks().Items().BeginUpdate();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();

    int nCount = 0;
    for (const auto& pNote : m_vNotes)
    {
        if (pNote.IsSelected())
        {
            MemSize nSize = MemSize::Unknown;

            const auto* pCodeNote = pCodeNotes ? pCodeNotes->FindCodeNoteModel(pNote.nAddress, false) : nullptr;
            if (pCodeNote != nullptr)
            {
                nSize = pCodeNote->GetMemSize();
                if (vmBookmarks.Bookmarks().Sizes().FindItemIndex(LookupItemViewModel::IdProperty, ra::etoi(nSize)) == -1)
                {
                    // size not supported by viewer
                    nSize = MemSize::Unknown;
                }
            }

            if (nSize == MemSize::Unknown)
            {
                switch (pNote.nBytes)
                {
                    default:
                    case 1:
                        nSize = MemSize::EightBit;
                        break;

                    case 2:
                        nSize = MemSize::SixteenBit;
                        break;

                    case 4:
                        nSize = MemSize::ThirtyTwoBit;
                        break;
                }
            }

            vmBookmarks.AddBookmark(pNote.nAddress, nSize);

            if (++nCount == 100)
                break;
        }
    }

    vmBookmarks.Bookmarks().Items().EndUpdate();

    if (nCount == 100)
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"Can only create 100 new bookmarks at a time.");
}

void CodeNotesViewModel::GetSelectedModifiedNoteAddresses(std::vector<ra::ByteAddress>& vAddresses)
{
    for (const auto& pNote : m_vNotes)
    {
        if (pNote.IsSelected() && pNote.IsModified())
            vAddresses.push_back(pNote.nAddress);
    }
}

void CodeNotesViewModel::PublishSelected()
{
    if (!CanPublishCurrentAddressNote())
        return;

    std::vector<ra::ByteAddress> vNotesToPublish;
    GetSelectedModifiedNoteAddresses(vNotesToPublish);

    if (vNotesToPublish.size() == 0)
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return;

    if (vNotesToPublish.size() > 1)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
        vmPrompt.SetHeader(ra::StringPrintf(L"Publish %zu notes?", vNotesToPublish.size()));
        vmPrompt.SetMessage(L"The selected modified notes will be uploaded to the server.");
        vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmPrompt.ShowModal() == DialogResult::No)
            return;
    }

    ra::ui::viewmodels::AssetUploadViewModel vmAssetUpload;

    for (const auto nAddress : vNotesToPublish)
        vmAssetUpload.QueueCodeNote(*pCodeNotes, nAddress);

    vmAssetUpload.ShowModal(*this);
    if (vNotesToPublish.size() > 1 || vmAssetUpload.HasFailures())
        vmAssetUpload.ShowResults();

    std::vector<ra::data::models::AssetModelBase*> vAssets;
    vAssets.push_back(pCodeNotes);
    pGameContext.Assets().SaveAssets(vAssets);

    OnSelectedItemsChanged();
}

void CodeNotesViewModel::RevertSelected()
{
    std::vector<ra::ByteAddress> vNotesToRevert;
    GetSelectedModifiedNoteAddresses(vNotesToRevert);

    if (vNotesToRevert.size() == 0)
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return;

    ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
    if (vNotesToRevert.size() == 1)
        vmPrompt.SetHeader(ra::StringPrintf(L"Revert note for address %s?", ra::ByteAddressToString(vNotesToRevert.at(0))));
    else
        vmPrompt.SetHeader(ra::StringPrintf(L"Revert %zu notes?", vNotesToRevert.size()));

    vmPrompt.SetMessage(L"This will discard all local work and revert the notes to the last state retrieved from the server.");
    vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    if (vmPrompt.ShowModal() == DialogResult::No)
        return;

    for (const auto nAddress : vNotesToRevert)
    {
        const auto* pOriginalNote = pCodeNotes->GetServerCodeNote(nAddress);
        if (pOriginalNote)
        {
            // make a copy as the original note will be destroyed when we eliminate the modification
            const std::wstring pOriginalNoteCopy = *pOriginalNote;
            pCodeNotes->SetCodeNote(nAddress, pOriginalNoteCopy);
        }
    }

    std::vector<ra::data::models::AssetModelBase*> vAssets;
    vAssets.push_back(pCodeNotes);
    pGameContext.Assets().SaveAssets(vAssets);

    OnSelectedItemsChanged();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
