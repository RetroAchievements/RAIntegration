#include "AssetListDialog.hh"

#include "RA_Resource.h"

#include "data\context\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::data::models::AssetModelBase;
using ra::ui::viewmodels::AssetListViewModel;
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

class IdColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    IdColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nId = vmItems.GetItemValue(nIndex, *m_pBoundProperty);

        if (nId >= ra::data::context::GameAssets::FirstLocalId)
            return L"0";

        return std::to_wstring(nId);
    }
};

// ------------------------------------

class PointsColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    PointsColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nType = ra::itoe<ra::data::models::AssetType>(vmItems.GetItemValue(nIndex, ra::data::models::AssetModelBase::TypeProperty));
        if (nType == ra::data::models::AssetType::Achievement)
        {
            const auto nPoints = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
            return std::to_wstring(nPoints);
        }

        return L"";
    }
};

// ------------------------------------

class IconColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    IconColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nType = ra::itoe<ra::data::models::AssetType>(vmItems.GetItemValue(nIndex, *m_pBoundProperty));
        switch (nType)
        {
            case ra::data::models::AssetType::Achievement:
                return L"\U0001F3C6"; // 1F3C6 - Trophy

            case ra::data::models::AssetType::Leaderboard:
                return L"\U0001F4CA"; // 1F4CA - Bar Chart

            //case ra::data::models::AssetType::RichPresence:
            //    return L"\U0001F4C4"; // 1F4C4 - Document
            //    return L"\U0001F4CD"; // 1F4CD - Round Pushpin
            //    return L"\U0001F4DC"; // 1F4DC - Scroll
            //    return L"\U0001F522"; // 1F5FA - World Map

            default:
                return std::to_wstring(ra::etoi(nType));
        }
    }

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nType = ra::itoe<ra::data::models::AssetType>(vmItems.GetItemValue(nIndex, *m_pBoundProperty));
        switch (nType)
        {
            case ra::data::models::AssetType::Achievement:
                return L"Achievement";

            case ra::data::models::AssetType::Leaderboard:
                return L"Leaderboard";

            default:
                return std::to_wstring(ra::etoi(nType));
        }
    }
};

// ------------------------------------

class ChangesColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    ChangesColumnBinding(const IntModelProperty& pBoundProperty, const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept
        : ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems)
    {
    }

    bool DependsOn(const ra::ui::StringModelProperty& pProperty) const noexcept override
    {
        return pProperty == AssetModelBase::ValidationErrorProperty;
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        const std::wstring& sLabel = m_vmItems.GetLabelForId(nValue);

        const auto& sWarning = vmItems.GetItemValue(nIndex, AssetModelBase::ValidationErrorProperty);
        if (sWarning.empty())
            return sLabel;

        return sLabel + L"\U000026A0"; // 26A0 - Warning Sign
    }
};

// ------------------------------------

AssetListDialog::AssetListDialog(AssetListViewModel& vmAssetList)
    : DialogBase(vmAssetList),
      m_bindAssets(vmAssetList),
      m_bindCategories(vmAssetList),
      m_bindSpecialFilters(vmAssetList),
      m_bindAssetTypeFilters(vmAssetList),
      m_bindProcessingActive(vmAssetList),
      m_bindKeepActive(vmAssetList)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Near, RelativePosition::After, "Achievements");

    m_bindCategories.BindItems(vmAssetList.Categories(), ra::ui::viewmodels::LookupItemViewModel::IdProperty,
        ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    m_bindCategories.BindSelectedItem(AssetListViewModel::FilterCategoryProperty);

    m_bindSpecialFilters.BindItems(vmAssetList.SpecialFilters(), ra::ui::viewmodels::LookupItemViewModel::IdProperty,
       ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    m_bindSpecialFilters.BindSelectedItem(AssetListViewModel::SpecialFilterProperty);

    m_bindAssetTypeFilters.BindItems(vmAssetList.AssetTypeFilters(), ra::ui::viewmodels::LookupItemViewModel::IdProperty,
        ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    m_bindAssetTypeFilters.BindSelectedItem(AssetListViewModel::AssetTypeFilterProperty);

#ifdef ASSET_ICONS
    constexpr int nOffset = 1;

    auto pIconColumn = std::make_unique<IconColumnBinding>(AssetModelBase::TypeProperty);
    pIconColumn->SetHeader(L"Type");
    pIconColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 24);
    m_bindAssets.BindColumn(0, std::move(pIconColumn));
#else
    constexpr int nOffset = 0;
#endif

    auto pIdColumn = std::make_unique<IdColumnBinding>(AssetListViewModel::AssetSummaryViewModel::IdProperty);
    pIdColumn->SetHeader(L"ID");
    pIdColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 49);
    pIdColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindAssets.BindColumn(0 + nOffset, std::move(pIdColumn));

    auto pNameColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        AssetListViewModel::AssetSummaryViewModel::LabelProperty);
    pNameColumn->SetHeader(L"Name");
    pNameColumn->SetWidth(GridColumnBinding::WidthType::Fill, 100);
    m_bindAssets.BindColumn(1 + nOffset, std::move(pNameColumn));

    auto pPointsColumn = std::make_unique<PointsColumnBinding>(ra::data::models::AchievementModel::PointsProperty);
    pPointsColumn->SetHeader(L"Points");
    pPointsColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 45);
    pPointsColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindAssets.BindColumn(2 + nOffset, std::move(pPointsColumn));

    auto pStateColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        AssetModelBase::StateProperty, vmAssetList.States());
    pStateColumn->SetHeader(L"State");
    pStateColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 70);
    m_bindAssets.BindColumn(3 + nOffset, std::move(pStateColumn));

    auto pCategoryColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        AssetModelBase::CategoryProperty, vmAssetList.Categories());
    pCategoryColumn->SetHeader(L"Category");
    pCategoryColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 70);
    m_bindAssets.BindColumn(4 + nOffset, std::move(pCategoryColumn));

    auto pChangesColumn = std::make_unique<ChangesColumnBinding>(
        AssetModelBase::ChangesProperty, vmAssetList.Changes());
    pChangesColumn->SetHeader(L"Changes");
    pChangesColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 82);
    m_bindAssets.BindColumn(5 + nOffset, std::move(pChangesColumn));

    m_bindAssets.SetDoubleClickHandler([this](gsl::index nIndex)
    {
        auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
        const auto* pSummary = vmAssets->FilteredAssets().GetItemAt(nIndex);
        if (pSummary)
            vmAssets->OpenEditor(pSummary);
    });
    m_bindAssets.BindEnsureVisible(AssetListViewModel::EnsureVisibleAssetIndexProperty);
    m_bindAssets.BindItems(vmAssetList.FilteredAssets());
    m_bindAssets.BindIsSelected(AssetListViewModel::AssetSummaryViewModel::IsSelectedProperty);

    m_bindWindow.BindLabel(IDC_RA_GAMEHASH, AssetListViewModel::GameIdProperty);
    m_bindWindow.BindLabel(IDC_RA_NUMACH, AssetListViewModel::AchievementCountProperty);
    m_bindWindow.BindLabel(IDC_RA_POINT_TOTAL, AssetListViewModel::TotalPointsProperty);

    m_bindWindow.BindEnabled(IDC_RA_RESET_ACH, AssetListViewModel::CanActivateProperty);
    m_bindWindow.BindLabel(IDC_RA_RESET_ACH, AssetListViewModel::ActivateButtonTextProperty);
    m_bindWindow.BindLabel(IDC_RA_COMMIT_ACH, AssetListViewModel::SaveButtonTextProperty);
    m_bindWindow.BindEnabled(IDC_RA_COMMIT_ACH, AssetListViewModel::CanSaveProperty);
    m_bindWindow.BindLabel(IDC_RA_DOWNLOAD_ACH, AssetListViewModel::ResetButtonTextProperty);
    m_bindWindow.BindEnabled(IDC_RA_DOWNLOAD_ACH, AssetListViewModel::CanResetProperty);
    m_bindWindow.BindLabel(IDC_RA_REVERTSELECTED, AssetListViewModel::RevertButtonTextProperty);
    m_bindWindow.BindEnabled(IDC_RA_REVERTSELECTED, AssetListViewModel::CanRevertProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_ACH, AssetListViewModel::CanCreateProperty);
    m_bindWindow.BindEnabled(IDC_RA_CLONE_ACH, AssetListViewModel::CanCloneProperty);

    m_bindProcessingActive.BindCheck(AssetListViewModel::IsProcessingActiveProperty);
    m_bindKeepActive.BindCheck(AssetListViewModel::KeepActiveProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_CATEGORY, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SPECIAL_FILTER, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_GAMEID, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_GAMEHASH, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_ACHIEVEMENTS, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_NUMACH, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_POINTS, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_POINT_TOTAL, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CHKACHPROCESSINGACTIVE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LISTACHIEVEMENTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_RESET_ACH, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CHK_ACTIVE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_COMMIT_ACH, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DOWNLOAD_ACH, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_REVERTSELECTED, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_ADD_ACH, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_CLONE_ACH, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(640, 293);
}

BOOL AssetListDialog::OnInitDialog()
{
    m_bindAssets.SetControl(*this, IDC_RA_LISTACHIEVEMENTS);
    m_bindProcessingActive.SetControl(*this, IDC_RA_CHKACHPROCESSINGACTIVE);
    m_bindKeepActive.SetControl(*this, IDC_RA_CHK_ACTIVE);

    m_bindCategories.SetControl(*this, IDC_RA_CATEGORY);
    m_bindSpecialFilters.SetControl(*this, IDC_RA_SPECIAL_FILTER);
    m_bindAssetTypeFilters.SetControl(*this, IDC_RA_TYPE_FILTER);

#ifdef ASSET_ICONS
    m_bindAssets.InitializeTooltips(std::chrono::milliseconds(500));
#endif

    return DialogBase::OnInitDialog();
}

BOOL AssetListDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_RESET_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->ActivateSelected();

            return TRUE;
        }

        case IDC_RA_COMMIT_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->SaveSelected();

            return TRUE;
        }

        case IDC_RA_DOWNLOAD_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->ResetSelected();

            return TRUE;
        }

        case IDC_RA_REVERTSELECTED:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->RevertSelected();

            return TRUE;
        }

        case IDC_RA_ADD_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->CreateNew();

            return TRUE;
        }

        case IDC_RA_CLONE_ACH:
        {
            auto* vmAssets = dynamic_cast<AssetListViewModel*>(&m_vmWindow);
            if (vmAssets)
                vmAssets->CloneSelected();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
