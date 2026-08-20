#include "PointerFinderViewModel.hh"

#include "RA_Defs.h"
#include "util\Strings.hh"

#include "context/IConsoleContext.hh"

#include "data/context/EmulatorContext.hh"
#include "data/context/GameContext.hh"

#include "services/IClipboard.hh"
#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "ui/viewmodels/FileDialogViewModel.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/ProgressViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty PointerFinderViewModel::ResultCountTextProperty("PointerFinderViewModel", "ResultCountText", L"0");
const IntModelProperty PointerFinderViewModel::SearchTypeProperty("PointerFinderViewModel", "SearchType", ra::etoi(ra::services::SearchType::ThirtyTwoBitAligned));
const BoolModelProperty PointerFinderViewModel::ShowProgressDialogProperty("PointerFinderViewModel", "ShowProgressDialog", true);

const StringModelProperty PointerFinderViewModel::StateViewModel::AddressProperty("StateViewModel", "Address", L"");
const StringModelProperty PointerFinderViewModel::StateViewModel::CaptureButtonTextProperty("StateViewModel", "CaptureButtonText", L"Capture");
const BoolModelProperty PointerFinderViewModel::StateViewModel::CanCaptureProperty("StateViewModel", "CanCapture", true);

const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::PointerAddressProperty("PotentialPointerViewModel", "PointerAddress", L"");
const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::OffsetProperty("PotentialPointerViewModel", "Offset", L"");
const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::PointerValue1Property("PotentialPointerViewModel", "PointerValue1", L"");
const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::PointerValue2Property("PotentialPointerViewModel", "PointerValue2", L"");
const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::PointerValue3Property("PotentialPointerViewModel", "PointerValue3", L"");
const StringModelProperty PointerFinderViewModel::PotentialPointerViewModel::PointerValue4Property("PotentialPointerViewModel", "PointerValue4", L"");
const BoolModelProperty PointerFinderViewModel::PotentialPointerViewModel::IsSelectedProperty("PotentialPointerViewModel", "IsSelected", false);

constexpr uint32_t MAX_OFFSET = 1024;

void PointerFinderViewModel::StateViewModel::DoFrame()
{
    if (CanCapture())
        m_pViewer.DoFrame();
}

void PointerFinderViewModel::StateViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == AddressProperty)
    {
        const auto nAddress = ra::data::Memory::ParseAddress(GetAddress());
        m_pViewer.InitializeFixedViewer(nAddress);
    }

    ViewModelBase::OnValueChanged(args);
}

void PointerFinderViewModel::StateViewModel::ToggleCapture()
{
    if (CanCapture())
        Capture();
    else
        ClearCapture();
}

void PointerFinderViewModel::StateViewModel::Capture()
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    if (pMemoryContext.TotalMemorySize() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"No game loaded.", L"Cannot capture memory without a loaded game.");
        return;
    }

    const auto& sAddress = GetAddress();
    const auto nAddress = ra::data::Memory::ParseAddress(sAddress);
    if (nAddress == 0)
    {
        bool bValid = sAddress.size() > 0;
        for (size_t i = 0; i < sAddress.size(); i++)
        {
            if (sAddress.at(i) != '0')
            {
                if (i != 1 || sAddress.at(i) != 'x')
                {
                    bValid = false;
                    break;
                }
            }
        }

        if (!bValid)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid address.");
            return;
        }
    }

    DispatchMemoryRead([this]() {
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto nMemorySize = gsl::narrow<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize());

        ra::services::SearchResults pInitialResults;
        pInitialResults.Initialize(0, nMemorySize, m_pOwner->GetSearchType());

        m_pCapture.Initialize(pInitialResults);
    });

    SetValue(CaptureButtonTextProperty, L"Release");
    SetValue(CanCaptureProperty, false);
}

void PointerFinderViewModel::StateViewModel::ClearCapture()
{
    ra::services::SearchResults pEmptyResults;
    m_pCapture.Initialize(pEmptyResults);

    SetValue(CaptureButtonTextProperty, CaptureButtonTextProperty.GetDefaultValue());
    SetValue(CanCaptureProperty, true);
}

// ------------------------------------

PointerFinderViewModel::PointerFinderViewModel()
{
    SetWindowTitle(L"Pointer Finder");

    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBit), L"16-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::TwentyFourBit), L"24-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBit), L"32-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBitAligned), L"16-bit (aligned)");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBitAligned), L"32-bit (aligned)");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBitBigEndian), L"16-bit BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBitBigEndian), L"32-bit BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBitBigEndianAligned), L"16-bit BE (aligned)");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBitBigEndianAligned), L"32-bit BE (aligned)");

    for (auto& pState : m_vStates)
        pState.SetOwner(this);
}

void PointerFinderViewModel::DoFrame()
{
    if (!IsVisible())
        return;

    for (auto& pState : m_vStates)
        pState.DoFrame();
}

void PointerFinderViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SearchTypeProperty)
    {
        // search type changed, discard any captured states
        for (auto& pState : m_vStates)
        {
            if (!pState.CanCapture())
                pState.ToggleCapture();
        }
    }
}

void PointerFinderViewModel::Find()
{
    // TODO: capture/restore selected address (find may be clicked again after changing captures)

    m_vResults.BeginUpdate();
    m_vResults.Clear();

    ra::services::PointerFinder pPointerFinder;
    int nUniqueAddresses = 0;
    size_t nPointerCount = 0;
    ra::data::ByteAddress nPreviousAddress = 0xFFFFFFFF;
    for (size_t i = 0; i < m_vStates.size(); i++)
    {
        const auto& pStateI = m_vStates.at(i);
        if (pStateI.CanCapture())
            continue;

        const auto nAddress = pStateI.Viewer().GetAddress();
        pPointerFinder.AddCapture(pStateI.CapturedMemory(), nAddress);
        if (nAddress != nPreviousAddress)
        {
            nPreviousAddress = nAddress;
            ++nUniqueAddresses;
        }

        nPointerCount += pStateI.CapturedMemory().GetCapturedPointerCount();
    }

    std::vector<ra::services::PointerFinder::PotentialPointer> vResults;

    if (GetValue(ShowProgressDialogProperty))
    {
        ProgressViewModel vmProgress;
        std::wstring sMessage = ra::util::String::Printf(L"Analyzing %u potential pointers", gsl::narrow_cast<uint32_t>(nPointerCount));
        vmProgress.SetMessage(sMessage);
        vmProgress.QueueTask([&pPointerFinder, &vResults, &vmProgress, &sMessage]() {
            uint32_t nCount = 0;
            pPointerFinder.Analyze(vResults, [&vmProgress, &sMessage, &nCount](size_t nProgress, size_t nTotal)
                {
                    if (vmProgress.GetDialogResult() != DialogResult::None)
                        return false;

                    static const std::array<const wchar_t*, 6> sProgressTicker = { L"", L".", L"..", L"...", L" ..", L"  ." };
                    nCount = (nCount + 1) % gsl::narrow_cast<uint32_t>(sProgressTicker.size());
                    vmProgress.SetMessage(sMessage + sProgressTicker.at(nCount));
                    vmProgress.SetProgress(gsl::narrow_cast<int>(nProgress * 100 / nTotal));
                    return true;
                });
            });
        vmProgress.ShowModal();
    }
    else
    {
        pPointerFinder.Analyze(vResults, nullptr);
    }

    const auto nSize = GetSearchSize();
    for (const auto& pPotentialPointer : vResults)
        AddPotentialPointer(pPotentialPointer, nSize);

    if (m_vResults.Count() == 0 && nUniqueAddresses >= 2)
    {
        auto* pPointer = &m_vResults.Add();
        pPointer->SetPointerAddress(L"No pointers found.");
        SetValue(ResultCountTextProperty, L"0");
    }
    else
    {
        SetValue(ResultCountTextProperty, std::to_wstring(vResults.size()));
    }

    m_vResults.EndUpdate();

    if (nUniqueAddresses < 2)
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Cannot find.", L"At least two unique addresses must be captured before potential pointers can be located.");
}

void PointerFinderViewModel::AddPotentialPointer(const ra::services::PointerFinder::PotentialPointer& pPotentialPointer, ra::data::Memory::Size nSize)
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();

    auto& pPointer = m_vResults.Add();
    pPointer.m_nAddress = pPotentialPointer.nRootAddress;
    pPointer.SetPointerAddress(pMemoryContext.FormatAddress(pPointer.m_nAddress));

    auto nOffset = pPotentialPointer.vOffsets.front();
    pPointer.m_nOffset = nOffset;
    pPointer.SetOffset(ra::util::String::Printf(L"+0x%02X", nOffset));

    std::array<ra::data::ByteAddress, NUM_STATES> vPointerAddress = {};
    for (gsl::index nStateIndex = 0; nStateIndex < NUM_STATES; ++nStateIndex)
    {
        const auto& pState = m_vStates.at(nStateIndex);
        if (!pState.CanCapture())
        {
            const auto* pValue = m_vStates.at(nStateIndex).CapturedMemory().GetValue(pPointer.m_nAddress);
            if (pValue)
            {
                pPointer.SetPointerValue(nStateIndex, ra::data::Memory::FormatValue(pValue->nValue, nSize, ra::data::Memory::Format::Hex));
                vPointerAddress.at(nStateIndex) = pValue->nValueAsAddress + nOffset;
            }
        }
    }

    for (gsl::index nOffsetIndex = 1; nOffsetIndex < gsl::narrow_cast<gsl::index>(pPotentialPointer.nOffsetLength); ++nOffsetIndex)
    {
        auto& pOffset = m_vResults.Add();
        nOffset = pPotentialPointer.vOffsets.at(nOffsetIndex);
        pOffset.m_bIsChild = true;
        pOffset.m_nOffset = nOffset;
        pOffset.SetOffset(ra::util::String::Printf(L"+0x%02X", nOffset));

        for (gsl::index nStateIndex = 0; nStateIndex < NUM_STATES; ++nStateIndex)
        {
            auto nPointerAddress = vPointerAddress.at(nStateIndex);
            if (nPointerAddress)
            {
                const auto* pValue = m_vStates.at(nStateIndex).CapturedMemory().GetValue(nPointerAddress);
                if (pValue)
                {
                    pOffset.SetPointerValue(nStateIndex, ra::data::Memory::FormatValue(pValue->nValue, nSize, ra::data::Memory::Format::Hex));
                    vPointerAddress.at(nStateIndex) = pValue->nValueAsAddress + nOffset;
                }
            }
        }
    }
}

ra::data::Memory::Size PointerFinderViewModel::GetSearchSize() const
{
    auto nSize = ra::data::Memory::Size::ThirtyTwoBit;
    switch (GetSearchType())
    {
        case ra::services::SearchType::SixteenBit:
        case ra::services::SearchType::SixteenBitAligned:
        case ra::services::SearchType::SixteenBitBigEndian:
            nSize = ra::data::Memory::Size::SixteenBit;
            break;
    }

    return nSize;
}

void PointerFinderViewModel::BookmarkSelected() const
{
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    std::vector<gsl::index> vSelectedItems;
    GetSelectedItems(vSelectedItems);
    if (vSelectedItems.empty())
        return;

    ConvertResultsToAchievementLogic(vSelectedItems,
        [&vmBookmarks](const std::string& sSerialized)
        {
            vmBookmarks.AddBookmark(sSerialized);
        });
}

void PointerFinderViewModel::CopySelectedToClipboard() const
{
    std::vector<gsl::index> vSelectedItems;
    GetSelectedItems(vSelectedItems);
    if (vSelectedItems.empty())
        return;

    if (vSelectedItems.size() > 1)
    {
        MessageBoxViewModel::ShowErrorMessage(L"Multiple items selected", L"Only one item can be copied at a time.");
        return;
    }

    ConvertResultsToAchievementLogic(vSelectedItems,
        [](const std::string& sSerialized)
        {
            auto& pClipboard = ra::services::ServiceLocator::Get<ra::services::IClipboard>();
            pClipboard.SetText(ra::util::String::Widen(sSerialized));
        });
}

void PointerFinderViewModel::GetSelectedItems(std::vector<gsl::index>& nIndices) const
{
    gsl::index nStartIndex = 0;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vResults.Count()); ++nIndex)
    {
        const auto* pItem = m_vResults.GetItemAt(nIndex);
        Expects(pItem != nullptr);

        if (!pItem->m_bIsChild)
            nStartIndex = nIndex;

        if (pItem->IsSelected())
        {
            if (nIndices.empty() || nIndices.back() != nStartIndex)
                nIndices.push_back(nStartIndex);
        }
    }
}

void PointerFinderViewModel::ConvertResultsToAchievementLogic(const std::vector<gsl::index>& nIndices, std::function<void(const std::string& sSerialized)> fCallback) const
{
    auto nSize = ra::data::Memory::Size::ThirtyTwoBit;
    uint32_t nMask = 0xFFFFFFFF;
    uint32_t nOffset = 0;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    if (!pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
    {
        nSize = GetSearchSize();
        nMask = 0xFFFFFFFF;
        nOffset = pConsoleContext.RealAddressFromByteAddress(0);
        if (nOffset == 0xFFFFFFFF)
            nOffset = 0;
    }
    else if (nMask != 0xFFFFFFFF)
    {
        const auto nBitsMask = ra::to_unsigned((1 << ra::data::Memory::SizeBits(nSize)) - 1);
        if (nBitsMask == nMask)
            nMask = 0xFFFFFFFF; // indicate masking is not needed
    }

    for (auto nIndex : nIndices)
    {
        const auto* pItem = m_vResults.GetItemAt(nIndex);
        Expects(pItem != nullptr);

        uint32_t nAddress = gsl::narrow_cast<uint32_t>(pItem->m_nAddress);
        std::string sBuffer;
        do
        {
            ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, ra::services::TriggerConditionType::AddAddress);
            ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Address, nSize, nAddress);

            if (nOffset != 0)
            {
                ra::services::AchievementLogicSerializer::AppendOperator(sBuffer, ra::services::TriggerOperatorType::Subtract);
                ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nOffset);
            }
            else if (nMask != 0xFFFFFFFF)
            {
                ra::services::AchievementLogicSerializer::AppendOperator(sBuffer, ra::services::TriggerOperatorType::BitwiseAnd);
                ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nMask);
            }

            ra::services::AchievementLogicSerializer::AppendConditionSeparator(sBuffer);

            const PotentialPointerViewModel* pNextItem = m_vResults.GetItemAt(++nIndex);
            if (pNextItem == nullptr || !pNextItem->m_bIsChild)
            {
                ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, ra::services::TriggerConditionType::Measured);
                ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, ra::services::TriggerOperandType::Address,
                    nSize, ra::to_unsigned(pItem->m_nOffset));
                break;
            }

            nAddress = pItem->m_nOffset;
            pItem = pNextItem;
        } while (true);

        fCallback(sBuffer);
    }
}

void PointerFinderViewModel::ExportResults() const
{
    if (m_vResults.Count() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Nothing to export");
        return;
    }

    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Export Pointer Results");
    vmFileDialog.AddFileType(L"CSV File", L"*.csv");
    vmFileDialog.SetDefaultExtension(L"csv");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    vmFileDialog.SetFileName(ra::util::String::Printf(L"%u-Pointers.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowSaveFileDialog() != ra::ui::DialogResult::OK)
        return;

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pTextWriter = pFileSystem.CreateTextFile(vmFileDialog.GetFileName());
    if (pTextWriter == nullptr)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            ra::util::String::Printf(L"Could not create %s", vmFileDialog.GetFileName()));
        return;
    }

    pTextWriter->Write("Address,Offset");
    for (size_t i = 0; i < m_vStates.size(); i++)
    {
        const auto& pStateI = m_vStates.at(i);
        if (pStateI.CanCapture())
            continue;

        pTextWriter->Write(",State");
        pTextWriter->Write(std::to_string(i + 1));
    }
    pTextWriter->WriteLine();

    for (const auto& pRow : m_vResults)
    {
        pTextWriter->Write(pRow.GetPointerAddress());
        pTextWriter->Write(",");
        pTextWriter->Write(pRow.GetOffset());

        for (gsl::index nState = 0; nState < gsl::narrow_cast<gsl::index>(m_vStates.size()); nState++)
        {
            const auto& sValue = pRow.GetPointerValue(nState);
            if (!sValue.empty()) {
                pTextWriter->Write(",");
                pTextWriter->Write(sValue);
            }
        }

        pTextWriter->WriteLine();
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
