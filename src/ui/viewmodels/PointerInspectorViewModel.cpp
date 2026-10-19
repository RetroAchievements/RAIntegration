#include "PointerInspectorViewModel.hh"

#include "RA_Defs.h"

#include "context\IConsoleContext.hh"
#include "context\IEmulatorMemoryContext.hh"

#include "data\models\ArrayMemoryNoteModel.hh"
#include "data\models\PointerMemoryNoteModel.hh"
#include "data\models\StructuredMemoryNoteModel.hh"
#include "data\util\AchievementLogicSerializer.hh"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

constexpr int SelectedNodeNone = -2;

const IntModelProperty PointerInspectorViewModel::SelectedRootAddressProperty("PointerInspectorViewModel", "SelectedRootAddress", 0);
const BoolModelProperty PointerInspectorViewModel::HasSelectedRootAddressProperty("PointerInspectorViewModel", "HasSelectedRootAddress", false);
const StringModelProperty PointerInspectorViewModel::SelectedNodeNoteProperty("PointerInspectorViewModel", "RootAddressNote", L"");
const IntModelProperty PointerInspectorViewModel::SelectedNodeProperty("PointerInspectorViewModel", "SelectedNode", SelectedNodeNone);
const BoolModelProperty PointerInspectorViewModel::HasSelectedNodeProperty("PointerInspectorViewModel", "HasSelectedNode", false);
const StringModelProperty PointerInspectorViewModel::CurrentFieldNoteProperty("PointerInspectorViewModel", "CurrentFieldNote", L"");

const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::OffsetProperty("StructFieldViewModel", "Offset", L"+0000");
const StringModelProperty PointerInspectorViewModel::StructFieldViewModel::BodyProperty("StructFieldViewModel", "Body", L"");

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

void PointerInspectorViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty && args.tNewValue)
    {
        DispatchMemoryRead([this]() { DoFrame(); });
    }

    WindowViewModelBase::OnValueChanged(args);
}

void PointerInspectorViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SelectedRootAddressProperty && !m_bSyncingAddress)
    {
        const auto nAddress = static_cast<ra::data::ByteAddress>(args.tNewValue);
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
        UpdateSourceMemoryNote();
    }
    else if (args.Property == SelectedNodeNoteProperty && !m_bSyncingNote)
    {
        UpdateSourceMemoryNote();
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
    if (args.Property == StructFieldViewModel::OffsetProperty && !m_bSyncingNote)
        OnFieldOffsetChanged(nIndex, args.tNewValue);
}

void PointerInspectorViewModel::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == StructFieldViewModel::SizeProperty && !m_bSyncingNote)
        OnFieldSizeChanged(nIndex);
}

void PointerInspectorViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    ra::data::ByteAddress nAddress = 0;

    if (pGameContext.GameId() != 0)
    {
        if (m_vPointers.Count() > 0)
            nAddress = gsl::narrow_cast<ra::data::ByteAddress>(m_vPointers.GetItemAt(0)->GetId());
    }

    if (nAddress == 0)
    {
        m_vPointers.Clear();
        m_vNodes.Clear();
        m_vPointerChain.Clear();
        m_vmFields.Items().Clear();
        m_pCurrentNote = nullptr;

        m_bSyncingAddress = true;
        SetSelectedRootAddress(0);
        SetValue(HasSelectedRootAddressProperty, false);
        m_bSyncingAddress = false;

        m_bSyncingNote = true;
        SetSelectedNodeNote(L"");
        m_bSyncingNote = false;

        m_bSyncingNote = true;
        SetCurrentFieldNote(L"");
        m_bSyncingNote = false;
    }
    else
    {
        m_bSyncingAddress = true;
        SetSelectedRootAddress(0xFFFFFFFF);
        m_bSyncingAddress = false;

        SetSelectedRootAddress(nAddress);
        SetValue(HasSelectedRootAddressProperty, true);
    }
}

void PointerInspectorViewModel::OnEndGameLoad()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    m_vPointers.BeginUpdate();
    m_vPointers.Clear();

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    const auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    if (pMemoryNotes != nullptr)
    {
        pMemoryNotes->EnumerateNotes(
            [this, &pMemoryContext](const ra::data::models::MemoryNoteModel::Reference& pNote) {
                if (pNote.pMemoryNote->GetType() == ra::data::models::MemoryNoteType::Pointer)
                {
                    m_vPointers.Add(pNote.nAddress,
                                    ra::util::String::Printf(L"%s | %s", pMemoryContext.FormatAddress(pNote.nAddress),
                                                                         pNote.pMemoryNote->GetSummary()));
                }

                return true;
            });

        if (m_vPointers.Count() == 0)
            m_vPointers.Add(0, L"No pointer notes found");
    }

    m_vPointers.EndUpdate();
}

void PointerInspectorViewModel::OnMemoryNoteChanged(ra::data::ByteAddress nAddress, const std::wstring&)
{
    if (nAddress == GetSelectedRootAddress() && !m_bSyncingNote)
    {
        m_bSyncingAddress = true;
        OnCurrentAddressChanged(nAddress); // not really, but causes the note to be reloaded
        m_bSyncingAddress = false;
    }

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    if (pMemoryNotes != nullptr)
    {
        const auto* pNote = pMemoryNotes->FindNote(nAddress, false);
        UpdatePointerVisibility(nAddress, pNote);
    }
}

void PointerInspectorViewModel::UpdatePointerVisibility(ra::data::ByteAddress nAddress, const ra::data::models::MemoryNoteModel* pNote)
{
    const bool bIsPointerNote = pNote && pNote->GetType() == ra::data::models::MemoryNoteType::Pointer;

    const gsl::index nCount = gsl::narrow_cast<gsl::index>(m_vPointers.Count());
    gsl::index nIndex = 0;
    while (nIndex < nCount)
    {
        const auto nPointerAddress = gsl::narrow_cast<ra::data::ByteAddress>(m_vPointers.GetItemValue(nIndex, LookupItemViewModel::IdProperty));
        if (nPointerAddress == nAddress)
        {
            if (!bIsPointerNote)
            {
                // non-pointer note found, remove it
                m_vPointers.RemoveAt(nIndex);

                if (GetSelectedRootAddress() == nAddress)
                    OnActiveGameChanged(); // force selection of first root address or no selection
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
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        m_vPointers.BeginUpdate();
        m_vPointers.Add(nAddress,
                        ra::util::String::Printf(L"%s | %s", pMemoryContext.FormatAddress(nAddress),
                             ra::data::models::MemoryNoteModel::TrimSize(pNote->GetSummary(), false)));
        m_vPointers.MoveItem(nCount, nIndex);
        m_vPointers.EndUpdate();

        if (!HasSelectedRootAddress())
        {
            SetSelectedRootAddress(nAddress);
            SetValue(HasSelectedRootAddressProperty, true);
        }
    }
}

void PointerInspectorViewModel::OnCurrentAddressChanged(ra::data::ByteAddress nNewAddress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    if (pMemoryNotes != nullptr)
    {
        // select an invalid node to force LoadNodes to select the new root node after it's been updated
        SetSelectedNode(SelectedNodeNone);

        const auto* pNote = pMemoryNotes->FindNote(nNewAddress);
        if (pNote)
        {
            LoadNodes(pNote);
            LoadNote(pNote, 0);
        }
        else
        {
            LoadNodes(nullptr);
            LoadNote(nullptr, 0);
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

const ra::data::models::MemoryNoteModel* PointerInspectorViewModel::FindNestedMemoryNoteModel(
    const ra::data::models::MemoryNoteModel& pRootNote, int nNewNode)
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

        const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pParentNote);
        const auto* pNestedNote = pStructuredNote ? pStructuredNote->GetNoteAtOffset(nOffset) : nullptr;
        if (!pNestedNote)
            return nullptr;

        pParentNote = pNestedNote;
        sChain.pop();
    } while (!sChain.empty());

    return pParentNote;
}

void PointerInspectorViewModel::OnSelectedNodeChanged(int nNewNode)
{
    SetValue(HasSelectedNodeProperty, nNewNode >= PointerNodeViewModel::RootNodeId);

    DispatchMemoryRead([this, nNewNode]() {
        const auto nSelectedNode = GetSelectedNode();
        if (nSelectedNode == nNewNode)
        {
            const auto pNoteReference = UpdatePointerChain(nNewNode);
            LoadNote(pNoteReference.pMemoryNote, pNoteReference.nElementIndex);
        }
    });
}

void PointerInspectorViewModel::OnSelectedFieldChanged(int nNewFieldIndex)
{
    m_bSyncingNote = true;
    if (nNewFieldIndex != -1)
    {
        const auto* pField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNewFieldIndex);
        Expects(pField != nullptr);
        SetCurrentFieldNote(pField->GetRealNote());
    }
    else
    {
        SetCurrentFieldNote(L"");
    }
    m_bSyncingNote = false;
}

const ra::data::models::MemoryNoteModel::Reference PointerInspectorViewModel::UpdatePointerChain(int nNewNode)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    const auto nCurrentAddress = GetSelectedRootAddress();
    const auto* pNote = pMemoryNotes ? pMemoryNotes->FindNote(nCurrentAddress) : nullptr;

    if (m_bRebuildNodes)
    {
        m_bRebuildNodes = false;

        if (pNote)
            LoadNodes(pNote);
    }

    auto nNewNodeIndex = Nodes().FindItemIndex(PointerNodeViewModel::IdProperty, nNewNode);
    if (nNewNodeIndex == -1)
    {
        if (Nodes().Count() == 0)
        {
            PointerChain().Clear();
            return {};
        }

        nNewNodeIndex = 0;
    }

    std::stack<const PointerNodeViewModel*> sChain;
    GetPointerChain(nNewNodeIndex, sChain);
    uint32_t nIndex = 0xFFFFFFFF;

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

        nIndex = 0xFFFFFFFF;
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

            const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pNote);
            if (pStructuredNote)
            {
                pNote = pStructuredNote->GetNoteAtOffset(pItem->m_nOffset);

                if (pNote && ra::to_signed(pNote->GetAddress()) != pItem->m_nOffset)
                {
                    std::vector<ra::data::models::MemoryNoteModel::Reference> vChain;
                    if (pStructuredNote->GetChainTo(vChain, *pNote) && vChain.size() == 3)
                        pNote = vChain.at(1).pMemoryNote;
                }

                const auto* pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(pNote);
                if (pArrayNote != nullptr && pArrayNote->GetElementCount() > 1)
                    nIndex = (pItem->m_nOffset - pArrayNote->GetAddress()) / pArrayNote->GetElementSize();
            }
            else
            {
                pNote = nullptr;
            }
        }

        pItem->m_pNote = pNote;
        if (pNote)
        {
            // SetRealNote will keep a "[pointer]" tag on the description, so strip it now.
            std::wstring sNote = ra::data::models::MemoryNoteModel::TrimSize(pNote->GetSummary(), false);

            if (nIndex != 0xFFFFFFFF)
            {
                sNote.push_back(' ');
                sNote.push_back('[');
                sNote.append(std::to_wstring(nIndex));
                sNote.push_back(']');
            }

            pItem->SetRealNote(sNote);
            pItem->SetSize(pNote->GetMemSize());
        }
        else
        {
            pItem->SetRealNote(L"");
            pItem->SetSize(ra::data::Memory::Size::EightBit);
        }

        pItem->m_nIndex = nIndex;

        // EndInitialization does memory reads, so it must be dispatched. we'll do it in a bit

        ++nInsertIndex;

        sChain.pop();
    } while (!sChain.empty());

    while (nCount > nInsertIndex)
        PointerChain().RemoveAt(--nCount);

    DispatchMemoryRead([this]() {
        for (auto& pItem : PointerChain())
            pItem.EndInitialization();

        UpdatePointerChainValues();
    });

    PointerChain().EndUpdate();

    return { pNote, 0, nIndex };
}

void PointerInspectorViewModel::SyncField(PointerInspectorViewModel::StructFieldViewModel& pFieldViewModel, const ra::data::models::MemoryNoteModel& pOffsetNote)
{
    pFieldViewModel.m_pNote = &pOffsetNote;

    pFieldViewModel.SetFormat(pOffsetNote.GetDefaultMemFormat());

    const auto sNote = pOffsetNote.GetNote();
    pFieldViewModel.SetRealNote(sNote);

    auto nSize = pOffsetNote.GetMemSize();
    switch (nSize)
    {
        case ra::data::Memory::Size::Unknown:
            nSize = ra::data::Memory::Size::EightBit;
            pFieldViewModel.SetReadOnly(false);
            break;

        case ra::data::Memory::Size::Array:
            pFieldViewModel.SetFormat(ra::data::Memory::Format::Unknown);
            pFieldViewModel.SetReadOnly(true);
            break;

        default:
            pFieldViewModel.SetReadOnly(false);
            break;
    }
    pFieldViewModel.SetSize(nSize);

    auto nIndex = sNote.find('\n');
    if (nIndex == std::string::npos)
    {
        pFieldViewModel.SetDescription(ra::data::models::MemoryNoteModel::TrimSize(sNote, true));
        pFieldViewModel.SetBody(L"");
    }
    else
    {
        std::wstring sTrimmed = sNote.substr(nIndex + 1);
        pFieldViewModel.SetBody(ra::util::String::Trim(sTrimmed));

        if (nIndex > 0 && sNote.at(nIndex - 1) == '\r')
            --nIndex;

        pFieldViewModel.SetDescription(ra::data::models::MemoryNoteModel::TrimSize(sNote.substr(0, nIndex), true));
    }
}

void PointerInspectorViewModel::LoadNote(const ra::data::models::MemoryNoteModel* pNote, uint32_t nElementIndex)
{
    if (pNote == nullptr)
    {
        SetSelectedNodeNote(L"");
        m_pCurrentNote = nullptr;
        m_vmFields.Items().Clear();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mtxLoadNote);

        m_bSyncingNote = true;
        SetSelectedNodeNote(pNote->GetFullSummary());
        m_bSyncingNote = false;

        m_pCurrentNote = pNote;
        gsl::index nCount = gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count());

        gsl::index nInsertIndex = 0;
        m_vmFields.Items().BeginUpdate();

        const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pNote);
        if (pStructuredNote != nullptr)
        {
            const auto pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(pStructuredNote);
            const auto nBaseAddress = pStructuredNote->GetBaseAddress();

            pStructuredNote->EnumerateOffsetNotes([this, &nCount, &nInsertIndex, nBaseAddress, pArrayNote, nElementIndex]
                (const ra::data::models::MemoryNoteModel::Reference& pOffsetNote)
                {
                    const auto nOffset = pOffsetNote.nAddress - nBaseAddress;

                    if (pArrayNote != nullptr && nOffset >= pArrayNote->GetElementSize())
                        return false;

                    const std::wstring sOffset = ra::util::String::Printf(L"+%04x", nOffset);

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
                    if (pArrayNote != nullptr)
                        pItem->m_nOffset += nElementIndex * pArrayNote->GetElementSize();

                    m_bSyncingNote = true;
                    pItem->SetOffset(sOffset);
                    pItem->SetAddressWithoutUpdatingValue(pOffsetNote.nAddress);
                    SyncField(*pItem, *pOffsetNote.pMemoryNote);
                    m_bSyncingNote = false;

                    // EndInitialization does memory reads, so it must be dispatched. we'll do it in a bit

                    ++nInsertIndex;
                    return true;
                },
                nBaseAddress);
        }

        while (nCount > nInsertIndex)
            m_vmFields.Items().RemoveAt(--nCount);
    }

    DispatchMemoryRead([this, pNote]() {
        if (pNote == m_pCurrentNote)
        {
            std::lock_guard lock(m_mtxLoadNote);
            if (pNote == m_pCurrentNote)
            {
                for (auto& pItem : m_vmFields.Items())
                    pItem.EndInitialization();

                UpdateValues();
            }
        }

        m_vmFields.Items().EndUpdate();

        if (pNote == m_pCurrentNote)
            OnSelectedFieldChanged(m_vmFields.GetSingleSelectionIndex());
    });
}

static void LoadSubNotes(LookupItemViewModelCollection& vNodes,
    const ra::data::models::StructuredMemoryNoteModel& pNote, ra::data::ByteAddress nBaseAddress, int nDepth, int nParentIndex)
{
    pNote.EnumerateOffsetNotes([&vNodes, nBaseAddress, nDepth, nParentIndex]
                                (const ra::data::models::MemoryNoteModel::Reference& pOffsetNote) {
        auto nOffset = pOffsetNote.nAddress - nBaseAddress;
        const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pOffsetNote.pMemoryNote);
        if (!pStructuredNote)
            return true;

        std::wstring sLabel;
        if (nDepth > 1)
            sLabel = std::wstring(gsl::narrow_cast<size_t>(nDepth) - 1, ' ');

        const auto pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(pStructuredNote);
        if (pArrayNote && pArrayNote->GetElementCount() > 1)
        {
            auto nStop = pArrayNote->GetElementCount();
            if (nStop > 32)
                nStop = 20;

            for (uint32_t nIndex = 0; nIndex < nStop; ++nIndex)
            {
                const std::wstring sArrayLabel = ra::util::String::Printf(L"%s+%04x | %s [%u]", sLabel, nOffset, pStructuredNote->GetFullSummary(), nIndex);

                vNodes.Add<PointerInspectorViewModel::PointerNodeViewModel>(nParentIndex, nOffset, sArrayLabel);

                if (pStructuredNote->HasNestedStructures())
                    LoadSubNotes(vNodes, *pStructuredNote, pStructuredNote->GetBaseAddress(), nDepth + 1, gsl::narrow_cast<int>(vNodes.Count() - 1));

                nOffset += pArrayNote->GetElementSize();
            }
        }
        else
        {
            sLabel += ra::util::String::Printf(L"+%04x | %s", nOffset, pStructuredNote->GetFullSummary());

            vNodes.Add<PointerInspectorViewModel::PointerNodeViewModel>(nParentIndex, nOffset, sLabel);

            if (pStructuredNote->HasNestedStructures())
                LoadSubNotes(vNodes, *pStructuredNote, pStructuredNote->GetBaseAddress(), nDepth + 1, gsl::narrow_cast<int>(vNodes.Count() - 1));
        }

        return true;
    }, nBaseAddress);
}

void PointerInspectorViewModel::LoadNodes(const ra::data::models::MemoryNoteModel* pNote)
{
    const int nSelectedNode = GetSelectedNode();

    m_vNodes.BeginUpdate();
    m_vNodes.Clear();

    if (pNote == nullptr)
    {
        m_vNodes.Add<PointerNodeViewModel>(PointerNodeViewModel::RootNodeId, ra::to_signed(GetSelectedRootAddress()), L"Root");
    }
    else
    {
        m_vNodes.Add<PointerNodeViewModel>(PointerNodeViewModel::RootNodeId, ra::to_signed(GetSelectedRootAddress()), pNote->GetFullSummary());

        const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pNote);
        if (pStructuredNote && pStructuredNote->HasNestedStructures())
            LoadSubNotes(m_vNodes, *pStructuredNote, pStructuredNote->GetBaseAddress(), 1, 0);
    }

    m_vNodes.EndUpdate();

    // if the selected item is no longer available, select the root node
    if (m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, nSelectedNode) == -1)
        SetSelectedNode(PointerNodeViewModel::RootNodeId);
}

static void AppendNoteOffsetAndSize(ra::util::StringBuilder& builder, const std::wstring& sIndent,
    const ra::data::models::MemoryNoteModel& pNote, int32_t nOffset, ra::data::Memory::Size nSize)
{
    builder.Append(ra::util::String::Printf(L"\r\n%s0x%02X: ", sIndent, ra::to_unsigned(nOffset)));

    if (nSize != ra::data::Memory::Size::Unknown)
    {
        builder.Append(L'[');
        if (nSize == ra::data::Memory::Size::Text)
        {
            builder.Append(pNote.GetBytes());
            builder.Append(L"-byte ASCII] ");
        }
        else
        {
            switch (pNote.GetType())
            {
                case ra::data::models::MemoryNoteType::Pointer:
                    builder.Append(ra::data::Memory::SizeString(nSize));
                    builder.Append(L" pointer] ");
                    break;

                case ra::data::models::MemoryNoteType::Array:
                {
                    const auto* pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(&pNote);
                    Expects(pArrayNote != nullptr);
                    builder.Append(ra::util::String::Printf(L"%ux%u bytes] ", pArrayNote->GetElementCount(), pArrayNote->GetElementSize()));
                    break;
                }

                default:
                    builder.Append(ra::data::Memory::SizeString(nSize));
                    builder.Append(L"] ");
                    break;
            }
        }
    }
}

void PointerInspectorViewModel::BuildNoteForCurrentNode(ra::util::StringBuilder& builder,
    std::stack<const PointerInspectorViewModel::PointerNodeViewModel*>& sChain, std::wstring& sIndent)
{
    std::unique_ptr<ra::data::models::MemoryNoteModel> newNote;

    // If a single field is selected, push the current field note value back into the field.
    const auto nSingleSelectionIndex = gsl::narrow_cast<gsl::index>(m_vmFields.GetSingleSelectionIndex());
    auto* pSelectedField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nSingleSelectionIndex);

    // SyncField will cause pField to point at the local newNote. Capture the current value so it can be restored.
    const ra::data::models::MemoryNoteModel* pExistingNote = pSelectedField ? pSelectedField->m_pNote : nullptr;

    if (pSelectedField)
    {
        newNote = ra::data::models::MemoryNoteModel::Parse(GetCurrentFieldNote());

        m_bSyncingNote = true;
        SyncField(*pSelectedField, *newNote); // Sets pSelectedField->m_pNote to newNote so we don't have to check in the loop.
        m_bSyncingNote = false;

        const bool bExistingPointer = pExistingNote && dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pExistingNote) != nullptr;
        const bool bNewPointer = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(newNote.get()) != nullptr;
        if (bExistingPointer != bNewPointer)
            m_bRebuildNodes = true;
    }

    // If we're viewing a non-zero index of an array, the offsets need to be adjusted back to the 0th element.
    int32_t nAdjustment = 0;
    const auto* pArrayParent = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(m_pCurrentNote);
    if (pArrayParent != nullptr && m_vPointerChain.Count() > 0)
    {
        const auto* pParent = m_vPointerChain.GetItemAt<StructFieldViewModel>(m_vPointerChain.Count() - 1);
        nAdjustment = pParent->m_nIndex * pArrayParent->GetElementSize();
    }

    for (auto& pItem : m_vmFields.Items())
    {
        const auto* pField = dynamic_cast<StructFieldViewModel*>(&pItem);
        if (!pField)
            continue;

        Expects(pField->m_pNote != nullptr);
        AppendNoteOffsetAndSize(builder, sIndent, *pField->m_pNote, pField->m_nOffset - nAdjustment, pField->GetSize());

        const auto& sDescription = pField->GetDescription();
        if (pField->m_pNote->GetType() == ra::data::models::MemoryNoteType::Pointer && ra::util::String::StartsWith(sDescription, L"[pointer] "))
            builder.Append(sDescription.substr(10));
        else
            builder.Append(sDescription);

        const auto& sBody = pField->GetBody();
        if (!sBody.empty())
        {
            builder.Append(L"\r\n");
            builder.Append(sBody);
        }

        const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pField->m_pNote);
        if (pStructuredNote != nullptr)
            BuildNote(builder, sChain, *pStructuredNote, sIndent);
    }

    // Restore the captured note (temporary note is about to go out of scope).
    if (newNote && pSelectedField)
        pSelectedField->m_pNote = pExistingNote;
}

void PointerInspectorViewModel::BuildNote(ra::util::StringBuilder& builder,
    std::stack<const PointerInspectorViewModel::PointerNodeViewModel*>& sChain,
    const ra::data::models::MemoryNoteModel& pNote, std::wstring& sIndent)
{
    const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(&pNote);
    Expects(pStructuredNote != nullptr);

    switch (pNote.GetType())
    {
        case ra::data::models::MemoryNoteType::Pointer:
            sIndent.push_back(L'+');
            break;

        case ra::data::models::MemoryNoteType::Array:
            sIndent.push_back(L'|');
            break;

        default:
            break;
    }

    if (sIndent.length() == sChain.size() && pStructuredNote == m_pCurrentNote)
    {
        BuildNoteForCurrentNode(builder, sChain, sIndent);
        sIndent.pop_back();
        return;
    }

    const auto nBaseAddress = pStructuredNote->GetBaseAddress();

    pStructuredNote->EnumerateOffsetNotes([this, &builder, &sChain, nBaseAddress, &sIndent]
        (const ra::data::models::MemoryNoteModel& pOffsetNote) {
            AppendNoteOffsetAndSize(builder, sIndent, pOffsetNote, pOffsetNote.GetAddress(), pOffsetNote.GetMemSize());

            const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(&pOffsetNote);
            if (pStructuredNote != nullptr)
            {
                // If this is the selected node, use the editable text value. Otherwise, pull the summary value from the note.
                if (&pOffsetNote == m_pCurrentNote)
                    builder.Append(ra::data::models::MemoryNoteModel::TrimSize(GetSelectedNodeNote(), false));
                else
                    builder.Append(ra::data::models::MemoryNoteModel::TrimSize(pOffsetNote.GetSummary(), false));

                BuildNote(builder, sChain, *pStructuredNote, sIndent);
            }
            else
            {
                // Not a structured note. Capture the entire note (header and body).
                builder.Append(ra::data::models::MemoryNoteModel::TrimSize(pOffsetNote.GetNote(), false));
            }

            return true;
        });

    sIndent.pop_back();
}

void PointerInspectorViewModel::UpdateSourceMemoryNote()
{
    const auto nAddress = GetSelectedRootAddress();
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    Expects(pMemoryNotes != nullptr);
    auto* pNote = pMemoryNotes->FindNote(nAddress);
    if (pNote != nullptr)
    {
        const auto nIndex = m_vNodes.FindItemIndex(LookupItemViewModel::IdProperty, GetSelectedNode());
        if (nIndex != -1)
        {
            std::stack<const PointerNodeViewModel*> sChain;
            GetPointerChain(nIndex, sChain);

            ra::util::StringBuilder builder;

            if (pNote == m_pCurrentNote)
                builder.Append(GetSelectedNodeNote());
            else
                builder.Append(pNote->GetFullSummary());

            m_bRebuildNodes = false;

            std::wstring sIndent;
            BuildNote(builder, sChain, *pNote, sIndent);

            // nested note reference is invalidated in SetMemoryNote. temporarily release
            // the reference and restore it after SetMemoryNote completes to prevent the UI
            // thread from trying to use it while it's invalid (we're most likely on a
            // background thread kicked off by a timer to support as-you-type changes).
            m_pCurrentNote = nullptr;

            m_bSyncingNote = true;
            pMemoryNotes->SetNote(nAddress, builder.ToWString());
            m_bSyncingNote = false;

            // update the current note pointer
            const auto pNoteReference = UpdatePointerChain(GetSelectedNode());
            m_pCurrentNote = pNoteReference.pMemoryNote;

            // update the field note pointers
            gsl::index nNoteIndex = 0;
            const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(m_pCurrentNote);
            if (pStructuredNote) {
                pStructuredNote->EnumerateOffsetNotes([this, &nNoteIndex](const ra::data::models::MemoryNoteModel& pOffsetNote)
                {
                    m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNoteIndex++)->m_pNote = &pOffsetNote;
                    return true;
                });
            }
            for (; nNoteIndex < gsl::narrow_cast<gsl::index>(m_vmFields.Items().Count()); ++nNoteIndex)
                m_vmFields.Items().GetItemAt<StructFieldViewModel>(nNoteIndex)->m_pNote = nullptr;

            // also write the changes to disk
            std::vector<ra::data::models::AssetModelBase*> vAssets;
            vAssets.push_back(pMemoryNotes);
            pGameContext.Assets().SaveAssets(vAssets);
        }
    }
}

void PointerInspectorViewModel::UpdatePointerChainValues()
{
    if (m_vPointerChain.Count() == 0)
        return;

    ra::data::ByteAddress nAddress = 0;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();

    m_vPointerChain.BeginUpdate();

    for (auto& pPointer : m_vPointerChain)
    {
        nAddress += pPointer.m_nOffset;
        pPointer.SetAddressWithoutUpdatingValue(nAddress);

        if (pPointer.DoFrame())
            UpdatePointerChainRowColor(pPointer);

        const auto* pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(pPointer.m_pNote);
        if (pArrayNote != nullptr)
        {
            
        }
        else
        {
            nAddress = pPointer.GetCurrentValueRaw();
            if (nAddress > 0)
            {
                const auto nAdjustedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
                if (nAdjustedAddress != 0xFFFFFFFF)
                    nAddress = nAdjustedAddress;
            }
        }
    }

    m_vPointerChain.EndUpdate();
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

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    bool bValid = false;

    auto nMemSize = ra::data::Memory::Size::Unknown;
    uint32_t nMask = 0xFFFFFFFF;
    uint32_t nOffset = 0;
    if (pConsoleContext.GetRealAddressConversion(&nMemSize, &nMask, &nOffset))
    {
        if (nMemSize == ra::data::Memory::Size::TwentyFourBit)
            nMemSize = ra::data::Memory::Size::ThirtyTwoBit;

        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto nRawPointer = pMemoryContext.ReadMemory(pPointer.GetAddress(), nMemSize);
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
    const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(m_pCurrentNote);
    const auto nBaseAddress = pStructuredNote ? pStructuredNote->GetBaseAddress() : 0U;

    m_vmFields.Items().BeginUpdate();

    for (auto& pItem : m_vmFields.Items())
    {
        auto* pField = dynamic_cast<StructFieldViewModel*>(&pItem);
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
    if (!IsVisible())
        return;

    UpdatePointerChainValues();
    UpdateValues();
}

void PointerInspectorViewModel::CopyDefinition() const
{
    const auto sDefinition = GetMemRefChain(false);
    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::util::String::Widen(sDefinition));
}

std::string PointerInspectorViewModel::GetMemRefChain(bool bMeasured) const
{
    std::string sBuffer;

    const auto nSelectedFieldIndex = m_vmFields.Items().FindItemIndex(LookupItemViewModel::IsSelectedProperty, true);
    if (nSelectedFieldIndex == -1)
        return sBuffer;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pMemoryNotes = pGameContext.Assets().FindMemoryNotes();
    if (pMemoryNotes == nullptr)
        return sBuffer;

    const auto* vmField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nSelectedFieldIndex);
    Expects(vmField != nullptr);

    auto* pRootNote = pMemoryNotes->FindNote(GetSelectedRootAddress());
    Expects(pRootNote != nullptr);

    std::vector<ra::data::models::MemoryNoteModel::Reference> vChain;
    const auto* pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(pRootNote);
    Expects(pStructuredNote != nullptr);
    pStructuredNote->GetChainTo(vChain, *m_pCurrentNote);

    std::unique_ptr<ra::data::models::MemoryNoteModel> pVirtualLeaf;
    pStructuredNote = dynamic_cast<const ra::data::models::StructuredMemoryNoteModel*>(m_pCurrentNote);
    Expects(pStructuredNote != nullptr);
    auto* pLeafNote = pStructuredNote->GetNoteAtOffset(vmField->m_nOffset);
    if (pLeafNote == nullptr)
    {
        pVirtualLeaf = ra::data::models::MemoryNoteModel::Parse(L"Virtual Note");
        pVirtualLeaf->SetMemSize(vmField->GetSize());
        pVirtualLeaf->SetAddress(vmField->m_nOffset);
        pLeafNote = pVirtualLeaf.get();
    }
    else if (vmField->m_nOffset != ra::to_signed(pLeafNote->GetAddress()))
    {
        const auto* pArrayNote = dynamic_cast<const ra::data::models::ArrayMemoryNoteModel*>(pStructuredNote);
        if (pArrayNote != nullptr)
            vChain.back().nElementIndex = (vmField->m_nOffset - pLeafNote->GetAddress()) / pArrayNote->GetElementSize();
    }
    vChain.emplace_back(pLeafNote, pStructuredNote->GetBaseAddress(), 0);

    sBuffer = ra::data::util::AchievementLogicSerializer::BuildMemRefChain(vChain);

    if (bMeasured) // BuildMemRefChain returns a Measured value. If that's what's wanted, return it.
        return sBuffer;

    // remove Measured flag
    const auto nIndex = sBuffer.rfind("M:");
    if (nIndex != std::string::npos)
        sBuffer.erase(nIndex, 2);

    // add comparison to current value
    ra::data::util::AchievementLogicSerializer::AppendOperator(sBuffer, ra::data::Requirement::OperatorType::Equals);
    ra::data::util::AchievementLogicSerializer::AppendOperand(sBuffer, ra::data::Requirement::OperandType::Value,
                                                            vmField->GetSize(), vmField->GetCurrentValueRaw());

    return sBuffer;
}

void PointerInspectorViewModel::BookmarkCurrentField() const
{
    auto sDefinition = GetMemRefChain(true);

    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.MemoryBookmarks.AddBookmark(sDefinition);

    auto* pBookmark = pWindowManager.MemoryBookmarks.Bookmarks().Items().GetItemAt(pWindowManager.MemoryBookmarks.Bookmarks().Items().Count() - 1);
    if (pBookmark != nullptr)
    {
        const auto nSelectedFieldIndex = m_vmFields.Items().FindItemIndex(LookupItemViewModel::IsSelectedProperty, true);
        if (nSelectedFieldIndex != -1)
        {
            const auto* vmField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(nSelectedFieldIndex);
            if (vmField != nullptr)
            {
                pBookmark->SetSize(vmField->GetSize());
                pBookmark->SetFormat(vmField->GetFormat());
            }
        }
    }

    if (!pWindowManager.MemoryBookmarks.IsVisible())
        pWindowManager.MemoryBookmarks.Show();
}

void PointerInspectorViewModel::OnFieldOffsetChanged(gsl::index nIndex, const std::wstring& sNewOffset)
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
        UpdateSourceMemoryNote();

        // lastly, update the current value (may trigger PauseOnChange)
        const auto* pPointerNote = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(m_pCurrentNote);
        const auto nBaseAddress = pPointerNote ? pPointerNote->GetBaseAddress() : 0U;
        pField->SetAddress(nBaseAddress + pField->m_nOffset);
        DispatchMemoryRead([pField]() { pField->DoFrame(); });
    }
}

void PointerInspectorViewModel::StructFieldViewModel::FormatOffset()
{
    std::wstring sLabel;
    if (m_nIndent > 0)
        sLabel = std::wstring(m_nIndent, ' ');
    sLabel += ra::util::String::Printf(L"+%04x", m_nOffset);

    m_bFormattingOffset = true;
    SetOffset(sLabel);
    m_bFormattingOffset = false;
}

void PointerInspectorViewModel::OnFieldSizeChanged(gsl::index nIndex)
{
    // if it's the selected note, we have to update the CurrentFieldNote
    // before calling UpdatingSourceMemoryNote().
    if (nIndex == Fields().GetSingleSelectionIndex())
    {
        const auto* pNote = Fields().Items().GetItemAt<StructFieldViewModel>(nIndex);
        if (pNote != nullptr)
        {
            const auto sUnsizedFieldNote = ra::data::models::MemoryNoteModel::TrimSize(GetCurrentFieldNote(), false);
            const auto sNewFieldNote = ra::util::String::Printf(L"[%s%s] %s",
                ra::data::Memory::SizeString(pNote->GetSize()),
                pNote->m_pNote->GetType() == ra::data::models::MemoryNoteType::Pointer ? L" pointer" : L"",
                sUnsizedFieldNote);

            m_bSyncingNote = true;
            SetCurrentFieldNote(sNewFieldNote);
            m_bSyncingNote = false;
        }
    }

    UpdateSourceMemoryNote();
}

void PointerInspectorViewModel::NewField()
{
    auto pField = std::make_unique<StructFieldViewModel>();
    pField->BeginInitialization();
    pField->SetFormat(ra::data::Memory::Format::Dec);

    if (m_vmFields.Items().Count() > 0)
    {
        auto pLastField = m_vmFields.Items().GetItemAt<StructFieldViewModel>(m_vmFields.Items().Count() - 1);
        Expects(pLastField != nullptr);
        pField->m_nOffset = pLastField->m_nOffset + pLastField->GetSizeBytes();
        pField->SetSize(pLastField->GetSize());
    }
    else
    {
        // TODO: ConsoleContext should return an architecture size
        auto nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
        uint32_t nMask = 0, nOffset = 0;
        auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        pConsoleContext.GetRealAddressConversion(&nReadSize, &nMask, &nOffset);

        switch (nReadSize)
        {
            case ra::data::Memory::Size::TwentyFourBit:
                nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
                break;
            case ra::data::Memory::Size::TwentyFourBitBigEndian:
                nReadSize = ra::data::Memory::Size::ThirtyTwoBitBigEndian;
                break;
        }

        pField->SetSize(nReadSize);
    }

    pField->SetRealNote(ra::util::String::Printf(L"[%s]", ra::data::Memory::SizeString(pField->GetSize())));

    const auto* pPointerNote = dynamic_cast<const ra::data::models::PointerMemoryNoteModel*>(m_pCurrentNote);
    const auto nBaseAddress = pPointerNote ? pPointerNote->GetBaseAddress() : 0U;
    pField->SetAddress(nBaseAddress + pField->m_nOffset);

    pField->FormatOffset();
    pField->EndInitialization();

    m_vmFields.Items().Append(std::move(pField));
}

void PointerInspectorViewModel::RemoveSelectedField()
{
    const auto nIndex = m_vmFields.GetSingleSelectionIndex();
    if (nIndex == -1)
        return;

    m_vmFields.Items().RemoveAt(nIndex);
    UpdateSourceMemoryNote();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
