#include "AssetEditorDialog.hh"

#include "RA_Resource.h"

#include "data\EmulatorContext.hh"

#include "services\IConfiguration.hh"

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

class SizeColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    SizeColumnBinding(const IntModelProperty& pBoundProperty, const IntModelProperty& pTypeProperty, const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept
        : ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems), m_pTypeProperty(&pTypeProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsValueHidden(vmItems, nIndex))
            return L"";

        return GridLookupColumnBinding::GetText(vmItems, nIndex);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == *m_pTypeProperty)
            return true;

        return GridLookupColumnBinding::DependsOn(pProperty);
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        auto* pGridBinding = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding);
        Expects(pGridBinding != nullptr);

        if (IsValueHidden(pGridBinding->GetItems(), pInfo.nItemIndex))
            return nullptr;

        return GridLookupColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

protected:
    bool IsValueHidden(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        const auto nOperandType = ra::itoe<ra::ui::viewmodels::TriggerOperandType>(vmItems.GetItemValue(nIndex, *m_pTypeProperty));
        return (nOperandType == ra::ui::viewmodels::TriggerOperandType::Value);
    }

    const IntModelProperty* m_pTypeProperty = nullptr;
};

class ValueColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    ValueColumnBinding(const IntModelProperty& pBoundProperty, const IntModelProperty& pTypeProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty), m_pTypeProperty(&pTypeProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);

        if (IsAddressType(vmItems, nIndex))
            return ra::Widen(ra::ByteAddressToString(nValue));

        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
            return std::to_wstring(nValue);

        return ra::StringPrintf(L"0x%02x", nValue);
    }

    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override
    {
        if (ra::StringStartsWith(sValue, L"0x"))
        {
            std::wstring sError;
            unsigned int nValue = 0U;

            if (!ParseHex(sValue, nValue, sError))
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                return false;
            }

            vmItems.SetItemValue(nIndex, *m_pBoundProperty, ra::to_signed(nValue));
            return true;
        }

        return GridNumberColumnBinding::SetText(vmItems, nIndex, sValue);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == *m_pTypeProperty)
            return true;

        return GridNumberColumnBinding::DependsOn(pProperty);
    }

protected:
    bool IsAddressType(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        const auto nOperandType = ra::itoe<ra::ui::viewmodels::TriggerOperandType>(vmItems.GetItemValue(nIndex, *m_pTypeProperty));
        return (nOperandType != ra::ui::viewmodels::TriggerOperandType::Value);
    }

    const IntModelProperty* m_pTypeProperty = nullptr;
};

class HitsColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    HitsColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nCurrentHits = vmItems.GetItemValue(nIndex, TriggerConditionViewModel::CurrentHitsProperty);
        const auto nRequiredHits = vmItems.GetItemValue(nIndex, *m_pBoundProperty);

        if (m_nEditingItemIndex == nIndex)
        {
            m_nEditingItemIndex = -1;
            return std::to_wstring(nRequiredHits);
        }

        return ra::StringPrintf(L"%d (%d)", nRequiredHits, nCurrentHits);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == TriggerConditionViewModel::CurrentHitsProperty)
            return true;

        return GridNumberColumnBinding::DependsOn(pProperty);
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        m_nEditingItemIndex = pInfo.nItemIndex;
        return GridNumberColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

private:
    mutable gsl::index m_nEditingItemIndex = -1;
};

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
      m_bindConditions(vmAssetList.Trigger())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::After, "Achievement Editor");
    m_bindWindow.AddChildViewModel(vmAssetList.Trigger());

    m_bindID.BindValue(AssetEditorViewModel::IDProperty);
    m_bindName.BindText(AssetEditorViewModel::NameProperty);
    m_bindDescription.BindText(AssetEditorViewModel::DescriptionProperty);
    m_bindBadge.BindText(AssetEditorViewModel::BadgeProperty);
    m_bindPoints.BindValue(AssetEditorViewModel::PointsProperty);

    m_bindWindow.BindEnabled(IDC_RA_ACH_TITLE, AssetEditorViewModel::AssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_ACH_DESC, AssetEditorViewModel::AssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_ACH_PAUSE_ON_RESET, AssetEditorViewModel::AssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER, AssetEditorViewModel::AssetLoadedProperty);

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
    m_bindConditions.BindColumn(0, std::move(pIdColumn));

    auto pFlagColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TypeProperty, vmAssetList.Trigger().ConditionTypes());
    pFlagColumn->SetHeader(L"Flag");
    pFlagColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_FLAG);
    pFlagColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(1, std::move(pFlagColumn));

    auto pSourceTypeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::SourceTypeProperty, vmAssetList.Trigger().OperandTypes());
    pSourceTypeColumn->SetHeader(L"Type");
    pSourceTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    pSourceTypeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(2, std::move(pSourceTypeColumn));

    auto pSourceSizeColumn = std::make_unique<SizeColumnBinding>(TriggerConditionViewModel::SourceSizeProperty,
        TriggerConditionViewModel::SourceTypeProperty, vmAssetList.Trigger().OperandSizes());
    pSourceSizeColumn->SetHeader(L"Size");
    pSourceSizeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_SIZE);
    pSourceSizeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(3, std::move(pSourceSizeColumn));

    auto pSourceValueColumn = std::make_unique<ValueColumnBinding>(
        TriggerConditionViewModel::SourceValueProperty, TriggerConditionViewModel::SourceTypeProperty);
    pSourceValueColumn->SetHeader(L"Memory");
    pSourceValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_VALUE);
    pSourceValueColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(4, std::move(pSourceValueColumn));

    auto pOperatorColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::OperatorProperty, vmAssetList.Trigger().OperatorTypes());
    pOperatorColumn->SetHeader(L"Cmp");
    pOperatorColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_OPERATOR);
    pOperatorColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(5, std::move(pOperatorColumn));

    auto pTargetTypeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TargetTypeProperty, vmAssetList.Trigger().OperandTypes());
    pTargetTypeColumn->SetHeader(L"Type");
    pTargetTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    pTargetTypeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(6, std::move(pTargetTypeColumn));

    auto pTargetSizeColumn = std::make_unique<SizeColumnBinding>(TriggerConditionViewModel::TargetSizeProperty,
        TriggerConditionViewModel::TargetTypeProperty, vmAssetList.Trigger().OperandSizes());
    pTargetSizeColumn->SetHeader(L"Size");
    pTargetSizeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_SIZE);
    pTargetSizeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(7, std::move(pTargetSizeColumn));

    auto pTargetValueColumn = std::make_unique<ValueColumnBinding>(
        TriggerConditionViewModel::TargetValueProperty, TriggerConditionViewModel::TargetTypeProperty);
    pTargetValueColumn->SetHeader(L"Mem/Val");
    pTargetValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_VALUE);
    pTargetValueColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(8, std::move(pTargetValueColumn));

    auto pHitsColumn = std::make_unique<HitsColumnBinding>(TriggerConditionViewModel::RequiredHitsProperty);
    pHitsColumn->SetHeader(L"Hits");
    pHitsColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, COLUMN_WIDTH_HITS);
    pHitsColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(9, std::move(pHitsColumn));

    m_bindConditions.BindIsSelected(TriggerConditionViewModel::IsSelectedProperty);
    m_bindConditions.BindItems(vmAssetList.Trigger().Conditions());

    m_bindConditions.SetCopyHandler([&vmAssetList]() { vmAssetList.Trigger().CopySelectedConditionsToClipboard(); });
    m_bindConditions.SetPasteHandler([&vmAssetList]() { vmAssetList.Trigger().PasteFromClipboard(); });
    m_bindConditions.BindEnsureVisible(TriggerViewModel::EnsureVisibleConditionIndexProperty);

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
    m_bindConditions.SetControl(*this, IDC_RA_LBX_CONDITIONS);

    m_bindPauseOnReset.SetControl(*this, IDC_RA_CHK_ACH_PAUSE_ON_RESET);
    m_bindPauseOnTrigger.SetControl(*this, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER);

    return DialogBase::OnInitDialog();
}

BOOL AssetEditorDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_COPYCOND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().CopySelectedConditionsToClipboard();

            return TRUE;
        }

        case IDC_RA_PASTECOND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().PasteFromClipboard();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
