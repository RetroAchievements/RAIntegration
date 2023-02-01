#include "PointerFinderViewModel.hh"

#include "RA_Defs.h"

#include "data/context/EmulatorContext.hh"
#include "data/context/GameContext.hh"

#include "services/IFileSystem.hh"

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

void PointerFinderViewModel::StateViewModel::DoFrame()
{
    if (CanCapture())
        m_pViewer.DoFrame();
}

void PointerFinderViewModel::StateViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == AddressProperty)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(GetAddress()));
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
    if (ra::services::ServiceLocator::Get<ra::data::context::GameContext>().GameId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"No game loaded.", L"Cannot capture memory without a loaded game.");
        return;
    }

    const auto& sAddress = GetAddress();
    const auto nAddress = ra::ByteAddressFromString(ra::Narrow(sAddress));
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

    const auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    const auto nMemorySize = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    m_pCapture.reset(new ra::services::SearchResults());
    m_pCapture->Initialize(0, nMemorySize, m_pOwner->GetSearchType());

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

static std::wstring FormatValue(const ra::services::SearchResults& srResults, ra::ByteAddress nAddress)
{
    auto sValue = srResults.GetFormattedValue(nAddress, srResults.GetSize());
    if (sValue.at(1) == 'x' && sValue.at(0) == '0')
        sValue.erase(0, 2);

    return sValue;
}

void PointerFinderViewModel::Find()
{
    bool bPerformedSearch = false;

    // TODO: capture/restore selected address

    m_vResults.BeginUpdate();
    m_vResults.Clear();

    for (size_t i = 0; i < m_vStates.size(); i++)
    {
        const auto& pStateI = m_vStates.at(i);
        if (pStateI.CanCapture())
            continue;

        for (size_t j = i + 1; j < m_vStates.size(); j++)
        {
            const auto& pStateJ = m_vStates.at(j);
            if (pStateJ.CanCapture())
                continue;

            const auto nAddressI = pStateI.Viewer().GetAddress();
            const auto nAddressJ = pStateJ.Viewer().GetAddress();
            if (nAddressI == nAddressJ)
                continue;

            // flag all items as having not been seen - anything that's still not seen after merging will be discarded
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vResults.Count()) - 1; nIndex >= 0; nIndex--)
                m_vResults.GetItemAt(nIndex)->m_bMatched = false;

            // compare the two memory states
            ra::services::SearchResults pResults;
            auto pReadMemory = [srSecond = pStateJ.CapturedMemory()](ra::ByteAddress nAddress, uint8_t* pBuffer, size_t nBufferSize) noexcept {
                srSecond->GetBytes(nAddress, pBuffer, nBufferSize);
            };
            if (nAddressI > nAddressJ)
                pResults.Initialize(*pStateI.CapturedMemory(), pReadMemory, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValueMinus, std::to_wstring(nAddressI - nAddressJ));
            else
                pResults.Initialize(*pStateI.CapturedMemory(), pReadMemory, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValuePlus, std::to_wstring(nAddressJ - nAddressI));

            // merge the new potential items into the list
            gsl::index nResultIndex = 0;
            for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pResults.MatchingAddressCount()); nIndex++)
            {
                ra::services::SearchResults::Result pResult;
                if (pResults.GetMatchingAddress(nIndex, pResult))
                {
                    PotentialPointerViewModel* pPointer = nullptr;
                    while (nResultIndex < gsl::narrow_cast<gsl::index>(m_vResults.Count()))
                    {
                        PotentialPointerViewModel* pItem = m_vResults.GetItemAt(nResultIndex);
                        if (pItem->m_nAddress > pResult.nAddress)
                            break;

                        if (pItem->m_nAddress == pResult.nAddress)
                        {
                            pPointer = pItem;
                            nResultIndex++;
                            break;
                        }

                        nResultIndex++;
                    }

                    if (pPointer == nullptr)
                    {
                        if (bPerformedSearch) // new address not in set, ignore
                            continue;

                        pPointer = &m_vResults.Add();
                        pPointer->m_nAddress = pResult.nAddress;
                        pPointer->SetPointerAddress(ra::Widen(ra::ByteAddressToString(pResult.nAddress)));
                        const auto nOffset = (nAddressJ - pResult.nValue);
                        pPointer->SetOffset(ra::StringPrintf(L"+0x%02X", nOffset));

                        pPointer->SetPointerValue(i, FormatValue(*pStateI.CapturedMemory(), pResult.nAddress));
                    }
                    else if (pPointer->GetPointerValue(i).empty())
                    {
                        pPointer->SetPointerValue(i, FormatValue(*pStateI.CapturedMemory(), pResult.nAddress));
                    }

                    pPointer->SetPointerValue(j, FormatValue(*pStateJ.CapturedMemory(), pResult.nAddress));
                    pPointer->m_bMatched = true;
                }
            }

            // remove any items that didn't also match the most recent search
            if (bPerformedSearch)
            {
                for (gsl::index nIndex = m_vResults.Count() - 1; nIndex >= 0; nIndex--)
                {
                    if (!m_vResults.GetItemAt(nIndex)->m_bMatched)
                        m_vResults.RemoveAt(nIndex);
                }
            }

            bPerformedSearch = true;
        }
    }

    if (m_vResults.Count() == 0 && bPerformedSearch)
    {
        auto* pPointer = &m_vResults.Add();
        pPointer->SetPointerAddress(L"No pointers found.");
    }

    m_vResults.EndUpdate();

    SetValue(ResultCountTextProperty, std::to_wstring(m_vResults.Count()));

    if (!bPerformedSearch)
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(L"Cannot find.", L"At least two unique addresses must be captured before potential pointers can be located.");
}

void PointerFinderViewModel::BookmarkSelected()
{
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vResults.Count()); nIndex++)
    {
        const auto* pItem = m_vResults.GetItemAt(nIndex);
        Expects(pItem != nullptr);
        if (pItem->IsSelected())
        {
            MemSize nSize = MemSize::ThirtyTwoBit;
            switch (GetSearchType())
            {
                case ra::services::SearchType::SixteenBit:
                case ra::services::SearchType::SixteenBitAligned:
                case ra::services::SearchType::SixteenBitBigEndian:
                    nSize = MemSize::SixteenBit;
                    break;
            }

            vmBookmarks.AddBookmark(pItem->m_nAddress, nSize);
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
    vmFileDialog.SetFileName(ra::StringPrintf(L"%u-Pointers.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowSaveFileDialog() != ra::ui::DialogResult::OK)
        return;

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pTextWriter = pFileSystem.CreateTextFile(vmFileDialog.GetFileName());
    if (pTextWriter == nullptr)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            ra::StringPrintf(L"Could not create %s", vmFileDialog.GetFileName()));
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

    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vResults.Count(); ++nIndex)
    {
        const auto* pRow = m_vResults.GetItemAt(nIndex);
        Expects(pRow != nullptr);
        pTextWriter->Write(pRow->GetPointerAddress());
        pTextWriter->Write(",");
        pTextWriter->Write(pRow->GetOffset());

        for (gsl::index nState = 0; nState < gsl::narrow_cast<gsl::index>(m_vStates.size()); nState++)
        {
            const auto& sValue = pRow->GetPointerValue(nState);
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
