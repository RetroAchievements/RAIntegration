#include "AssetEditorDialog.hh"

#include "RA_Resource.h"

#include "api\FetchBadgeIds.hh"

#include "data\context\EmulatorContext.hh"

#include "services\IConfiguration.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::data::models::AssetModelBase;
using ra::data::models::AchievementModel;
using ra::ui::viewmodels::AssetEditorViewModel;
using ra::ui::viewmodels::TriggerViewModel;
using ra::ui::viewmodels::TriggerConditionViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

static constexpr int COLUMN_WIDTH_ID = 30;
static constexpr int COLUMN_WIDTH_FLAG = 78;
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

class HideableGridLookupColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    HideableGridLookupColumnBinding(const IntModelProperty& pBoundProperty, const BoolModelProperty& pIsHiddenProperty,
        const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept
        : ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems), m_pIsHiddenProperty(pIsHiddenProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return GridLookupColumnBinding::GetText(vmItems, nIndex);
    }

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return GridLookupColumnBinding::GetTooltip(vmItems, nIndex);
    }

    bool DependsOn(const ra::ui::BoolModelProperty& pProperty) const override
    {
        if (pProperty == m_pIsHiddenProperty)
            return true;

        return GridColumnBinding::DependsOn(pProperty);
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        auto* pGridBinding = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding);
        Expects(pGridBinding != nullptr);

        if (IsHidden(pGridBinding->GetItems(), pInfo.nItemIndex))
            return nullptr;

        return GridLookupColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

protected:
    bool IsHidden(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        return !vmItems.GetItemValue(nIndex, m_pIsHiddenProperty);
    }

    const BoolModelProperty& m_pIsHiddenProperty;
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

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto* vmConditions = dynamic_cast<const ViewModelCollection<TriggerConditionViewModel>*>(&vmItems);
        Expects(vmConditions != nullptr);
        const auto* vmCondition = vmConditions->GetItemAt(nIndex);
        if (vmCondition != nullptr)
            return vmCondition->GetTooltip(*m_pBoundProperty);

        return L"";
    }

    bool HandleRightClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) override
    {
        if (IsAddressType(vmItems, nIndex))
        {
            auto nAddress = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
            if (vmItems.GetItemValue(nIndex, TriggerConditionViewModel::IsIndirectProperty))
            {
                const auto* pConditionViewModel = dynamic_cast<const TriggerConditionViewModel*>(vmItems.GetViewModelAt(nIndex));
                Expects(pConditionViewModel != nullptr);
                nAddress = pConditionViewModel->GetIndirectAddress(nAddress);
            }

            auto& pMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
            pMemoryInspector.SetCurrentAddress(nAddress);

            if (!pMemoryInspector.IsVisible())
                pMemoryInspector.Show();

            return true;
        }

        return GridNumberColumnBinding::HandleRightClick(vmItems, nIndex);
    }

protected:
    bool IsAddressType(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        const auto nOperandType = ra::itoe<ra::ui::viewmodels::TriggerOperandType>(vmItems.GetItemValue(nIndex, *m_pTypeProperty));
        return (nOperandType != ra::ui::viewmodels::TriggerOperandType::Value);
    }

    const IntModelProperty* m_pTypeProperty = nullptr;
};

class TargetValueColumnBinding : public ValueColumnBinding
{
public:
    TargetValueColumnBinding(const IntModelProperty& pBoundProperty, const IntModelProperty& pTypeProperty) noexcept
        : ValueColumnBinding(pBoundProperty, pTypeProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return ValueColumnBinding::GetText(vmItems, nIndex);
    }

    bool DependsOn(const ra::ui::BoolModelProperty& pProperty) const override
    {
        if (pProperty == TriggerConditionViewModel::HasTargetProperty)
            return true;

        return GridColumnBinding::DependsOn(pProperty);
    }

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return ValueColumnBinding::GetTooltip(vmItems, nIndex);
    }

    bool HandleRightClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) override
    {
        if (IsHidden(vmItems, nIndex))
            return false;

        return ValueColumnBinding::HandleRightClick(vmItems, nIndex);
    }

protected:
    bool IsHidden(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        return !vmItems.GetItemValue(nIndex, TriggerConditionViewModel::HasTargetProperty);
    }
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
        if (!vmItems.GetItemValue(nIndex, TriggerConditionViewModel::HasHitsProperty))
            return L"";

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
        if (pProperty == TriggerConditionViewModel::HasHitsProperty)
            return true;

        return GridNumberColumnBinding::DependsOn(pProperty);
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        auto* pGridBinding = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding);
        Expects(pGridBinding != nullptr);
        if (!pGridBinding->GetItems().GetItemValue(pInfo.nItemIndex, TriggerConditionViewModel::HasHitsProperty))
            return nullptr;

        m_nEditingItemIndex = pInfo.nItemIndex;
        return GridNumberColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

private:
    mutable gsl::index m_nEditingItemIndex = -1;
};

void AssetEditorDialog::BadgeNameBinding::UpdateSourceFromText(const std::wstring& sValue)
{
    std::wstring sError;
    if (!sValue.empty())
    {
        // special case - don't validate if the string is entirely made of 0s.
        // the default value is "00000", which is less than the minimum.
        const wchar_t* pChar = &sValue.at(0);
        Expects(pChar != nullptr);
        while (*pChar && *pChar == L'0')
            ++pChar;

        if (*pChar)
            ParseValue(sValue, sError);
    }

    if (!sError.empty())
    {
        ::EnableWindow(m_hWndSpinner, false);
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(sError);
    }
    else
    {
        ::EnableWindow(m_hWndSpinner, true);

        if (sValue.length() < 5)
        {
            std::wstring sBadge(sValue);
            sBadge.insert(0, 5 - sValue.length(), '0');
            SetWindowTextW(m_hWnd, sBadge.c_str());
            TextBoxBinding::UpdateSourceFromText(sBadge);
        }
        else
        {
            TextBoxBinding::UpdateSourceFromText(sValue);
        }
    }
}

void AssetEditorDialog::BadgeNameBinding::UpdateTextFromSource(const std::wstring& sText)
{
    if (ra::StringStartsWith(sText, L"local\\"))
    {
        SetWindowTextW(m_hWnd, L"[local]");
        ::EnableWindow(m_hWndSpinner, false);
    }
    else
    {
        SetWindowTextW(m_hWnd, sText.c_str());

        if (m_nMinimum == 0)
        {
            ::EnableWindow(m_hWndSpinner, false);

            ra::api::FetchBadgeIds::Request request;
            request.CallAsync([this](const ra::api::FetchBadgeIds::Response& response) noexcept
            {
                SetRange(ra::to_signed(response.FirstID), ra::to_signed(response.NextID) - 1);
                ::EnableWindow(m_hWndSpinner, true);
            });
        }
        else
        {
            ::EnableWindow(m_hWndSpinner, true);
        }
    }
}

void AssetEditorDialog::DecimalPreferredBinding::OnValueChanged()
{
    m_pOwner->m_bindConditions.RefreshColumn(4); // source value
    m_pOwner->m_bindConditions.RefreshColumn(8); // target value
}

bool AssetEditorDialog::ActiveCheckBoxBinding::IsActive() const
{
    const auto& vmAssetEditor = GetViewModel<ra::ui::viewmodels::AssetEditorViewModel&>();
    return ra::data::models::AssetModelBase::IsActive(vmAssetEditor.GetState());
}

void AssetEditorDialog::ActiveCheckBoxBinding::SetHWND(DialogBase& pDialog, HWND hControl)
{
    ControlBinding::SetHWND(pDialog, hControl);

    Button_SetCheck(m_hWnd, IsActive() ? BST_CHECKED : BST_UNCHECKED);
}

void AssetEditorDialog::ActiveCheckBoxBinding::OnCommand()
{
    // check state has already been updated - update the backing data to match
    const auto bIsChecked = Button_GetCheck(m_hWnd);
    if (bIsChecked)
    {
        if (GetValue(ra::ui::viewmodels::AssetEditorViewModel::HasAssetValidationErrorProperty))
        {
            Button_SetCheck(m_hWnd, BST_UNCHECKED);

            const auto sError = ra::StringPrintf(L"The following errors must be corrected:\n* %s",
                GetValue(ra::ui::viewmodels::AssetEditorViewModel::AssetValidationErrorProperty));
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unable to activate", sError);

            return;
        }

        SetValue(ra::ui::viewmodels::AssetEditorViewModel::StateProperty, ra::etoi(ra::data::models::AssetState::Waiting));
    }
    else
    {
        SetValue(ra::ui::viewmodels::AssetEditorViewModel::StateProperty, ra::etoi(ra::data::models::AssetState::Inactive));
    }

    OnValueChanged();
}

void AssetEditorDialog::ActiveCheckBoxBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::ui::viewmodels::AssetEditorViewModel::StateProperty)
    {
        const bool bIsActive = IsActive();
        const bool bWasActive = Button_GetCheck(m_hWnd);
        if (bIsActive != bWasActive)
        {
            Button_SetCheck(m_hWnd, bIsActive);
            OnValueChanged();
        }
    }
}

AssetEditorDialog::AssetEditorDialog(AssetEditorViewModel& vmAssetEditor)
    : DialogBase(vmAssetEditor),
    m_bindID(vmAssetEditor),
    m_bindName(vmAssetEditor),
    m_bindDescription(vmAssetEditor),
    m_bindBadge(vmAssetEditor),
    m_bindBadgeImage(vmAssetEditor),
    m_bindPoints(vmAssetEditor),
    m_bindPauseOnReset(vmAssetEditor),
    m_bindPauseOnTrigger(vmAssetEditor),
    m_bindActive(vmAssetEditor),
    m_bindDecimalPreferred(this, vmAssetEditor),
    m_bindGroups(vmAssetEditor.Trigger()),
    m_bindConditions(vmAssetEditor.Trigger())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::After, "Achievement Editor");
    m_bindWindow.AddChildViewModel(vmAssetEditor.Trigger());

    m_bindID.BindValue(AssetEditorViewModel::IDProperty);
    m_bindName.BindText(AssetEditorViewModel::NameProperty);
    m_bindDescription.BindText(AssetEditorViewModel::DescriptionProperty);
    m_bindBadge.BindText(AssetEditorViewModel::BadgeProperty);
    m_bindBadge.SetWrapAround(true);
    m_bindBadgeImage.BindImage(AssetEditorViewModel::BadgeProperty, ra::ui::ImageType::Badge);
    m_bindPoints.BindValue(AssetEditorViewModel::PointsProperty);
    m_bindWindow.BindEnabled(IDC_RA_TITLE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DESCRIPTION, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_ACTIVE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_POINTS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_BADGENAME, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_BADGE_SPIN, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_UPLOAD_BADGE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_PAUSE_ON_RESET, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_PAUSE_ON_TRIGGER, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_LBX_GROUPS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_GROUP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DELETE_GROUP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_LBX_CONDITIONS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DELETE_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_COPY_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_PASTE_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_COND_UP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_COND_DOWN, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindLabel(IDC_RA_CHK_ACTIVE, AssetEditorViewModel::WaitingLabelProperty);
    m_bindWindow.BindVisible(IDC_RA_ERROR_INDICATOR, AssetEditorViewModel::HasAssetValidationErrorProperty);
    m_bindWindow.BindLabel(IDC_RA_MEASURED, AssetEditorViewModel::MeasuredValueProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_MEASURED, AssetEditorViewModel::HasMeasuredProperty);
    m_bindWindow.BindVisible(IDC_RA_MEASURED, AssetEditorViewModel::HasMeasuredProperty);

    m_bindPauseOnReset.BindCheck(AssetEditorViewModel::PauseOnResetProperty);
    m_bindPauseOnTrigger.BindCheck(AssetEditorViewModel::PauseOnTriggerProperty);
    m_bindDecimalPreferred.BindCheck(AssetEditorViewModel::DecimalPreferredProperty);

    auto pGroupColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerViewModel::GroupViewModel::LabelProperty);
    pGroupColumn->SetHeader(L"Group");
    pGroupColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, 50);
    m_bindGroups.BindColumn(0, std::move(pGroupColumn));

    m_bindGroups.BindIsSelected(TriggerViewModel::GroupViewModel::IsSelectedProperty);
    m_bindGroups.BindItems(vmAssetEditor.Trigger().Groups());
    m_bindGroups.BindEnsureVisible(TriggerViewModel::EnsureVisibleGroupIndexProperty);

    auto pIdColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
        TriggerConditionViewModel::IndexProperty);
    pIdColumn->SetHeader(L"ID");
    pIdColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_ID);
    m_bindConditions.BindColumn(0, std::move(pIdColumn));

    auto pFlagColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::TypeProperty, vmAssetEditor.Trigger().ConditionTypes());
    pFlagColumn->SetHeader(L"Flag");
    pFlagColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_FLAG);
    pFlagColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(1, std::move(pFlagColumn));

    auto pSourceTypeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        TriggerConditionViewModel::SourceTypeProperty, vmAssetEditor.Trigger().OperandTypes());
    pSourceTypeColumn->SetHeader(L"Type");
    pSourceTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    pSourceTypeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(2, std::move(pSourceTypeColumn));

    auto pSourceSizeColumn = std::make_unique<HideableGridLookupColumnBinding>(TriggerConditionViewModel::SourceSizeProperty,
        TriggerConditionViewModel::HasSourceSizeProperty, vmAssetEditor.Trigger().OperandSizes());
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
        TriggerConditionViewModel::OperatorProperty, vmAssetEditor.Trigger().OperatorTypes());
    pOperatorColumn->SetHeader(L"Cmp");
    pOperatorColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_OPERATOR);
    pOperatorColumn->SetVisibilityFilter(TriggerConditionViewModel::IsComparisonVisible);
    pOperatorColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(5, std::move(pOperatorColumn));

    auto pTargetTypeColumn = std::make_unique<HideableGridLookupColumnBinding>(TriggerConditionViewModel::TargetTypeProperty,
        TriggerConditionViewModel::HasTargetProperty, vmAssetEditor.Trigger().OperandTypes());
    pTargetTypeColumn->SetHeader(L"Type");
    pTargetTypeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_TYPE);
    pTargetTypeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(6, std::move(pTargetTypeColumn));

    auto pTargetSizeColumn = std::make_unique<HideableGridLookupColumnBinding>(TriggerConditionViewModel::TargetSizeProperty,
        TriggerConditionViewModel::HasTargetSizeProperty, vmAssetEditor.Trigger().OperandSizes());
    pTargetSizeColumn->SetHeader(L"Size");
    pTargetSizeColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_SIZE);
    pTargetSizeColumn->SetReadOnly(false);
    m_bindConditions.BindColumn(7, std::move(pTargetSizeColumn));

    auto pTargetValueColumn = std::make_unique<TargetValueColumnBinding>(
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
    m_bindConditions.BindItems(vmAssetEditor.Trigger().Conditions());

    m_bindConditions.SetCopyHandler([&vmAssetEditor]() { vmAssetEditor.Trigger().CopySelectedConditionsToClipboard(); });
    m_bindConditions.SetPasteHandler([&vmAssetEditor]() { vmAssetEditor.Trigger().PasteFromClipboard(); });
    m_bindConditions.BindEnsureVisible(TriggerViewModel::EnsureVisibleConditionIndexProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_TITLE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ID, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_ID, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CHK_ACTIVE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DESCRIPTION, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBL_POINTS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_POINTS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LBL_BADGENAME, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGENAME, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGE_SPIN, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_UPLOAD_BADGE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGEPIC, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_GROUPS, Anchor::Top | Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADD_GROUP, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_DELETE_GROUP, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_CHK_PAUSE_ON_RESET, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CHK_PAUSE_ON_TRIGGER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_CONDITIONS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADD_COND, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_DELETE_COND, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_COPY_COND, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_PASTE_COND, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_MOVE_COND_UP, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_MOVE_COND_DOWN, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_CHK_SHOW_DECIMALS, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(665, 348);
}

BOOL AssetEditorDialog::OnInitDialog()
{
    m_bindID.SetControl(*this, IDC_RA_ID);
    m_bindName.SetControl(*this, IDC_RA_TITLE);
    m_bindDescription.SetControl(*this, IDC_RA_DESCRIPTION);
    m_bindBadge.SetControl(*this, IDC_RA_BADGENAME);
    m_bindBadge.SetSpinnerControl(*this, IDC_RA_BADGE_SPIN);
    m_bindBadgeImage.SetControl(*this, IDC_RA_BADGEPIC);
    m_bindPoints.SetControl(*this, IDC_RA_POINTS);

    m_bindGroups.SetControl(*this, IDC_RA_LBX_GROUPS);
    m_bindConditions.SetControl(*this, IDC_RA_LBX_CONDITIONS);
    m_bindConditions.InitializeTooltips(std::chrono::seconds(30));

    m_bindPauseOnReset.SetControl(*this, IDC_RA_CHK_PAUSE_ON_RESET);
    m_bindPauseOnTrigger.SetControl(*this, IDC_RA_CHK_PAUSE_ON_TRIGGER);
    m_bindActive.SetControl(*this, IDC_RA_CHK_ACTIVE);
    m_bindDecimalPreferred.SetControl(*this, IDC_RA_CHK_SHOW_DECIMALS);

    if (!m_hErrorIcon)
    {
        SHSTOCKICONINFO sii{};
        sii.cbSize = sizeof(sii);
        if (SUCCEEDED(::SHGetStockIconInfo(SIID_ERROR, SHGSI_ICON | SHGSI_SMALLICON, &sii)))
            m_hErrorIcon = sii.hIcon;
    }

    const auto hErrorIconControl = GetDlgItem(GetHWND(), IDC_RA_ERROR_INDICATOR);
    if (m_hErrorIcon)
        GSL_SUPPRESS_TYPE1 ::SendMessage(hErrorIconControl, STM_SETICON, reinterpret_cast<WPARAM>(m_hErrorIcon), NULL);

    m_hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

    if (m_hTooltip)
    {
        TOOLINFO toolInfo;
        memset(&toolInfo, 0, sizeof(toolInfo));
        GSL_SUPPRESS_ES47 toolInfo.cbSize = TTTOOLINFO_V1_SIZE;
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.hwnd = GetHWND();
        GSL_SUPPRESS_TYPE1 toolInfo.uId = reinterpret_cast<UINT_PTR>(hErrorIconControl);
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        ::GetClientRect(hErrorIconControl, &toolInfo.rect);
        GSL_SUPPRESS_TYPE1 SendMessage(m_hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
        SendMessage(m_hTooltip, TTM_ACTIVATE, TRUE, 0);
    }

    return DialogBase::OnInitDialog();
}

INT_PTR CALLBACK AssetEditorDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmHdr;
            GSL_SUPPRESS_TYPE1{ pnmHdr = reinterpret_cast<LPNMHDR>(lParam); }
            switch (pnmHdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    if (pnmHdr->hwndFrom == m_hTooltip)
                    {
                        const auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
                        if (vmAssetEditor)
                        {
                            NMTTDISPINFO* pnmTtDispInfo;
                            GSL_SUPPRESS_TYPE1{ pnmTtDispInfo = reinterpret_cast<NMTTDISPINFO*>(lParam); }
                            Expects(pnmTtDispInfo != nullptr);
                            GSL_SUPPRESS_TYPE3 pnmTtDispInfo->lpszText = const_cast<LPTSTR>(vmAssetEditor->GetAssetValidationError().c_str());
                            return 0;
                        }
                    }

                    break;
                }
            }

            break;
        }
    }

    return DialogBase::DialogProc(hDlg, uMsg, wParam, lParam);
}

void AssetEditorDialog::OnDestroy()
{
    if (m_hErrorIcon)
    {
        ::DestroyIcon(m_hErrorIcon);
        m_hErrorIcon = nullptr;
    }

    if (m_hTooltip)
    {
        ::DestroyWindow(m_hTooltip);
        m_hTooltip = nullptr;
    }

    return DialogBase::OnDestroy();
}

BOOL AssetEditorDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_ADD_COND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().NewCondition();

            return TRUE;
        }

        case IDC_RA_COPY_COND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().CopySelectedConditionsToClipboard();

            return TRUE;
        }

        case IDC_RA_PASTE_COND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().PasteFromClipboard();

            return TRUE;
        }

        case IDC_RA_DELETE_COND:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().RemoveSelectedConditions();

            return TRUE;
        }

        case IDC_RA_MOVE_COND_UP:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().MoveSelectedConditionsUp();

            return TRUE;
        }

        case IDC_RA_MOVE_COND_DOWN:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().MoveSelectedConditionsDown();

            return TRUE;
        }

        case IDC_RA_UPLOAD_BADGE:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->SelectBadgeFile();

            return TRUE;
        }

        case IDC_RA_ADD_GROUP:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().AddGroup();

            return TRUE;
        }

        case IDC_RA_DELETE_GROUP:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().RemoveGroup();

            return TRUE;
        }

        case IDC_RA_COPY_ALL:
        {
            auto* vmAssetEditor = dynamic_cast<AssetEditorViewModel*>(&m_vmWindow);
            if (vmAssetEditor)
                vmAssetEditor->Trigger().CopyToClipboard();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
