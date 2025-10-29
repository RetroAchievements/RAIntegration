#include "MemoryRegionsViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

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
const IntModelProperty MemoryRegionsViewModel::SelectedRegionIndexProperty("MemoryRegionsViewModel", "SelectedRegionIndexProperty", -1);

const StringModelProperty MemoryRegionsViewModel::MemoryRegionViewModel::RangeProperty("MemoryRegionViewModel", "Range", L"");
const BoolModelProperty MemoryRegionsViewModel::MemoryRegionViewModel::IsCustomProperty("MemoryRegionViewModel", "IsCustom", false);

MemoryRegionsViewModel::MemoryRegionsViewModel() noexcept
{
    SetWindowTitle(L"Memory Regions");

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

    // TODO: add custom regions
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
        const auto nIndex = m_vRegions.FindItemIndex(MemoryRegionViewModel::IdProperty, args.tNewValue);
        if (nIndex == -1)
        {
            SetValue(CanRemoveRegionProperty, false);
        }
        else
        {
            const auto* pItem = m_vRegions.GetItemAt(nIndex);
            Expects(pItem != nullptr);
            SetValue(CanRemoveRegionProperty, pItem->IsCustom());
        }
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

    SetSelectedRegionIndex(m_vRegions.Count() - 1);
}

void MemoryRegionsViewModel::RemoveRegion()
{
    auto nIndex = GetSelectedRegionIndex();
    auto* pItem = m_vRegions.GetItemAt(nIndex);
    if (pItem == nullptr || !pItem->IsCustom())
        return;

    // remove the item
    m_vRegions.RemoveAt(nIndex);

    // update the IDs on the remaining items
    while (nIndex < m_vRegions.Count())
        pItem->SetId(nIndex++);

    // clear the selected item index
    SetSelectedRegionIndex(-1);
}

void MemoryRegionsViewModel::SaveCustomRegions()
{
    // TODO: save custom regions
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
