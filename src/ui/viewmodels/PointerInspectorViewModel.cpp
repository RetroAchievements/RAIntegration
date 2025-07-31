#include "PointerInspectorViewModel.hh"

#include "RA_Defs.h"

#include "data\context\ConsoleContext.hh"

#include "services\AchievementLogicSerializer.hh"
#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PointerInspectorViewModel::CurrentAddressProperty("PointerInspectorViewModel", "CurrentAddress", 0);
const StringModelProperty PointerInspectorViewModel::CurrentAddressTextProperty("PointerInspectorViewModel", "CurrentAddressText", L"0x0000");
const StringModelProperty PointerInspectorViewModel::CurrentAddressNoteProperty("PointerInspectorViewModel", "CurrentAddressNote", L"");
const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::OffsetProperty("StructFieldViewModel", "Offset", L"+0000");
const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::BodyProperty("StructFieldViewModel", "Body", L"");
const IntModelProperty PointerInspectorViewModel::SelectedNodeProperty("PointerInspectorViewModel", "SelectedNode", -2);
const StringModelProperty PointerInspectorViewModel::CurrentFieldNoteProperty("PointerInspectorViewModel", "CurrentFieldNote", L"");

PointerInspectorViewModel::PointerInspectorViewModel()
{
    SetWindowTitle(L"Pointer Inspector");

    m_vmFields.AddNotifyTarget(*this); // for properties on the field list
    m_vmFields.Items().AddNotifyTarget(*this); // for properties on individual fields
}

void PointerInspectorViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    // don't let the MemoryWatchListViewModel sync notes. if the watched pointer changes, it
    // will cause all the fields to blank out before they get resynced to the new addresses.
    // instead, we'll update all the fields if we detect the parent node changing.
    m_vmFields.InitializeNotifyTargets(false);
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

    WindowViewModelBase::OnValueChanged(args);
}

void PointerInspectorViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentFieldNoteProperty && !m_bSyncingNote)
    {
        UpdateSourceCodeNote();
    }
    else if (args.Property == CurrentAddressNoteProperty && !m_bSyncingNote)
    {
        UpdateSourceCodeNote();
    }
    else if (args.Property == CurrentAddressTextProperty && !m_bSyncingAddress)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(args.tNewValue));

        // ignore change event for current address so text field is not modified
        m_bSyncingAddress = true;
        SetCurrentAddress(nAddress);
        m_bSyncingAddress = false;

        OnCurrentAddressChanged(nAddress);
    }

    WindowViewModelBase::OnValueChanged(args);
}

void PointerInspectorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryWatchListViewModel::SingleSelectionIndexProperty)
        OnSelectedFieldChanged(args.tNewValue);
}

void PointerInspectorViewModel::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == StructFieldViewModel::OffsetProperty)
        OnOffsetChanged(nIndex, args.tNewValue);
}

void PointerInspectorViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    ra::ByteAddress nAddress = 0;

    if (pGameContext.GameId() != 0)
    {
        if (m_vPointers.Count() > 0)
            nAddress = gsl::narrow_cast<ra::ByteAddress>(m_vPointers.GetItemAt(0)->GetId());
    }

    if (nAddress == 0)
    {
        m_vPointers.Clear();
        m_vNodes.Clear();
        m_vPointerChain.Clear();
        m_vmFields.Items().Clear();

        m_bSyncingAddress = true;
        SetCurrentAddress(0);
        m_bSyncingAddress = false;

        m_bSyncingNote = true;
        SetCurrentAddressNote(L"");
        m_bSyncingNote = false;

        m_bSyncingNote = true;
        SetCurrentFieldNote(L"");
        m_bSyncingNote = false;
    }
    else
    {
        OnCurrentAddressChanged(nAddress);
    }
}

void PointerInspectorViewModel::OnEndGameLoad()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    m_vPointers.BeginUpdate();
    m_vPointers.Clear();

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        pCodeNotes->EnumerateCodeNotes(
            [this, &pEmulatorContext](ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pNote) {
                if (pNote.IsPointer())
                {
                    m_vPointers.Add(nAddress,
                                    ra::StringPrintf(L"%s | %s", pEmulatorContext.FormatAddress(nAddress),
                                                     ra::data::models::CodeNoteModel::TrimSize(
                                                         pNote.GetPointerDescription(), false)));
                }

                return true;
            });
    }

    m_vPointers.EndUpdate();
}

void PointerInspectorViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring&)
{
    if (nAddress == GetCurrentAddress() && !m_bSyncingNote)
    {
        m_bSyncingAddress = true;
        OnCurrentAddressChanged(nAddress); // not really, but causes the note to be reloaded
        m_bSyncingAddress = false;
    }

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        const auto* pNote = pCodeNotes->FindCodeNoteModel(nAddress, false);
        UpdatePointerVisibility(nAddress, pNote);
    }
}

void PointerInspectorViewModel::UpdatePointerVisibility(ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel* pNote)
{
    const bool bIsPointerNote = pNote && pNote->IsPointer();

    const gsl::index nCount = gsl::narrow_cast<gsl::index>(m_vPointers.Count());
    gsl::index nIndex = 0;
    while (nIndex < nCount)
    {
        const auto nPointerAddress = gsl::narrow_cast<ra::ByteAddress>(m_vPointers.GetItemValue(nIndex, LookupItemViewModel::IdProperty));
        if (nPointerAddress == nAddress)
        {
            if (!bIsPointerNote)
            {
                // non-pointer note found, remove it
                m_vPointers.RemoveAt(nIndex);
            }

            // address found, we're done
            return;
        }

        if (nPointerAddress > nAddress)
        {
            // note should go here
            break;
        }

        ++nIndex;
    }

    if (bIsPointerNote)
    {
        // valid pointer note not found, insert it
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        m_vPointers.BeginUpdate();
        m_vPointers.Add(nAddress,
                        ra::StringPrintf(L"%s | %s", pEmulatorContext.FormatAddress(nAddress),
                             ra::data::models::CodeNoteModel::TrimSize(pNote->GetPointerDescription(), false)));
        m_vPointers.MoveItem(nCount, nIndex);
        m_vPointers.EndUpdate();
    }
}

void PointerInspectorViewModel::OnCurrentAddressChanged(ra::ByteAddress nNewAddress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        // select an invalid node to force LoadNodes to select the new root node after it's been updated
        SetSelectedNode(-2);

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

        UpdatePointerChain(GetSelectedNode());
    }
}

void PointerInspectorViewModel::GetPointerChain(gsl::index nIndex, std::stack<const PointerNodeViewModel*>& sChain) const
{
    const auto* pNode = m_vNodes.GetItemAt<PointerNodeViewModel>(nIndex);
    Expects(pNode != nullptr);

    while (!pNode->IsRootNode())
    {
        sChain.push(pNode);
        pNode = m_vNodes.GetItemAt<PointerNodeViewModel>(pNode->GetParentIndex());
        Expects(pNode != nullptr);
    }

    sChain.push(pNode);
}

const ra::data::models::CodeNoteModel* PointerInspectorViewModel::FindNestedCodeNoteModel(
    const ra::data::models::CodeNoteModel& pRootNote, int nNewNode)
{
    const auto nIndex = m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, nNewNode);
    if (nIndex == -1)
        return nullptr;

    std::stack<const PointerNodeViewModel*> sChain;
    GetPointerChain(nIndex, sChain);

    const auto* pParentNote = &pRootNote;
    do
    {
        const auto* pNestedNode = sChain.top();
        Expects(pNestedNode != nullptr);
        const auto nOffset = pNestedNode->GetOffset();

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
    DispatchMemoryRead([this, nNewNode]() {
        const auto* pNote = UpdatePointerChain(nNewNode);
        LoadNote(pNote);
    });
}

void PointerInspectorViewModel::OnSelectedFieldChanged(int nNewFieldIndex)
{
    m_bSyncingNote = true;
    if (nNewFieldIndex != -1)
    {
        const auto* pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNewFieldIndex);
        Expects(pField != nullptr);
        SetCurrentFieldNote(pField->GetBody());
    }
    else
    {
        SetCurrentFieldNote(L"");
    }
    m_bSyncingNote = false;
}

const ra::data::models::CodeNoteModel* PointerInspectorViewModel::UpdatePointerChain(int nNewNode)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    const auto nCurrentAddress = GetCurrentAddress();
    const auto* pNote = pCodeNotes ? pCodeNotes->FindCodeNoteModel(nCurrentAddress) : nullptr;

    auto nNewNodeIndex = Nodes().FindItemIndex(PointerNodeViewModel::IdProperty, nNewNode);
    if (nNewNodeIndex == -1)
    {
        if (Nodes().Count() == 0)
        {
            PointerChain().Clear();
            return nullptr;
        }

        nNewNodeIndex = 0;
    }

    std::stack<const PointerNodeViewModel*> sChain;
    GetPointerChain(nNewNodeIndex, sChain);

    gsl::index nCount = gsl::narrow_cast<gsl::index>(PointerChain().Count());
    gsl::index nInsertIndex = 0;
    PointerChain().BeginUpdate();

    do
    {
        StructFieldViewModel* pItem = nullptr;
        if (nInsertIndex < nCount)
        {
            pItem = PointerChain().GetItemAt(nInsertIndex);
        }
        else
        {
            ++nCount;
            pItem = &PointerChain().Add();
        }

        Expects(pItem != nullptr);
        pItem->BeginInitialization();

        const auto* pNode = sChain.top();
        Expects(pNode != nullptr);
        if (pNode->IsRootNode())
        {
            pItem->m_nOffset = nCurrentAddress;
            pItem->m_nIndent = 0;
            pItem->SetOffset(L"");
        }
        else
        {
            pItem->m_nOffset = pNode->GetOffset();
            pItem->m_nIndent = gsl::narrow_cast<int32_t>(nInsertIndex) - 1;
            pItem->FormatOffset();

            pNote = pNote ? pNote->GetPointerNoteAtOffset(pItem->m_nOffset) : nullptr;
        }

        if (pNote)
        {
            pItem->SetRealNote(pNote->GetPointerDescription());
            pItem->SetSize(pNote->GetMemSize());
        }
        else
        {
            pItem->SetRealNote(L"");
            pItem->SetSize(MemSize::EightBit);
        }

        // EndInitialization does memory reads, so it must be dispatched. we'll do it in a bit

        ++nInsertIndex;

        sChain.pop();
    } while (!sChain.empty());

    while (nCount > nInsertIndex)
        PointerChain().RemoveAt(--nCount);

    DispatchMemoryRead([this]() {
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(PointerChain().Count()); i++)
            PointerChain().GetItemAt(i)->EndInitialization();

        UpdatePointerChainValues();
    });

    PointerChain().EndUpdate();

    return pNote;
}

void PointerInspectorViewModel::SyncField(PointerInspectorViewModel::StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote)
{
    pFieldViewModel.m_pNote = &pOffsetNote;

    pFieldViewModel.SetFormat(pOffsetNote.GetDefaultMemFormat());

    const auto nSize = pOffsetNote.GetMemSize();
    pFieldViewModel.SetSize((nSize == MemSize::Unknown) ? MemSize::EightBit : nSize);

    const auto& sNote = pOffsetNote.IsPointer() ? pOffsetNote.GetPointerDescription() : pOffsetNote.GetPrimaryNote();
    pFieldViewModel.SetBody(sNote);

    auto nIndex = sNote.find('\n');
    if (nIndex == std::string::npos)
    {
        pFieldViewModel.SetRealNote(ra::data::models::CodeNoteModel::TrimSize(sNote, true));
    }
    else
    {
        if (nIndex > 0 && sNote.at(nIndex - 1) == '\r')
            --nIndex;

        pFieldViewModel.SetRealNote(ra::data::models::CodeNoteModel::TrimSize(sNote.substr(0, nIndex), true));
    }
}

void PointerInspectorViewModel::LoadNote(const ra::data::models::CodeNoteModel* pNote)
{
    if (pNote == nullptr)
    {
        SetCurrentAddressNote(L"");
        m_pCurrentNote = nullptr;
        m_vmFields.Items().Clear();
        return;
    }

    m_bSyncingNote = true;
    SetCurrentAddressNote(pNote->GetPrimaryNote());
    m_bSyncingNote = false;

    m_pCurrentNote = pNote;
    const auto nBaseAddress = m_pCurrentNote->GetPointerAddress();
    gsl::index nCount = gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count());

    gsl::index nInsertIndex = 0;
    m_vmFields.Items().BeginUpdate();
    pNote->EnumeratePointerNotes([this, &nCount, &nInsertIndex, nBaseAddress]
        (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote)
        {
            const auto nOffset = nAddress - nBaseAddress;
            const std::wstring sOffset = ra::StringPrintf(L"+%04x", nOffset);

            StructFieldViewModel* pItem = nullptr;
            if (nInsertIndex < nCount)
            {
                pItem = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nInsertIndex);
                Expects(pItem != nullptr);
                pItem->SetSelected(false);
            }
            else
            {
                ++nCount;
                pItem = &m_vmFields.Items().Add<StructFieldViewModel>();
            }

            pItem->BeginInitialization();

            pItem->m_nOffset = nOffset;
            pItem->SetOffset(sOffset);
            SyncField(*pItem, pOffsetNote);

            // EndInitialization does memory reads, so it must be dispatched. we'll do it in a bit

            ++nInsertIndex;
            return true;
        });

    while (nCount > nInsertIndex)
        m_vmFields.Items().RemoveAt(--nCount);

    DispatchMemoryRead([this]() {
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count()); i++)
            m_vmFields.Items().GetItemAt(i)->EndInitialization();

        UpdateValues();
    });

    m_vmFields.Items().EndUpdate();

    OnSelectedFieldChanged(m_vmFields.GetSingleSelectionIndex());
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
            sLabel = std::wstring(gsl::narrow_cast<size_t>(nDepth) - 1, ' ');
        sLabel += ra::StringPrintf(L"+%04x | %s", nOffset, pOffsetNote.GetPointerDescription());

        vNodes.Add<PointerInspectorViewModel::PointerNodeViewModel>(nParentIndex, nOffset, sLabel);

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
        m_vNodes.Add<PointerNodeViewModel>(-1, ra::to_signed(GetCurrentAddress()), L"Root");
    }
    else
    {
        m_vNodes.Add<PointerNodeViewModel>(-1, ra::to_signed(GetCurrentAddress()), pNote->GetPrimaryNote());
        if (pNote->HasNestedPointers())
            LoadSubNotes(m_vNodes, *pNote, pNote->GetPointerAddress(), 1, 0);
    }

    m_vNodes.EndUpdate();

    // if the selected item is no longer available, select the root node
    if (m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, nSelectedNode) == -1)
        SetSelectedNode(PointerNodeViewModel::RootNodeId);
}

void PointerInspectorViewModel::BuildNoteForCurrentNode(ra::StringBuilder& builder,
    std::stack<const PointerInspectorViewModel::PointerNodeViewModel*>& sChain,
    gsl::index nDepth)
{
    // if a single field is selected, push the current field value back into the field
    const auto nSingleSelectionIndex = gsl::narrow_cast<gsl::index>(m_vmFields.GetSingleSelectionIndex());
    auto* pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nSingleSelectionIndex);
    if (pField)
    {
        // SyncField will cause pField to point at the local newNote, capture the current value so it can be restored
        const ra::data::models::CodeNoteModel* pExistingNote = pField->m_pNote;
        ra::data::models::CodeNoteModel newNote;
        newNote.SetNote(GetCurrentFieldNote());
        SyncField(*pField, newNote);
        pField->m_pNote = pExistingNote;
    }

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count()); ++nIndex)
    {
        pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nIndex);
        if (!pField)
            continue;

        builder.Append(L"\r\n");
        builder.Append(std::wstring(nDepth, '+'));
        builder.Append(ra::StringPrintf(L"0x%02X: ", pField->m_nOffset));

        if (pField->GetSize() != MemSize::Unknown)
        {
            builder.Append('[');
            builder.Append(ra::data::MemSizeString(pField->GetSize()));

            if (pField->m_pNote->IsPointer())
                builder.Append(L" pointer] ");
            else
                builder.Append(L"] ");
        }

        builder.Append(pField->GetDescription());

        const auto& sBody = pField->GetBody();
        const auto nLineIndex = sBody.find('\n');
        if (nLineIndex != std::string::npos)
        {
            builder.Append(L"\r\n");
            builder.Append(sBody.substr(nLineIndex + 1));
        }

        if (pField->m_pNote->IsPointer())
            BuildNote(builder, sChain, nDepth + 1, *pField->m_pNote);
    }
}

void PointerInspectorViewModel::BuildNote(ra::StringBuilder& builder,
    std::stack<const PointerInspectorViewModel::PointerNodeViewModel*>& sChain,
    gsl::index nDepth, const ra::data::models::CodeNoteModel& pNote)
{
    if (nDepth == gsl::narrow_cast<gsl::index>(sChain.size()))
    {
        BuildNoteForCurrentNode(builder, sChain, nDepth);
        return;
    }

    const auto nBaseAddress = pNote.GetPointerAddress();

    pNote.EnumeratePointerNotes([this, &builder, &sChain, nDepth, nBaseAddress]
        (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote) {
            const auto nOffset = nAddress - nBaseAddress;

            builder.Append(L"\r\n");
            builder.Append(std::wstring(nDepth, '+'));
            builder.Append(ra::StringPrintf(L"0x%02X: ", nOffset));

            if (pOffsetNote.GetMemSize() != MemSize::Unknown)
            {
                builder.Append('[');
                builder.Append(ra::data::MemSizeString(pOffsetNote.GetMemSize()));

                if (pOffsetNote.IsPointer())
                    builder.Append(L" pointer] ");
                else
                    builder.Append(L"] ");
            }

            if (!pOffsetNote.IsPointer())
                builder.Append(ra::data::models::CodeNoteModel::TrimSize(pOffsetNote.GetNote(), false));
            else if (&pOffsetNote == m_pCurrentNote)
                builder.Append(ra::data::models::CodeNoteModel::TrimSize(GetCurrentAddressNote(), false));
            else
                builder.Append(ra::data::models::CodeNoteModel::TrimSize(pOffsetNote.GetPointerDescription(), false));

            if (pOffsetNote.IsPointer())
                BuildNote(builder, sChain, nDepth + 1, pOffsetNote);

            return true;
        });
}

void PointerInspectorViewModel::UpdateSourceCodeNote()
{
    const auto nAddress = GetCurrentAddress();
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    Expects(pCodeNotes != nullptr);
    auto* pNote = pCodeNotes->FindCodeNoteModel(nAddress);
    if (pNote != nullptr)
    {
        const auto nIndex = m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, GetSelectedNode());
        if (nIndex != -1)
        {
            std::stack<const PointerNodeViewModel*> sChain;
            GetPointerChain(nIndex, sChain);

            ra::StringBuilder builder;

            if (pNote == m_pCurrentNote)
                builder.Append(GetCurrentAddressNote());
            else
                builder.Append(pNote->GetPointerDescription());

            BuildNote(builder, sChain, 1, *pNote);

            // nested note reference is invalidated in SetCodeNote. temporarily release
            // the reference and restore it after SetCodeNote completes to prevent the UI
            // thread from trying to use it while it's invalid (we're most likely on a
            // background thread kicked off by a timer to support as-you-type changes).
            m_pCurrentNote = nullptr;

            m_bSyncingNote = true;
            pCodeNotes->SetCodeNote(nAddress, builder.ToWString());
            m_bSyncingNote = false;

            // update the current note pointer
            m_pCurrentNote = UpdatePointerChain(GetSelectedNode());

            // update the field note pointers
            gsl::index nNoteIndex = 0;
            m_pCurrentNote->EnumeratePointerNotes([this, &nNoteIndex]
                (ra::ByteAddress, const ra::data::models::CodeNoteModel& pOffsetNote)
                {
                    m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNoteIndex++)->m_pNote = &pOffsetNote;
                    return true;
                });
            for (; nNoteIndex < gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count()); ++nNoteIndex)
                m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNoteIndex)->m_pNote = nullptr;
        }
    }
}

void PointerInspectorViewModel::UpdatePointerChainValues()
{
    ra::ByteAddress nAddress = 0;
    const auto nCount = gsl::narrow_cast<gsl::index>(PointerChain().Count());

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();

    PointerChain().BeginUpdate();

    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pPointer = PointerChain().GetItemAt(nIndex);
        if (pPointer != nullptr)
        {
            nAddress += pPointer->m_nOffset;
            pPointer->SetAddressWithoutUpdatingValue(nAddress);

            if (pPointer->DoFrame())
                UpdatePointerChainRowColor(*pPointer);

            nAddress = pPointer->GetCurrentValueRaw();
            if (nAddress > 0)
            {
                const auto nAdjustedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
                if (nAdjustedAddress != 0xFFFFFFFF)
                    nAddress = nAdjustedAddress;
            }
        }
    }

    PointerChain().EndUpdate();
}

void PointerInspectorViewModel::UpdatePointerChainRowColor(PointerInspectorViewModel::StructFieldViewModel& pPointer)
{
    const auto nPointerValue = pPointer.GetCurrentValueRaw();
    if (nPointerValue == 0)
    {
        // pointer is null
        pPointer.SetRowColor(ra::ui::Color(0xFFFFC0C0));
        return;
    }

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    bool bValid = false;

    MemSize nMemSize = MemSize::Unknown;
    uint32_t nMask = 0xFFFFFFFF;
    uint32_t nOffset = 0;
    if (pConsoleContext.GetRealAddressConversion(&nMemSize, &nMask, &nOffset))
    {
        if (nMemSize == MemSize::TwentyFourBit)
            nMemSize = MemSize::ThirtyTwoBit;

        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        const auto nRawPointer = pEmulatorContext.ReadMemory(pPointer.GetAddress(), nMemSize);
        bValid = (pConsoleContext.ByteAddressFromRealAddress(nRawPointer) != 0xFFFFFFFF);
    }
    else
    {
        bValid = (nPointerValue <= pConsoleContext.MaxAddress());
    }

    if (!bValid)
    {
        // pointer is invalid
        pPointer.SetRowColor(ra::ui::Color(0xFFFFFFC0));
    }
    else
    {
        // pointer is valid
        pPointer.SetRowColor(
            ra::ui::Color(ra::to_unsigned(MemoryWatchViewModel::RowColorProperty.GetDefaultValue())));
    }
}

void PointerInspectorViewModel::UpdateValues()
{
    const auto nBaseAddress = (m_pCurrentNote != nullptr) ? m_pCurrentNote->GetPointerAddress() : 0U;

    m_vmFields.Items().BeginUpdate();

    const auto nCount = gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count());
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nIndex);
        if (pField != nullptr)
        {
            pField->SetAddress(nBaseAddress + pField->m_nOffset);
            pField->DoFrame();
        }
    }

    m_vmFields.Items().EndUpdate();
}

void PointerInspectorViewModel::DoFrame()
{
    UpdatePointerChainValues();
    UpdateValues();
}

void PointerInspectorViewModel::CopyDefinition() const
{
    const auto sDefinition = GetMemRefChain(false);
    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sDefinition));
}

class VirtualCodeNoteModel : public ra::data::models::CodeNoteModel
{
    bool GetPointerChain(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pRootNote) const override
    {
        if (!ra::data::models::CodeNoteModel::GetPointerChain(vChain, pRootNote))
            return false;

        vChain.push_back(this);
        return true;
    }
};

std::string PointerInspectorViewModel::GetMemRefChain(bool bMeasured) const
{
    std::string sBuffer;

    const auto nSelectedFieldIndex = m_vmFields.Items().FindItemIndex(LookupItemViewModel::IsSelectedProperty, true);
    if (nSelectedFieldIndex == -1)
        return sBuffer;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return sBuffer;

    const auto* vmField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nSelectedFieldIndex);
    Expects(vmField != nullptr);

    auto* pRootNote = pCodeNotes->FindCodeNoteModel(GetCurrentAddress());
    Expects(pRootNote != nullptr);

    VirtualCodeNoteModel oLeafNote;
    auto* pLeafNote = m_pCurrentNote->GetPointerNoteAtOffset(vmField->m_nOffset);
    if (pLeafNote == nullptr)
    {
        oLeafNote.SetMemSize(vmField->GetSize());
        oLeafNote.SetAddress(vmField->m_nOffset);
        pLeafNote = &oLeafNote;
    }

    sBuffer = ra::services::AchievementLogicSerializer::BuildMemRefChain(*pRootNote, *pLeafNote);

    if (bMeasured) // BuildMemRefChain returns a Measured value. If that's what's wanted, return it.
        return sBuffer;

    // remove Measured flag
    const auto nIndex = sBuffer.rfind("M:");
    if (nIndex != std::string::npos)
        sBuffer.erase(nIndex, 2);

    // add comparison to current value
    ra::services::AchievementLogicSerializer::AppendOperator(sBuffer, ra::services::TriggerOperatorType::Equals);
    ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Value,
                                                            vmField->GetSize(), vmField->GetCurrentValueRaw());

    return sBuffer;
}

void PointerInspectorViewModel::BookmarkCurrentField() const
{
    auto sDefinition = GetMemRefChain(true);

    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.MemoryBookmarks.AddBookmark(sDefinition);

    if (!pWindowManager.MemoryBookmarks.IsVisible())
        pWindowManager.MemoryBookmarks.Show();
}

void PointerInspectorViewModel::OnOffsetChanged(gsl::index nIndex, const std::wstring& sNewOffset)
{
    auto* pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nIndex);
    if (!pField || pField->m_bFormattingOffset)
        return;

    const auto nOldOffset = pField->m_nOffset;
    unsigned nOffset;
    std::wstring sError;
    if (ra::ParseHex(sNewOffset, 0xFFFFFFFF, nOffset, sError))
        pField->m_nOffset = ra::to_signed(nOffset);

    pField->FormatOffset();

    // keep the fields sorted
    if (pField->m_nOffset != nOldOffset)
    {
        auto nNewIndex = nIndex;
        while (nNewIndex > 0)
        {
            auto* pPrevField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNewIndex - 1);
            if (pPrevField->m_nOffset < pField->m_nOffset)
                break;

            nNewIndex--;
        }
        if (nNewIndex == nIndex)
        {
            while (nNewIndex < gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count()) - 1)
            {
                auto* pNextField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNewIndex + 1);
                if (pNextField->m_nOffset > pField->m_nOffset)
                    break;

                nNewIndex++;
            }
        }

        if (nNewIndex != nIndex)
        {
            m_vmFields.Items().MoveItem(nIndex, nNewIndex);

            const auto nSelectionIndex = m_vmFields.GetSingleSelectionIndex();
            if (nSelectionIndex == gsl::narrow_cast<int>(nIndex))
                m_vmFields.SetSingleSelectionIndex(gsl::narrow_cast<int>(nNewIndex));
        }

        // update the composite note
        UpdateSourceCodeNote();

        // lastly, update the current value (may trigger PauseOnChange)
        const auto nBaseAddress = (m_pCurrentNote != nullptr) ? m_pCurrentNote->GetPointerAddress() : 0U;
        pField->SetAddress(nBaseAddress + pField->m_nOffset);
        pField->DoFrame();
    }
}

void PointerInspectorViewModel::StructFieldViewModel::FormatOffset()
{
    std::wstring sLabel;
    if (m_nIndent > 0)
        sLabel = std::wstring(m_nIndent, ' ');
    sLabel += ra::StringPrintf(L"+%04x", m_nOffset);

    m_bFormattingOffset = true;
    SetOffset(sLabel);
    m_bFormattingOffset = false;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
