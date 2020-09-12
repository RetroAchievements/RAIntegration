#include "AssetListDialog.hh"

#include "RA_Resource.h"

#include "data\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::AssetListViewModel;
using ra::ui::viewmodels::AssetViewModelBase;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool AssetListDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const AssetListViewModel*>(&vmViewModel) != nullptr);
}

void AssetListDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void AssetListDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<AssetListViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new AssetListDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTS), this))
            RA_LOG_ERR("Could not create Asset List dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void AssetListDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

AssetListDialog::AssetListDialog(AssetListViewModel& vmAssetList)
    : DialogBase(vmAssetList),
      m_bindAssets(vmAssetList),
      m_bindProcessingActive(vmAssetList)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Achievements");
    m_bindAssets.BindIsSelected(AssetViewModelBase::IsSelectedProperty);

    auto pNameColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        AssetViewModelBase::NameProperty);
    pNameColumn->SetHeader(L"Name");
    pNameColumn->SetWidth(GridColumnBinding::WidthType::Fill, 100);
    m_bindAssets.BindColumn(0, std::move(pNameColumn));

    auto pPointsColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
       ra::ui::viewmodels::AchievementViewModel::PointsProperty);
    pPointsColumn->SetHeader(L"Points");
    pPointsColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 50);
    pPointsColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindAssets.BindColumn(1, std::move(pPointsColumn));

    auto pStateColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        AssetViewModelBase::StateProperty, vmAssetList.States());
    pStateColumn->SetHeader(L"State");
    pStateColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    pStateColumn->SetReadOnly(false);
    m_bindAssets.BindColumn(2, std::move(pStateColumn));

    auto pCategoryColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        AssetViewModelBase::CategoryProperty, vmAssetList.Categories());
    pCategoryColumn->SetHeader(L"Category");
    pCategoryColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindAssets.BindColumn(3, std::move(pCategoryColumn));

    auto pChangesColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        AssetViewModelBase::ChangesProperty, vmAssetList.Changes());
    pChangesColumn->SetHeader(L"Changes");
    pChangesColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindAssets.BindColumn(4, std::move(pChangesColumn));

    m_bindAssets.BindItems(vmAssetList.Assets());

    m_bindWindow.BindLabel(IDC_RA_GAMEHASH, AssetListViewModel::GameIdProperty);
    m_bindWindow.BindLabel(IDC_RA_NUMACH, AssetListViewModel::AchievementCountProperty);
    m_bindWindow.BindLabel(IDC_RA_POINT_TOTAL, AssetListViewModel::TotalPointsProperty);

    m_bindProcessingActive.BindCheck(AssetListViewModel::IsProcessingActiveProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LISTACHIEVEMENTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(640, 288);
}

BOOL AssetListDialog::OnInitDialog()
{
    m_bindAssets.SetControl(*this, IDC_RA_LISTACHIEVEMENTS);
    m_bindProcessingActive.SetControl(*this, IDC_RA_CHKACHPROCESSINGACTIVE);

    return DialogBase::OnInitDialog();
}

BOOL AssetListDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_ADD_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->CreateNew();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
