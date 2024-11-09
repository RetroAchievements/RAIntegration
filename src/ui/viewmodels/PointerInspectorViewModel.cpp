#include "PointerInspectorViewModel.hh"

#include "RA_Defs.h"

#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PointerInspectorViewModel::CurrentAddressProperty("PointerInspectorViewModel", "CurrentAddress", 0);
const StringModelProperty PointerInspectorViewModel::CurrentAddressTextProperty("PointerInspectorViewModel", "CurrentAddressText", L"0x0000");
const StringModelProperty PointerInspectorViewModel::CurrentAddressNoteProperty("PointerInspectorViewModel", "CurrentAddressNote", L"");
const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::OffsetProperty("StructFieldViewModel", "Offset", L"+0000");

PointerInspectorViewModel::PointerInspectorViewModel()
    : MemoryBookmarksViewModel()
{
    SetWindowTitle(L"Pointer Inspector");
}

void PointerInspectorViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentAddressProperty && !m_bSyncingAddress)
    {
        const auto nAddress = static_cast<ra::ByteAddress>(args.tNewValue);

        m_bSyncingAddress = true;
        SetCurrentAddressText(ra::Widen(ra::ByteAddressToString(nAddress)));
        m_bSyncingAddress = false;

        OnCurrentAddressChanged(nAddress);
    }

    MemoryBookmarksViewModel::OnValueChanged(args);
}

void PointerInspectorViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentAddressTextProperty && !m_bSyncingAddress)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(args.tNewValue));

        // ignore change event for current address so text field is not modified
        m_bSyncingAddress = true;
        SetCurrentAddress(nAddress);
        m_bSyncingAddress = false;

        OnCurrentAddressChanged(nAddress);
    }

    MemoryBookmarksViewModel::OnValueChanged(args);
}

void PointerInspectorViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GameId() == 0)
        SetCurrentAddress(0);
    else
        OnCurrentAddressChanged(GetCurrentAddress());
}

void PointerInspectorViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring&)
{
    if (nAddress == GetCurrentAddress())
        OnCurrentAddressChanged(nAddress); // not really, but causes the note to be reloaded
}

void PointerInspectorViewModel::OnCurrentAddressChanged(ra::ByteAddress nNewAddress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        const auto* pNote = pCodeNotes->FindCodeNoteModel(nNewAddress);
        if (pNote)
            LoadNote(pNote);
        else
            LoadNote(nullptr);
    }
}

void PointerInspectorViewModel::SyncField(PointerInspectorViewModel::StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote)
{
    pFieldViewModel.SetSize(pOffsetNote.GetMemSize());
    pFieldViewModel.SetDescription(pOffsetNote.GetNote());
}

void PointerInspectorViewModel::LoadNote(const ra::data::models::CodeNoteModel* pNote)
{
    if (pNote == nullptr)
    {
        SetCurrentAddressNote(L"");
        m_nPointerSize = MemSize::Unknown;
        Bookmarks().Clear();
        return;
    }

    SetCurrentAddressNote(pNote->GetPrimaryNote());
    m_nPointerSize = pNote->GetMemSize();

    const auto nBaseAddress = pNote->GetPointerAddress();
    gsl::index nCount = gsl::narrow_cast<gsl::index>(Fields().Count());

    gsl::index nInsertIndex = 0;
    Fields().BeginUpdate();
    pNote->EnumeratePointerNotes([this, &nCount, &nInsertIndex, nBaseAddress]
        (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote)
        {
            const auto nOffset = nAddress - nBaseAddress;
            const std::wstring sOffset = ra::StringPrintf(L"+%04x", nOffset);

            bool bFound = false;
            for (gsl::index nExistingIndex = nInsertIndex; nExistingIndex < nCount; ++nExistingIndex)
            {
                if (Fields().GetItemValue(nExistingIndex, StructFieldViewModel::OffsetProperty) == sOffset)
                {
                    // item exists. update it.
                    auto* pItem = Fields().GetItemAt(nExistingIndex);
                    Expects(pItem != nullptr);
                    SyncField(*pItem, pOffsetNote);
                    Fields().MoveItem(nExistingIndex, nInsertIndex);
                    bFound = true;
                }
            }

            if (!bFound)
            {
                // item doesn't exist. add it.
                auto& pItem = Fields().Add();
                pItem.m_nOffset = nOffset;
                pItem.SetOffset(sOffset);
                SyncField(pItem, pOffsetNote);

                Fields().MoveItem(nCount, nInsertIndex);
                ++nCount;
            }

            ++nInsertIndex;
            return true;
        });

    while (nCount > nInsertIndex)
        Fields().RemoveAt(--nCount);

    UpdateValues();
    Fields().EndUpdate();
}

void PointerInspectorViewModel::UpdateValues()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    const auto nBaseAddress = pEmulatorContext.ReadMemory(GetCurrentAddress(), m_nPointerSize);

    pEmulatorContext.RemoveNotifyTarget(*this);

    const auto nCount = gsl::narrow_cast<gsl::index>(Fields().Count());
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pField = Fields().GetItemAt(nIndex);
        if (pField != nullptr)
        {
            pField->SetAddress(nBaseAddress + pField->m_nOffset);
            UpdateBookmark(*pField, pEmulatorContext);
        }
    }

    pEmulatorContext.AddNotifyTarget(*this);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
