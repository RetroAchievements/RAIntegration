#include "AssetEditorDialog.hh"

#include "RA_Resource.h"

#include "data\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::AssetEditorViewModel;
using ra::ui::viewmodels::AssetViewModelBase;
using ra::ui::viewmodels::AchievementViewModel;
using ra::ui::viewmodels::TriggerViewModel;
using ra::ui::viewmodels::TriggerConditionViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

static constexpr int COLUMN_WIDTH_ID = 30;
static constexpr int COLUMN_WIDTH_FLAG = 75;
static constexpr int COLUMN_WIDTH_TYPE = 42;
static constexpr int COLUMN_WIDTH_SIZE = 54;
static constexpr int COLUMN_WIDTH_VALUE = 72;
static constexpr int COLUMN_WIDTH_OPERATOR = 35;
static constexpr int COLUMN_WIDTH_HITS = 84;

bool AssetEditorDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const AssetEditorViewModel*>(&vmViewModel) != nullptr);
}

void AssetEditorDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void AssetEditorDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<AssetEditorViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new AssetEditorDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTEDITOR), this))
            RA_LOG_ERR("Could not create Asset Editor dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void AssetEditorDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

AssetEditorDialog::AssetEditorDialog(AssetEditorViewModel& vmAssetList)
    : DialogBase(vmAssetList),
      m_bindID(vmAssetList),
      m_bindName(vmAssetList),
      m_bindDescription(vmAssetList),
      m_bindBadge(vmAssetList),
      m_bindPoints(vmAssetList),
      m_bindPauseOnReset(vmAssetList),
      m_bindPauseOnTrigger(vmAssetList),
      m_bindGroups(vmAssetList),
      m_bindTrigger(vmAssetList)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::After, "Achievement Editor");
    m_bindWindow.AddChildViewModel(vmAssetList.Trigger());

    m_bindID.BindValue(AssetEditorViewModel::IDProperty);
    m_bindName.BindText(AssetEditorViewModel::NameProperty);
    m_bindDescription.BindText(AssetEditorViewModel::DescriptionProperty);
    m_bindBadge.BindText(AssetEditorViewModel::BadgeProperty);
    m_bindPoints.BindValue(AssetEditorViewModel::PointsProperty);

    m_bindPauseOnReset.BindCheck(AssetEditorViewModel::PauseOnResetProperty);
    m_bindPauseOnTrigger.BindCheck(AssetEditorViewModel::PauseOnTriggerProperty);

    auto pGroupColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerViewModel::GroupViewModel::LabelProperty);
    pGroupColumn->SetHeader(L"Group");
    pGroupColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, 50);
    m_bindGroups.BindColumn(0, std::move(pGroupColumn));

    m_bindGroups.BindIsSelected(TriggerViewModel::GroupViewModel::IsSelectedProperty);
    m_bindGroups.BindItems(vmAssetList.Trigger().Groups());

    auto pIdColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
        TriggerConditionViewModel::IndexProperty);
    pIdColumn->SetHeader(L"ID");
    pIdColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_ID);
    m_bindTrigger.BindColumn(0, std::move(pIdColumn));

    auto pFlagColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TypeProperty, vmAssetList.Trigger().ConditionTypes());
    pFlagColumn->SetHeader(L"Flag");
    pFlagColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_FLAG);
    m_bindTrigger.BindColumn(1, std::move(pFlagColumn));

    auto pSourceTypeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::SourceTypeProperty, vmAssetList.Trigger().OperandTypes());
    pSourceTypeColumn->SetHeader(L"Type");
    pSourceTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    m_bindTrigger.BindColumn(2, std::move(pSourceTypeColumn));

    auto pSourceSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::SourceSizeProperty, vmAssetList.Trigger().OperandSizes());
    pSourceSizeColumn->SetHeader(L"Size");
    pSourceSizeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_SIZE);
    m_bindTrigger.BindColumn(3, std::move(pSourceSizeColumn));

    auto pSourceValueColumn = std::make_unique<ra::ui::win32::bindings::GridAddressColumnBinding>(
        TriggerConditionViewModel::SourceValueProperty);
    pSourceValueColumn->SetHeader(L"Memory");
    pSourceValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_VALUE);
    m_bindTrigger.BindColumn(4, std::move(pSourceValueColumn));

    auto pOperatorColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::OperatorProperty, vmAssetList.Trigger().OperatorTypes());
    pOperatorColumn->SetHeader(L"Oper");
    pOperatorColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_OPERATOR);
    m_bindTrigger.BindColumn(5, std::move(pOperatorColumn));

    auto pTargetTypeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TargetTypeProperty, vmAssetList.Trigger().OperandTypes());
    pTargetTypeColumn->SetHeader(L"Type");
    pTargetTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    m_bindTrigger.BindColumn(6, std::move(pTargetTypeColumn));

    auto pTargetSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TargetSizeProperty, vmAssetList.Trigger().OperandSizes());
    pTargetSizeColumn->SetHeader(L"Size");
    pTargetSizeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_SIZE);
    m_bindTrigger.BindColumn(7, std::move(pTargetSizeColumn));

    auto pTargetValueColumn = std::make_unique<ra::ui::win32::bindings::GridAddressColumnBinding>(
        TriggerConditionViewModel::TargetValueProperty);
    pTargetValueColumn->SetHeader(L"Mem/Val");
    pTargetValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_VALUE);
    m_bindTrigger.BindColumn(8, std::move(pTargetValueColumn));

    auto pHitsColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
        TriggerConditionViewModel::RequiredHitsProperty);
    pHitsColumn->SetHeader(L"Hits");
    pHitsColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, COLUMN_WIDTH_HITS);
    m_bindTrigger.BindColumn(9, std::move(pHitsColumn));

    m_bindTrigger.BindItems(vmAssetList.Trigger().Conditions());


    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LISTACHIEVEMENTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(400, 188);
}

BOOL AssetEditorDialog::OnInitDialog()
{
    m_bindID.SetControl(*this, IDC_RA_ACH_ID);
    m_bindName.SetControl(*this, IDC_RA_ACH_TITLE);
    m_bindDescription.SetControl(*this, IDC_RA_ACH_DESC);
    m_bindBadge.SetControl(*this, IDC_RA_BADGENAME);
    m_bindPoints.SetControl(*this, IDC_RA_ACH_POINTS);

    m_bindGroups.SetControl(*this, IDC_RA_ACH_GROUP);
    m_bindTrigger.SetControl(*this, IDC_RA_LBX_CONDITIONS);

    return DialogBase::OnInitDialog();
}

BOOL AssetEditorDialog::OnCommand(WORD nCommand)
{
    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
