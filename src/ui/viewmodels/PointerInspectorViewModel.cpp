#include "PointerInspectorViewModel.hh"

#include "RA_Defs.h"

#include "data\context\ConsoleContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PointerInspectorViewModel::CurrentAddressProperty("PointerInspectorViewModel", "CurrentAddress", 0);
const StringModelProperty PointerInspectorViewModel::CurrentAddressTextProperty("PointerInspectorViewModel", "CurrentAddressText", L"0x0000");
const StringModelProperty PointerInspectorViewModel::CurrentAddressNoteProperty("PointerInspectorViewModel", "CurrentAddressNote", L"");
const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::OffsetProperty("StructFieldViewModel", "Offset", L"+0000");
const IntModelProperty PointerInspectorViewModel::SelectedNodeProperty("PointerInspectorViewModel", "SelectedNode", -2);

constexpr int RootNodeID = -1;

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
    else if (args.Property == SelectedNodeProperty && !m_bSyncingAddress)
    {
        OnSelectedNodeChanged(args.tNewValue);
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
        SetSelectedNode(RootNodeID);

        const auto* pNote = pCodeNotes->FindCodeNoteModel(nNewAddress);
        if (pNote)
        {
            LoadNodes(pNote);
            LoadNote(pNote);
        }
        else
        {
            LoadNodes(nullptr);
            LoadNote(nullptr);
        }
    }
}

void PointerInspectorViewModel::GetPointerChain(gsl::index nIndex, std::stack<const LookupItemViewModel*>& sChain) const
{
    const auto* pNode = m_vNodes.GetItemAt(nIndex);
    Expects(pNode != nullptr);

    sChain.push(pNode);

    auto nDepth = pNode->GetId() >> 24;
    while (nDepth > 0 && nIndex > 1)
    {
        const auto* pNestedNode = m_vNodes.GetItemAt(--nIndex);
        const auto nNestedDepth = pNestedNode->GetId() >> 24;
        if (nNestedDepth < nDepth)
        {
            sChain.push(pNestedNode);
            nDepth = nNestedDepth;
        }
    }
}

const ra::data::models::CodeNoteModel* PointerInspectorViewModel::FindNestedCodeNoteModel(
    const ra::data::models::CodeNoteModel& pRootNote, int nNewNode)
{
    auto nIndex = m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, nNewNode);
    if (nIndex == -1)
        return nullptr;

    std::stack<const LookupItemViewModel*> sChain;
    GetPointerChain(nIndex, sChain);

    const auto* pParentNote = &pRootNote;
    do
    {
        const auto* pNestedNode = sChain.top();
        const auto nOffset = pNestedNode->GetId() & 0x00FFFFFF;

        const auto* pNestedNote = pParentNote->GetPointerNoteAtOffset(nOffset);
        if (!pNestedNote)
            return nullptr;

        pParentNote = pNestedNote;
        sChain.pop();
    } while (!sChain.empty());

    return pParentNote;
}

void PointerInspectorViewModel::OnSelectedNodeChanged(int nNewNode)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        const auto* pNote = pCodeNotes->FindCodeNoteModel(GetCurrentAddress());
        if (pNote != nullptr && nNewNode != RootNodeID)
            pNote = FindNestedCodeNoteModel(*pNote, nNewNode);

        LoadNote(pNote);
    }
}

void PointerInspectorViewModel::SyncField(PointerInspectorViewModel::StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote)
{
    const auto nSize = pOffsetNote.GetMemSize();
    pFieldViewModel.SetSize((nSize == MemSize::Unknown) ? MemSize::EightBit : nSize);
    pFieldViewModel.SetDescription(pOffsetNote.GetPrimaryNote());
}

void PointerInspectorViewModel::LoadNote(const ra::data::models::CodeNoteModel* pNote)
{
    if (pNote == nullptr)
    {
        SetCurrentAddressNote(L"");
        m_pCurrentNote = nullptr;
        Bookmarks().Clear();
        return;
    }

    SetCurrentAddressNote(pNote->GetPrimaryNote());

    m_pCurrentNote = pNote;
    const auto nBaseAddress = m_pCurrentNote->GetPointerAddress();
    gsl::index nCount = gsl::narrow_cast<gsl::index>(Fields().Count());

    gsl::index nInsertIndex = 0;
    Fields().BeginUpdate();
    pNote->EnumeratePointerNotes([this, &nCount, &nInsertIndex, nBaseAddress]
        (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote)
        {
            const auto nOffset = nAddress - nBaseAddress;
            const std::wstring sOffset = ra::StringPrintf(L"+%04x", nOffset);

            StructFieldViewModel* pItem;
            if (nInsertIndex < nCount)
            {
                pItem = Fields().GetItemAt(nInsertIndex);
            }
            else
            {
                ++nCount;
                pItem = &Fields().Add();
            }

            pItem->m_nOffset = nOffset;
            pItem->SetOffset(sOffset);
            SyncField(*pItem, pOffsetNote);

            ++nInsertIndex;
            return true;
        });

    while (nCount > nInsertIndex)
        Fields().RemoveAt(--nCount);

    UpdateValues();
    Fields().EndUpdate();
}

static void LoadSubNotes(LookupItemViewModelCollection& vNodes,
    const ra::data::models::CodeNoteModel& pNote, ra::ByteAddress nBaseAddress, int nDepth, int nParentIndex)
{
    pNote.EnumeratePointerNotes([&vNodes, nBaseAddress, nDepth, nParentIndex]
                                (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote) {
        const auto nOffset = nAddress - nBaseAddress;
        if (!pOffsetNote.IsPointer())
            return true;

        std::wstring sLabel;
        if (nDepth > 1)
            sLabel = std::wstring(nDepth - 1, ' ');
        sLabel += ra::StringPrintf(L"+%04x | %s", nOffset, pOffsetNote.GetPointerDescription());

        vNodes.Add(nParentIndex << 24 | nOffset & 0x00FFFFFF, sLabel);

        if (pOffsetNote.HasNestedPointers())
            LoadSubNotes(vNodes, pOffsetNote, pOffsetNote.GetPointerAddress(), nDepth + 1, gsl::narrow_cast<int>(vNodes.Count() - 1));

        return true;
    });
}

void PointerInspectorViewModel::LoadNodes(const ra::data::models::CodeNoteModel* pNote)
{
    const int nSelectedNode = GetSelectedNode();

    m_vNodes.BeginUpdate();
    m_vNodes.Clear();

    if (pNote == nullptr)
    {
        m_vNodes.Add(RootNodeID, L"Root");
    }
    else
    {
        m_vNodes.Add(RootNodeID, pNote->GetPrimaryNote());
        if (pNote->HasNestedPointers())
            LoadSubNotes(m_vNodes, *pNote, pNote->GetPointerAddress(), 1, 0);
    }

    m_vNodes.EndUpdate();

    // if the selected item is no longer available, select the root node
    if (m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, nSelectedNode) == -1)
        SetSelectedNode(0);
}

void PointerInspectorViewModel::UpdateValues()
{
    const auto nBaseAddress = (m_pCurrentNote != nullptr) ? m_pCurrentNote->GetPointerAddress() : 0U;

    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.RemoveNotifyTarget(*this);

    const auto nCount = gsl::narrow_cast<gsl::index>(Fields().Count());
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pField = Fields().GetItemAt(nIndex);
        if (pField != nullptr)
        {
            pField->SetAddress(nBaseAddress + pField->m_nOffset);
            UpdateBookmark(*pField);
        }
    }

    pEmulatorContext.AddNotifyTarget(*this);
}

void PointerInspectorViewModel::DoFrame()
{
    UpdateValues();
}

void PointerInspectorViewModel::CopyDefinition() const
{
    const auto sDefinition = GetDefinition();
}

static void AppendMaskedPointer(ra::StringBuilder& builder, uint32_t nMaskBits, MemSize nSize, const std::string& sAddress)
{
    const auto nSizeBits = ra::data::MemSizeBits(nSize);
    if (nSizeBits < nMaskBits)
        nMaskBits = nSizeBits;

    const auto nBytes = (nMaskBits + 7) / 8;
    switch (nBytes)
    {
        default: builder.Append('H'); break;
        case 2:  builder.Append(' '); break;
        case 3:  builder.Append('W'); break;
        case 4:  builder.Append('X'); break;
    }

    builder.AppendSubString(sAddress.c_str() + 2, sAddress.length() - 2);

    if (nMaskBits != nBytes * 8)
    {
        builder.Append('&');
        builder.Append((1 << nMaskBits) - 1);
    }
}

static void AppendPointer(ra::StringBuilder& builder, const ra::data::models::CodeNoteModel* pNote)
{
    MemSize nSize;
    uint32_t nMask;
    uint32_t nOffset;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    if (!pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
    {
        nSize = pNote->GetMemSize();
        nMask = 0xFFFFFFFF;
        nOffset = pConsoleContext.RealAddressFromByteAddress(0);
    }

    builder.Append("I:0x");
    switch (nSize)
    {
        case MemSize::ThirtyTwoBit:          builder.Append('X'); break;
        case MemSize::TwentyFourBit:         builder.Append('W'); break;
        case MemSize::SixteenBit:            builder.Append(' '); break;
        case MemSize::EightBit:              builder.Append('H'); break;
        case MemSize::ThirtyTwoBitBigEndian: builder.Append('G'); break;
        case MemSize::SixteenBitBigEndian:   builder.Append('I'); break;
        case MemSize::TwentyFourBitBigEndian:builder.Append('J'); break;
    }

    const auto sAddress = ra::ByteAddressToString(pNote->GetPointerAddress());
    builder.Append(sAddress.c_str() + 2, sAddress.length() - 2);

    if (nOffset != 0)
    {
        builder.Append('-');
        builder.Append(nOffset);
    }
    else if (nMask != 0xFFFFFFFF && nMask != ((1 << ra::data::MemSizeBits(nSize)) - 1))
    {
        builder.Append('&');
        builder.Append(nMask);
    }
}

std::string PointerInspectorViewModel::GetDefinition() const
{
    ra::StringBuilder builder;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        const auto* pRootNote = pCodeNotes->FindCodeNoteModel(GetCurrentAddress());

        std::stack<const LookupItemViewModel*> sChain;
        GetPointerChain(GetSelectedNode(), sChain);

        const auto* pParentNote = pRootNote;
        builder.Append("A:");
        AppendPointer(builder, pRootNote);
        do
        {
            const auto* pNestedNode = sChain.top();
            const auto nOffset = pNestedNode->GetId() & 0x00FFFFFF;

            const auto* pNestedNote = pParentNote->GetPointerNoteAtOffset(nOffset);
            if (!pNestedNote)
                return builder.ToString();

            AppendPointer(builder, pNestedNote);

            pParentNote = pNestedNote;
            sChain.pop();
        } while (!sChain.empty());

        // TODO: append field
    }

    return builder.ToString();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
