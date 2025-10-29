#include "MemoryRegionsDialog.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::MemoryRegionsViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool MemoryRegionsDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const MemoryRegionsViewModel*>(&vmViewModel) != nullptr);
}

void MemoryRegionsDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmMemoryRegions = dynamic_cast<MemoryRegionsViewModel*>(&vmViewModel);
    Expects(vmMemoryRegions != nullptr);

    MemoryRegionsDialog oDialog(*vmMemoryRegions);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_MEMORYREGIONS), this, hParentWnd);
}

void MemoryRegionsDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryRegions = dynamic_cast<MemoryRegionsViewModel*>(&vmViewModel);
    Expects(vmMemoryRegions != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryRegionsDialog(*vmMemoryRegions));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_MEMORYREGIONS), this))
            RA_LOG_ERR("Could not create Code Notes dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryRegionsDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

class RegionGridTextColumnBinding : public ra::ui::win32::bindings::GridTextColumnBinding
{
public:
    RegionGridTextColumnBinding(const StringModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridTextColumnBinding(pBoundProperty)
    {
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        const auto& pItems = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding)->GetItems();
        if (!pItems.GetItemValue(pInfo.nItemIndex, MemoryRegionsViewModel::MemoryRegionViewModel::IsCustomProperty))
            return nullptr;

        ra::ui::win32::bindings::GridTextColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }
};

// ------------------------------------

MemoryRegionsDialog::MemoryRegionsDialog(MemoryRegionsViewModel& vmMemoryRegions)
    : DialogBase(vmMemoryRegions),
    m_bindRegions(vmMemoryRegions)
{
    //m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Code Notes");

    auto pLabelColumn = std::make_unique<RegionGridTextColumnBinding>(
        MemoryRegionsViewModel::MemoryRegionViewModel::LabelProperty);
    pLabelColumn->SetHeader(L"Address");
    pLabelColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 200);
    m_bindRegions.BindColumn(0, std::move(pLabelColumn));

    auto pRangeColumn = std::make_unique<RegionGridTextColumnBinding>(
        MemoryRegionsViewModel::MemoryRegionViewModel::RangeProperty);
    pRangeColumn->SetHeader(L"Range");
    pRangeColumn->SetWidth(GridColumnBinding::WidthType::Fill, 50);
    m_bindRegions.BindColumn(1, std::move(pRangeColumn));

    m_bindRegions.BindItems(vmMemoryRegions.Regions());
    m_bindRegions.BindIsSelected(MemoryRegionsViewModel::MemoryRegionViewModel::IsSelectedProperty);

    m_bindWindow.BindEnabled(IDC_RA_REMOVE_REGION, MemoryRegionsViewModel::CanRemoveRegionProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LBX_REGIONS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADD_REGION, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_REMOVE_REGION, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDOK, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDCANCEL, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(400, 200);
}

BOOL MemoryRegionsDialog::OnInitDialog()
{
    m_bindRegions.SetControl(*this, IDC_RA_LBX_REGIONS);

    return DialogBase::OnInitDialog();
}

BOOL MemoryRegionsDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_ADD_REGION:
        {
            auto* vmMemoryRegions = dynamic_cast<MemoryRegionsViewModel*>(&m_vmWindow);
            if (vmMemoryRegions)
                vmMemoryRegions->AddNewRegion();

            return TRUE;
        }

        case IDC_RA_REMOVE_REGION:
        {
            auto* vmMemoryRegions = dynamic_cast<MemoryRegionsViewModel*>(&m_vmWindow);
            if (vmMemoryRegions)
                vmMemoryRegions->RemoveRegion();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
