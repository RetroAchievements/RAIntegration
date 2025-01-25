#include "PointerInspectorViewModel.hh"

#include "RA_Defs.h"

#include "data\context\ConsoleContext.hh"

#include "services\AchievementLogicSerializer.hh"
#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

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
    else if (args.Property == SingleSelectionIndexProperty)
    {
        OnSelectedFieldChanged(args.tNewValue);
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
    const auto* pNote = UpdatePointerChain(nNewNode);
    LoadNote(pNote);
}

void PointerInspectorViewModel::OnSelectedFieldChanged(int nNewFieldIndex)
{
    m_bSyncingNote = true;
    if (nNewFieldIndex != -1)
    {
        const auto* pField = Bookmarks().GetItemAt<StructFieldViewModel>(nNewFieldIndex);
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
        nNewNodeIndex = 0;

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
            pItem->SetOffset(L"");
        }
        else
        {
            pItem->m_nOffset = pNode->GetOffset();

            std::wstring sLabel;
            if (nInsertIndex > 1)
                sLabel = std::wstring(nInsertIndex - 1, ' ');
            sLabel += ra::StringPrintf(L"+%04x", pItem->m_nOffset);
            pItem->SetOffset(sLabel);

            pNote = pNote ? pNote->GetPointerNoteAtOffset(pItem->m_nOffset) : nullptr;
        }

        if (pNote)
        {
            pItem->SetDescription(pNote->GetPointerDescription());
            pItem->SetSize(pNote->GetMemSize());
        }
        else
        {
            pItem->SetDescription(L"");
            pItem->SetSize(MemSize::EightBit);
        }

        UpdatePointerChainRowColor(*pItem);
        pItem->EndInitialization();

        ++nInsertIndex;

        sChain.pop();
    } while (!sChain.empty());

    while (nCount > nInsertIndex)
        Bookmarks().RemoveAt(--nCount);

    UpdatePointerChainValues();

    PointerChain().EndUpdate();

    return pNote;
}

static std::wstring TrimSize(const std::wstring& sNote)
{
    size_t nEndIndex;
    size_t nStartIndex = sNote.find('[');
    if (nStartIndex != std::string::npos)
    {
        nEndIndex = sNote.find(']', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return sNote;
    }
    else
    {
        nStartIndex = sNote.find('(');
        if (nStartIndex == std::string::npos)
            return sNote;

        nEndIndex = sNote.find(')', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return sNote;
    }

    bool bPointer = false;
    std::wstring sWord;
    ra::data::models::CodeNoteModel::Parser::TokenType nTokenType;
    ra::data::models::CodeNoteModel::Parser parser(sNote, nStartIndex + 1, nEndIndex);
    do
    {
        nTokenType = parser.NextToken(sWord);
        if (nTokenType == ra::data::models::CodeNoteModel::Parser::TokenType::Other)
        {
            if (sWord == L"pointer")
                bPointer = true;
            else
                return sNote;
        }
    } while (nTokenType != ra::data::models::CodeNoteModel::Parser::TokenType::None);

    while (nStartIndex > 0 && isspace(sNote.at(nStartIndex - 1)))
        nStartIndex--;

    while (nEndIndex < sNote.length() - 1 && isspace(sNote.at(nEndIndex + 1)))
        nEndIndex++;

    std::wstring sNoteCopy = sNote;
    sNoteCopy.erase(nStartIndex, nEndIndex - nStartIndex + 1);

    if (bPointer)
        sNoteCopy.insert(0, L"[pointer] ");

    return sNoteCopy;
}

void PointerInspectorViewModel::SyncField(PointerInspectorViewModel::StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote)
{
    const auto nSize = pOffsetNote.GetMemSize();
    pFieldViewModel.SetSize((nSize == MemSize::Unknown) ? MemSize::EightBit : nSize);

    const auto& sNote = pOffsetNote.GetPrimaryNote();
    pFieldViewModel.SetBody(sNote);

    const auto nIndex = sNote.find('\n');
    if (nIndex == std::string::npos)
        pFieldViewModel.SetDescription(TrimSize(sNote));
    else
        pFieldViewModel.SetDescription(TrimSize(sNote.substr(0, nIndex)));
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
    gsl::index nCount = gsl::narrow_cast<gsl::index>(Bookmarks().Count());

    gsl::index nInsertIndex = 0;
    Bookmarks().BeginUpdate();
    pNote->EnumeratePointerNotes([this, &nCount, &nInsertIndex, nBaseAddress]
        (ra::ByteAddress nAddress, const ra::data::models::CodeNoteModel& pOffsetNote)
        {
            const auto nOffset = nAddress - nBaseAddress;
            const std::wstring sOffset = ra::StringPrintf(L"+%04x", nOffset);

            StructFieldViewModel* pItem = nullptr;
            if (nInsertIndex < nCount)
            {
                pItem = Bookmarks().GetItemAt<StructFieldViewModel>(nInsertIndex);
                Expects(pItem != nullptr);
                pItem->SetSelected(false);
                pItem->SetFormat(MemFormat::Hex);
            }
            else
            {
                ++nCount;
                pItem = &Bookmarks().Add<StructFieldViewModel>();
            }

            pItem->BeginInitialization();

            pItem->m_nOffset = nOffset;
            pItem->SetOffset(sOffset);
            SyncField(*pItem, pOffsetNote);

            pItem->EndInitialization();

            ++nInsertIndex;
            return true;
        });

    while (nCount > nInsertIndex)
        Bookmarks().RemoveAt(--nCount);

    UpdateValues();
    Bookmarks().EndUpdate();

    OnSelectedFieldChanged(GetValue(SingleSelectionIndexProperty));
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

void PointerInspectorViewModel::UpdatePointerChainValues()
{
    ra::ByteAddress nAddress = 0;
    const auto nCount = gsl::narrow_cast<gsl::index>(PointerChain().Count());

    PointerChain().BeginUpdate();

    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pPointer = PointerChain().GetItemAt(nIndex);
        if (pPointer != nullptr)
        {
            nAddress += pPointer->m_nOffset;
            pPointer->SetAddressWithoutUpdatingValue(nAddress);

            if (pPointer->MemoryChanged())
                UpdatePointerChainRowColor(*pPointer);

            nAddress = pPointer->GetCurrentValueRaw();
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
            ra::ui::Color(ra::to_unsigned(MemoryBookmarkViewModel::RowColorProperty.GetDefaultValue())));
    }
}

void PointerInspectorViewModel::UpdateValues()
{
    const auto nBaseAddress = (m_pCurrentNote != nullptr) ? m_pCurrentNote->GetPointerAddress() : 0U;

    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.RemoveNotifyTarget(*this);

    Bookmarks().BeginUpdate();

    const auto nCount = gsl::narrow_cast<gsl::index>(Bookmarks().Count());
    for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
    {
        auto* pField = Bookmarks().GetItemAt<StructFieldViewModel>(nIndex);
        if (pField != nullptr)
        {
            pField->SetAddress(nBaseAddress + pField->m_nOffset);
            UpdateBookmark(*pField);
        }
    }
    Bookmarks().EndUpdate();

    pEmulatorContext.AddNotifyTarget(*this);
}

void PointerInspectorViewModel::DoFrame()
{
    UpdatePointerChainValues();
    UpdateValues();
}

void PointerInspectorViewModel::CopyDefinition() const
{
    const auto sDefinition = GetDefinition();
    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sDefinition));
}

std::string PointerInspectorViewModel::GetDefinition() const
{
    std::string sBuffer;

    const auto nSelectedFieldIndex = Bookmarks().FindItemIndex(LookupItemViewModel::IsSelectedProperty, true);
    if (nSelectedFieldIndex == -1)
        return sBuffer;

    const auto nSelectedNodeIndex = Nodes().FindItemIndex(LookupItemViewModel::IdProperty, GetSelectedNode());
    if (nSelectedNodeIndex == -1)
        return sBuffer;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return sBuffer;

    std::stack<const PointerNodeViewModel*> sChain;
    GetPointerChain(nSelectedNodeIndex, sChain);

    auto* pNote = pCodeNotes->FindCodeNoteModel(GetCurrentAddress());
    Expects(pNote != nullptr);

    do
    {
        const auto* pNode = sChain.top();
        Expects(pNode != nullptr);

        if (!pNode->IsRootNode())
            pNote = pNote->GetPointerNoteAtOffset(pNode->GetOffset());
        Expects(pNote != nullptr);

        ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, ra::services::TriggerConditionType::AddAddress);
        ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Address,
                                                                pNote->GetMemSize(), pNote->GetAddress());
        ra::services::AchievementLogicSerializer::AppendConditionSeparator(sBuffer);

        sChain.pop();
    } while (!sChain.empty());

    const auto* vmField = Bookmarks().GetItemAt<StructFieldViewModel>(nSelectedFieldIndex);
    Expects(vmField != nullptr);
    ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Address,
                                                            vmField->GetSize(), ra::to_unsigned(vmField->m_nOffset));
    ra::services::AchievementLogicSerializer::AppendOperator(sBuffer, ra::services::TriggerOperatorType::Equals);
    ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Value,
                                                            MemSize::ThirtyTwoBit, vmField->GetCurrentValueRaw());

    return sBuffer;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
