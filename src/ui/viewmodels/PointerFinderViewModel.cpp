#include "PointerFinderViewModel.hh"

#include "RA_Defs.h"
#include "util\Strings.hh"

#include "context/IConsoleContext.hh"

#include "data/context/EmulatorContext.hh"
#include "data/context/GameContext.hh"

#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "ui/viewmodels/FileDialogViewModel.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty PointerFinderViewModel::ResultCountTextProperty("PointerFinderViewModel", "ResultCountText", L"0");
const IntModelProperty PointerFinderViewModel::SearchTypeProperty("PointerFinderViewModel", "SearchType", ra::etoi(ra::services::SearchType::ThirtyTwoBitAligned));

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
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto nMemorySize = gsl::narrow<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize());

        ra::services::SearchResults pInitialResults;
        pInitialResults.Initialize(0, nMemorySize, m_pOwner->GetSearchType());

        m_pCapture.reset(new ra::services::SearchResults());

        m_pCapture->Initialize(pInitialResults,
            [&pConsoleContext](const ra::services::SearchResult& pSearchResult) {
                if (pSearchResult.nValue == 0)
                    return false;

                return (pConsoleContext.ByteAddressFromRealAddress(pSearchResult.nValue) != 0xFFFFFFFF);
            });
    });

    SetValue(CaptureButtonTextProperty, L"Release");
    SetValue(CanCaptureProperty, false);
}

void PointerFinderViewModel::StateViewModel::ClearCapture()
{
    m_pCapture.reset();

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
    std::vector<PotentialPointerChain> vPotentialPointers;
    ra::data::Memory::Size nSize = ra::data::Memory::Size::Unknown;
    bool bPerformedSearch = false;

    // TODO: capture/restore selected address (find may be clicked again after changing captures)

    m_vResults.BeginUpdate();
    m_vResults.Clear();

    for (size_t i = 0; i < m_vStates.size(); i++)
    {
        const auto& pStateI = m_vStates.at(i);
        if (pStateI.CanCapture())
            continue;

        if (vPotentialPointers.empty())
        {
            FindBestChains(vPotentialPointers, pStateI, i);
            nSize = pStateI.CapturedMemory()->GetSize();
        }
        else
        {
            FindMatches(vPotentialPointers, pStateI, i);
            bPerformedSearch = true;
        }

        if (vPotentialPointers.empty())
            break;
    }

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    for (const auto& pPotentialPointer : vPotentialPointers)
    {
        auto& pPointer = m_vResults.Add();
        const auto& pNode = pPotentialPointer.vNodes.at(0);
        pPointer.m_nAddress = pNode.nAddress;
        pPointer.SetPointerAddress(pMemoryContext.FormatAddress(pPointer.m_nAddress));
        pPointer.SetOffset(ra::util::String::Printf(L"+0x%02X", pNode.nOffset));

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vStates.size()); ++nIndex)
        {
            const auto nValue = pNode.nValue.at(nIndex);
            if (nValue)
                pPointer.SetPointerValue(nIndex, ra::data::Memory::FormatValue(nValue, nSize, ra::data::Memory::Format::Hex));
        }
    }

    if (m_vResults.Count() == 0 && bPerformedSearch)
    {
        auto* pPointer = &m_vResults.Add();
        pPointer->SetPointerAddress(L"No pointers found.");
        SetValue(ResultCountTextProperty, L"0");
    }
    else
    {
        SetValue(ResultCountTextProperty, std::to_wstring(m_vResults.Count()));
    }

    m_vResults.EndUpdate();

    if (!bPerformedSearch)
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Cannot find.", L"At least two unique addresses must be captured before potential pointers can be located.");
}

void PointerFinderViewModel::FindBestChains(std::vector<PotentialPointerChain>& vPotentialPointers, const StateViewModel& pState, size_t nStateIndex)
{
    // extract all pointers from the search results
    const auto* pResults = pState.CapturedMemory();
    Expects(pResults != nullptr);

    std::vector<ra::data::ByteAddress> vPointerAddresses;
    GetPointerAddresses(vPointerAddresses, *pResults);

    // limit the results to addresses within MAX_OFFSET of the pointer value
    const auto nSearchAddress = pState.Viewer().GetAddress();
    const auto pRange = NarrowSearch(vPointerAddresses, nSearchAddress);

    // get things pointing to the filtered addresses
    FindPointers(vPotentialPointers, *pResults, nStateIndex, vPointerAddresses, pRange, nSearchAddress);
}

void PointerFinderViewModel::GetPointerAddresses(std::vector<ra::data::ByteAddress>& vPointerAddresses, const ra::services::SearchResults& pResults)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pResults.MatchingAddressCount()); nIndex++)
    {
        ra::services::SearchResult pResult;
        if (!pResults.GetMatchingAddress(nIndex, pResult))
            continue;

        // ignore null values
        if (pResult.nValue == 0)
            continue;

        // ignore non-pointer values
        const auto nPointerAddress = pConsoleContext.ByteAddressFromRealAddress(pResult.nValue);
        if (nPointerAddress == 0xFFFFFFFF)
            continue;

        // add to the list if not already there
        const auto pInsertIter = std::lower_bound(vPointerAddresses.begin(), vPointerAddresses.end(), nPointerAddress);
        if (pInsertIter == vPointerAddresses.end() || *pInsertIter != nPointerAddress)
            vPointerAddresses.insert(pInsertIter, nPointerAddress);
    }
}

PointerFinderViewModel::PointerAddressRange PointerFinderViewModel::NarrowSearch(
    const std::vector<ra::data::ByteAddress>& vPointerAddresses, ra::data::ByteAddress nSearchAddress)
{
    if (vPointerAddresses.size() == 0)
        return PointerAddressRange(vPointerAddresses.data(), vPointerAddresses.data());

    const auto pSearchIter = std::lower_bound(vPointerAddresses.begin(), vPointerAddresses.end(), nSearchAddress);
    auto pEnd = pSearchIter;
    auto pStart = pSearchIter;

    uint32_t nMaxOffset = MAX_OFFSET;
    for (int i = 0; i < 4; ++i, nMaxOffset *= 2)
    {
        while (pEnd < vPointerAddresses.end() && *pEnd - nSearchAddress <= nMaxOffset)
            ++pEnd;

        while (pStart > vPointerAddresses.begin() && nSearchAddress - *(pStart - 1) <= nMaxOffset)
            --pStart;

        const auto nMatches = gsl::narrow_cast<size_t>(pEnd - pStart);
        if (nMatches >= 8 || nMatches == vPointerAddresses.size())
            break;
    }

    return PointerAddressRange(vPointerAddresses.data() + (pStart - vPointerAddresses.begin()),
                               vPointerAddresses.data() + (pEnd - vPointerAddresses.begin()));
}

void PointerFinderViewModel::FindPointers(std::vector<PotentialPointerChain>& vPotentialPointers, const ra::services::SearchResults& pResults, size_t nStateIndex,
    const std::vector<ra::data::ByteAddress>& vPointerAddresses, PointerFinderViewModel::PointerAddressRange pRange, ra::data::ByteAddress nSearchAddress)
{
    if (pRange.first == pRange.second)
        return;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pResults.MatchingAddressCount()); nIndex++)
    {
        ra::services::SearchResult pResult;
        if (!pResults.GetMatchingAddress(nIndex, pResult))
            continue;

        // ignore null values
        if (pResult.nValue == 0)
            continue;

        // ignore non-pointer values
        const auto nPointerAddress = pConsoleContext.ByteAddressFromRealAddress(pResult.nValue);
        if (nPointerAddress == 0xFFFFFFFF)
            continue;

        // ignore pointers not in the search range
        if (!std::binary_search(pRange.first, pRange.second, nPointerAddress))
            continue;

        auto& pPointerChain = vPotentialPointers.emplace_back();
        auto& pPointer = pPointerChain.vNodes.emplace_back();
        memset(&pPointer, 0, sizeof(pPointer));
        pPointer.nAddress = pResult.nAddress;
        pPointer.nOffset = ra::to_signed(nSearchAddress) - nPointerAddress;
        pPointer.nValue.at(nStateIndex) = pResult.nValue;
    }
}

void PointerFinderViewModel::FindMatches(std::vector<PotentialPointerChain>& vPotentialPointers, const StateViewModel& pState, size_t nStateIndex)
{
    if (vPotentialPointers.empty())
        return;

    auto pPotentialPointer = vPotentialPointers.begin();
    auto nNextAddress = pPotentialPointer->vNodes.begin()->nAddress;
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();

    const auto* pResults = pState.CapturedMemory();
    Expects(pResults != nullptr);
    const auto nSearchAddress = pState.Viewer().GetAddress();

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pResults->MatchingAddressCount()); nIndex++)
    {
        ra::services::SearchResult pResult;
        if (!pResults->GetMatchingAddress(nIndex, pResult))
            continue;

        if (pResult.nAddress < nNextAddress)
            continue;

        bool bIsMatch = false;
        if (pResult.nAddress == nNextAddress && pResult.nValue != 0)
        {
            const auto nPointerAddress = pConsoleContext.ByteAddressFromRealAddress(pResult.nValue);
            if (nPointerAddress != 0xFFFFFFFF)
            {
                auto& pNode = *pPotentialPointer->vNodes.begin();
                if (nPointerAddress + pNode.nOffset == nSearchAddress)
                {
                    pNode.nValue.at(nStateIndex) = pResult.nValue;
                    bIsMatch = true;
                }
            }
        }

        pPotentialPointer->bPrune = !bIsMatch;
        ++pPotentialPointer;

        if (pPotentialPointer == vPotentialPointers.end())
            break;

        nNextAddress = pPotentialPointer->vNodes.begin()->nAddress;
    }

    vPotentialPointers.erase(std::remove_if(
        vPotentialPointers.begin(), vPotentialPointers.end(),
        [](const PotentialPointerChain& pChain) { return pChain.bPrune; }
    ), vPotentialPointers.end());
}

void PointerFinderViewModel::BookmarkSelected()
{
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    for (const auto& pItem : m_vResults)
    {
        if (pItem.IsSelected())
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

            vmBookmarks.AddBookmark(pItem.m_nAddress, nSize);
            break;
        }
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
