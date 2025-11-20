#include "MemoryRegionsViewModel.hh"

#include "RA_Defs.h"
#include "util\Strings.hh"

#include "data\context\ConsoleContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\AssetUploadViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const BoolModelProperty MemoryRegionsViewModel::CanRemoveRegionProperty("MemoryRegionsViewModel", "CanRemoveRegion", false);
const BoolModelProperty MemoryRegionsViewModel::CanSaveProperty("MemoryRegionsViewModel", "CanSave", true);
const IntModelProperty MemoryRegionsViewModel::SelectedRegionIndexProperty("MemoryRegionsViewModel", "SelectedRegionIndexProperty", -1);

const StringModelProperty MemoryRegionsViewModel::MemoryRegionViewModel::RangeProperty("MemoryRegionViewModel", "Range", L"");
const BoolModelProperty MemoryRegionsViewModel::MemoryRegionViewModel::IsCustomProperty("MemoryRegionViewModel", "IsCustom", false);
const BoolModelProperty MemoryRegionsViewModel::MemoryRegionViewModel::IsInvalidProperty("MemoryRegionViewModel", "IsInvalid", false);

MemoryRegionsViewModel::MemoryRegionsViewModel()
{
    SetWindowTitle(L"Memory Regions");
}

void MemoryRegionsViewModel::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryRegionViewModel::RangeProperty)
    {
        auto* pItem = m_vRegions.GetItemAt(nIndex);
        if (pItem)
        {
            ra::ByteAddress nStartAddress = 0, nEndAddress = 0;
            if (!ra::data::models::MemoryRegionsModel::ParseFilterRange(args.tNewValue, nStartAddress, nEndAddress))
            {
                pItem->SetInvalid(true);
                SetValue(CanSaveProperty, false);
            }
            else
            {
                pItem->SetInvalid(false);
                UpdateCanSave();
            }
        }
    }
}

void MemoryRegionsViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryRegionViewModel::IsSelectedProperty)
    {
        if (args.tNewValue)
        {
            SetSelectedRegionIndex(gsl::narrow_cast<int>(nIndex));
        }
        else
        {
            int nNewSelection = -1;
            for (const auto& vmRegion : m_vRegions)
            {
                if (vmRegion.IsSelected())
                {
                    nNewSelection = vmRegion.GetId();
                    break;
                }
            }

            SetSelectedRegionIndex(nNewSelection);
        }
    }
}

void MemoryRegionsViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SelectedRegionIndexProperty)
    {
        bool bFound = false;
        for (auto& pRegion : m_vRegions)
        {
            if (pRegion.GetId() == args.tNewValue)
            {
                pRegion.SetSelected(true);
                SetValue(CanRemoveRegionProperty, pRegion.IsCustom());
                bFound = true;
            }
            else
            {
                pRegion.SetSelected(false);
            }
        }

        if (!bFound)
            SetValue(CanRemoveRegionProperty, false);
    }
    else if (args.Property == DialogResultProperty)
    {
        if (args.tNewValue == ra::etoi(DialogResult::OK))
            SaveCustomRegions();
    }

    WindowViewModelBase::OnValueChanged(args);
}

void MemoryRegionsViewModel::AddNewRegion()
{
    auto pItem = std::make_unique<MemoryRegionViewModel>();
    pItem->SetId(gsl::narrow_cast<int>(m_vRegions.Count()));
    pItem->SetLabel(L"New Custom Region");
    pItem->SetRange(ra::StringPrintf(L"%s-%s",
        ra::ByteAddressToString(0),
        ra::ByteAddressToString(0)));
    pItem->SetCustom(true);

    m_vRegions.Append(std::move(pItem));

    SetSelectedRegionIndex(gsl::narrow_cast<int>(m_vRegions.Count()) - 1);
}

void MemoryRegionsViewModel::RemoveRegion()
{
    auto nIndex = GetSelectedRegionIndex();
    auto* pItem = m_vRegions.GetItemAt(nIndex);
    if (pItem == nullptr || !pItem->IsCustom())
        return;

    const bool bWasInvalid = pItem->IsInvalid();

    // remove the item
    m_vRegions.RemoveAt(nIndex);

    // update the IDs on the remaining items
    while (nIndex < gsl::narrow_cast<int>(m_vRegions.Count()))
        pItem->SetId(nIndex++);

    // clear the selected item index
    SetSelectedRegionIndex(-1);

    if (bWasInvalid)
        UpdateCanSave();
}

void MemoryRegionsViewModel::InitializeRegions()
{
    for (const auto& pRegion : ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>().MemoryRegions())
    {
        switch (pRegion.Type)
        {
            case ra::data::context::ConsoleContext::AddressType::Unused:
            case ra::data::context::ConsoleContext::AddressType::VirtualRAM:
            case ra::data::context::ConsoleContext::AddressType::ReadOnlyMemory:
            case ra::data::context::ConsoleContext::AddressType::HardwareController:
            case ra::data::context::ConsoleContext::AddressType::VideoRAM:
                continue;
        }

        auto pItem = std::make_unique<MemoryRegionViewModel>();
        pItem->SetId(gsl::narrow_cast<int>(m_vRegions.Count()));
        pItem->SetLabel(ra::Widen(pRegion.Description));
        pItem->SetRange(ra::StringPrintf(L"%s-%s",
            ra::ByteAddressToString(pRegion.StartAddress),
            ra::ByteAddressToString(pRegion.EndAddress)));

        m_vRegions.Append(std::move(pItem));
    }

    const auto* pMemoryRegions = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().Assets().FindMemoryRegions();
    if (pMemoryRegions)
    {
        for (const auto& pRegion : pMemoryRegions->CustomRegions())
        {
            auto pItem = std::make_unique<MemoryRegionViewModel>();
            pItem->SetId(gsl::narrow_cast<int>(m_vRegions.Count()));
            pItem->SetLabel(ra::Widen(pRegion.sLabel));
            pItem->SetRange(ra::StringPrintf(L"%s-%s",
                ra::ByteAddressToString(pRegion.nStartAddress),
                ra::ByteAddressToString(pRegion.nEndAddress)));
            pItem->SetCustom(true);

            m_vRegions.Append(std::move(pItem));
        }
    }

    m_vRegions.AddNotifyTarget(*this);
}

void MemoryRegionsViewModel::UpdateCanSave()
{
    bool hasInvalid = false;
    for (const auto& pRegion : m_vRegions)
    {
        if (pRegion.IsInvalid())
        {
            hasInvalid = true;
            break;
        }
    }

    SetValue(CanSaveProperty, !hasInvalid);
}

void MemoryRegionsViewModel::SaveCustomRegions()
{
    auto& pAssets = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>().Assets();
    auto* pMemoryRegions = pAssets.FindMemoryRegions();
    if (pMemoryRegions == nullptr)
    {
        auto pNewMemoryRegions = std::make_unique<ra::data::models::MemoryRegionsModel>();
        pMemoryRegions = dynamic_cast<ra::data::models::MemoryRegionsModel*>(&pAssets.Append(std::move(pNewMemoryRegions)));
    }

    pMemoryRegions->ResetCustomRegions();
    for (const auto& pRegion : m_vRegions)
    {
        if (!pRegion.IsCustom())
            continue;

        ra::ByteAddress nStartAddress, nEndAddress;
        if (ra::data::models::MemoryRegionsModel::ParseFilterRange(pRegion.GetRange(), nStartAddress, nEndAddress))
            pMemoryRegions->AddCustomRegion(nStartAddress, nEndAddress, pRegion.GetLabel());
    }

    std::vector<ra::data::models::AssetModelBase*> vToSave;
    vToSave.push_back(pMemoryRegions);
    pAssets.SaveAssets(vToSave);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
