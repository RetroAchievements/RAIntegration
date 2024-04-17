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
static constexpr int COLUMN_WIDTH_SIZE = 72;
static constexpr int COLUMN_WIDTH_VALUE = 72;
static constexpr int COLUMN_WIDTH_OPERATOR = 35;
static constexpr int COLUMN_WIDTH_HITS = 84;

using fnGetStockIconInfo = std::add_pointer_t<HRESULT WINAPI(SHSTOCKICONID, UINT, SHSTOCKICONINFO*)>;
static fnGetStockIconInfo pGetStockIconInfo = nullptr;

AssetEditorDialog::Presenter::Presenter() noexcept
{
    // SHGetStockIconInfo isn't supported on WinXP, so we have to dynamically find it.
    auto hDll = LoadLibraryA("Shell32");
    if (hDll)
        GSL_SUPPRESS_TYPE1 pGetStockIconInfo = reinterpret_cast<fnGetStockIconInfo>(GetProcAddress(hDll, "SHGetStockIconInfo"));
}

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

class ValueColumnBinding : public ra::ui::win32::bindings::GridTextColumnBinding
{
public:
    ValueColumnBinding(const StringModelProperty& pBoundProperty, const IntModelProperty& pTypeProperty) noexcept
        : ra::ui::win32::bindings::GridTextColumnBinding(pBoundProperty), m_pTypeProperty(&pTypeProperty)
    {
    }

    void SetMaximum(unsigned int nValue) noexcept { m_nMaximum = nValue; }

    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override
    {
        const auto nOperandType = ra::itoe<ra::ui::viewmodels::TriggerOperandType>(vmItems.GetItemValue(nIndex, *m_pTypeProperty));
        switch (nOperandType)
        {
            case ra::ui::viewmodels::TriggerOperandType::Float:
            {
                std::wstring sError;
                float fValue = 0.0;

                if (!ParseFloat(sValue, fValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }

                vmItems.SetItemValue(nIndex, *m_pBoundProperty,
                    ra::ui::viewmodels::TriggerConditionViewModel::FormatValue(fValue, nOperandType));
                return true;
            }

            case ra::ui::viewmodels::TriggerOperandType::Value:
            {
                const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
                {
                    std::wstring sError;
                    unsigned int nValue = 0U;

                    if (!ParseUnsignedInt(sValue, m_nMaximum, nValue, sError))
                    {
                        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                        return false;
                    }

                    vmItems.SetItemValue(nIndex, *m_pBoundProperty,
                        ra::ui::viewmodels::TriggerConditionViewModel::FormatValue(nValue, nOperandType));
                    return true;
                }
            }
            _FALLTHROUGH;

            default:
            {
                std::wstring sError;
                unsigned int nValue = 0U;

                if (!ParseHex(sValue, m_nMaximum, nValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }

                vmItems.SetItemValue(nIndex, *m_pBoundProperty,
                    ra::ui::viewmodels::TriggerConditionViewModel::FormatValue(nValue, nOperandType));

                return true;
            }
        }
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const override
    {
        if (pProperty == *m_pTypeProperty)
            return true;

        return GridTextColumnBinding::DependsOn(pProperty);
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
            unsigned int nAddress;
            std::wstring sError;
            auto sAddress = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
            if (!ra::ParseHex(sAddress, 0xFFFFFFFF, nAddress, sError))
                return false;

            if (vmItems.GetItemValue(nIndex, TriggerConditionViewModel::IsIndirectProperty))
            {
                const auto* pConditionViewModel = dynamic_cast<const TriggerConditionViewModel*>(vmItems.GetViewModelAt(nIndex));
                Expects(pConditionViewModel != nullptr);
                nAddress = pConditionViewModel->GetIndirectAddress(nAddress, nullptr);
            }

            auto& pMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
            pMemoryInspector.SetCurrentAddress(nAddress);

            if (!pMemoryInspector.IsVisible())
                pMemoryInspector.Show();

            return true;
        }

        return GridTextColumnBinding::HandleRightClick(vmItems, nIndex);
    }

protected:
    bool IsAddressType(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const
    {
        const auto nOperandType = ra::itoe<ra::ui::viewmodels::TriggerOperandType>(vmItems.GetItemValue(nIndex, *m_pTypeProperty));
        return (nOperandType != ra::ui::viewmodels::TriggerOperandType::Value &&
            nOperandType != ra::ui::viewmodels::TriggerOperandType::Float);
    }

    const IntModelProperty* m_pTypeProperty = nullptr;
    unsigned int m_nMaximum = 0xFFFFFFFF;
};

class TargetValueColumnBinding : public ValueColumnBinding
{
public:
    TargetValueColumnBinding(const StringModelProperty& pBoundProperty, const IntModelProperty& pTypeProperty) noexcept
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

        const auto nTotalHits = vmItems.GetItemValue(nIndex, TriggerConditionViewModel::TotalHitsProperty);
        if (nTotalHits > 0)
            return ra::StringPrintf(L"%d (%d) [%d]", nRequiredHits, nCurrentHits, nTotalHits);

        return ra::StringPrintf(L"%d (%d)", nRequiredHits, nCurrentHits);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == TriggerConditionViewModel::CurrentHitsProperty)
            return true;
        if (pProperty == TriggerConditionViewModel::TotalHitsProperty)
            return true;

        return GridNumberColumnBinding::DependsOn(pProperty);
    }

    bool DependsOn(const ra::ui::BoolModelProperty& pProperty) const override
    {
        if (pProperty == TriggerConditionViewModel::HasHitsProperty)
            return true;

        return GridNumberColumnBinding::DependsOn(pProperty);
    }

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto* vmConditions = dynamic_cast<const ViewModelCollection<TriggerConditionViewModel>*>(&vmItems);
        Expects(vmConditions != nullptr);

        std::wstring sTooltip;
        if (!TriggerViewModel::BuildHitChainTooltip(sTooltip, *vmConditions, nIndex))
            sTooltip.clear();

        return sTooltip;
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        auto* pGridBinding = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding);
        Expects(pGridBinding != nullptr);
        if (!pGridBinding->GetItems().GetItemValue(pInfo.nItemIndex, TriggerConditionViewModel::HasHitsProperty))
            return nullptr;
        if (!pGridBinding->GetItems().GetItemValue(pInfo.nItemIndex, TriggerConditionViewModel::CanEditHitsProperty))
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
        // ignore virtual tag that's hiding a more complex path
        if (sValue == L"[local]")
            return;

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
    InvokeOnUIThread([this, sTextCopy = sText]()
    {
        if (ra::StringStartsWith(sTextCopy, L"local\\"))
        {
            SetWindowTextW(m_hWnd, L"[local]");
            ::EnableWindow(m_hWndSpinner, false);
        }
        else
        {
            SetWindowTextW(m_hWnd, sTextCopy.c_str());

            if (m_nMinimum == 0)
            {
                ::EnableWindow(m_hWndSpinner, false);

                ra::api::FetchBadgeIds::Request request;
                request.CallAsync([this](const ra::api::FetchBadgeIds::Response& response) noexcept {
                    SetRange(ra::to_signed(response.FirstID), ra::to_signed(response.NextID) - 1);
                    ::EnableWindow(m_hWndSpinner, true);
                });
            }
            else
            {
                ::EnableWindow(m_hWndSpinner, true);
            }
        }
    });
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

        InvokeOnUIThread([this, bIsActive]() {
            const bool bWasActive = Button_GetCheck(m_hWnd);
            if (bIsActive != bWasActive)
            {
                Button_SetCheck(m_hWnd, bIsActive);
                OnValueChanged();
            }
        });
    }
}

void AssetEditorDialog::ConditionsGridBinding::OnViewModelAdded(gsl::index nIndex)
{
    CheckIdWidth();
    GridBinding::OnViewModelAdded(nIndex);
}

void AssetEditorDialog::ConditionsGridBinding::OnViewModelRemoved(gsl::index nIndex)
{
    CheckIdWidth();
    GridBinding::OnViewModelRemoved(nIndex);
}

void AssetEditorDialog::ConditionsGridBinding::OnEndViewModelCollectionUpdate()
{
    CheckIdWidth();
    GridBinding::OnEndViewModelCollectionUpdate();
}

void AssetEditorDialog::ConditionsGridBinding::CheckIdWidth()
{
    const bool bWideId = m_vmItems->Count() >= 1000;
    if (bWideId != m_bWideId)
    {
        m_vColumns.at(0)->SetWidth(GridColumnBinding::WidthType::Pixels, COLUMN_WIDTH_ID + (bWideId ? 5 : 0));
        m_bWideId = bWideId;

        UpdateLayout();
    }
}

AssetEditorDialog::ErrorIconBinding::~ErrorIconBinding()
{
    if (m_hErrorIcon)
    {
        ::DestroyIcon(m_hErrorIcon);
        m_hErrorIcon = nullptr;
    }

    if (m_hWarningIcon)
    {
        ::DestroyIcon(m_hWarningIcon);
        m_hWarningIcon = nullptr;
    }
}

void AssetEditorDialog::ErrorIconBinding::SetHWND(DialogBase& pDialog, HWND hControl)
{
    ControlBinding::SetHWND(pDialog, hControl);
    UpdateImage();
}

void AssetEditorDialog::ErrorIconBinding::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == AssetEditorViewModel::HasAssetValidationErrorProperty)
        UpdateImage();
    else if (args.Property == AssetEditorViewModel::HasAssetValidationWarningProperty)
        UpdateImage();
}

static HICON GetIcon(SHSTOCKICONID nStockIconId, LPCWSTR nOicIconId) noexcept
{
    if (pGetStockIconInfo != nullptr)
    {
        SHSTOCKICONINFO sii{};
        sii.cbSize = sizeof(sii);
        if (SUCCEEDED(pGetStockIconInfo(nStockIconId, SHGSI_ICON | SHGSI_SMALLICON, &sii)))
            return sii.hIcon;

        return nullptr;
    }

    // despite requesting a 16x16 icon, this returns a 32x32 one, which looks awkward in the space
    // provided. GetStockIconInfo is prefered because it returns an appropriately sized icon. this
    // is fallback logic for WinXP.
    GSL_SUPPRESS_TYPE1
    return reinterpret_cast<HICON>(LoadImage(nullptr, nOicIconId, IMAGE_ICON, 16, 16, LR_SHARED));
}

void AssetEditorDialog::ErrorIconBinding::SetErrorIcon() noexcept
{
    if (!m_hErrorIcon)
        m_hErrorIcon = GetIcon(SIID_ERROR, MAKEINTRESOURCE(OIC_ERROR));

    if (m_hErrorIcon)
        GSL_SUPPRESS_TYPE1::SendMessage(m_hWnd, STM_SETICON, reinterpret_cast<WPARAM>(m_hErrorIcon), NULL);
}

void AssetEditorDialog::ErrorIconBinding::SetWarningIcon() noexcept
{
    if (!m_hWarningIcon)
        m_hWarningIcon = GetIcon(SIID_WARNING, MAKEINTRESOURCE(OIC_WARNING));

    if (m_hWarningIcon)
        GSL_SUPPRESS_TYPE1::SendMessage(m_hWnd, STM_SETICON, reinterpret_cast<WPARAM>(m_hWarningIcon), NULL);
}

void AssetEditorDialog::ErrorIconBinding::UpdateImage()
{
    if (!m_hWnd)
        return;

    if (GetValue(AssetEditorViewModel::HasAssetValidationErrorProperty))
    {
        SetErrorIcon();
        ShowWindow(m_hWnd, SW_SHOW);
    }
    else if (GetValue(AssetEditorViewModel::HasAssetValidationWarningProperty))
    {
        SetWarningIcon();
        ShowWindow(m_hWnd, SW_SHOW);
    }
    else
    {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

AssetEditorDialog::PointBinding::PointBinding(ViewModelBase& vmViewModel) noexcept
    : ra::ui::win32::bindings::ComboBoxBinding(vmViewModel)
{
    static ra::ui::viewmodels::LookupItemViewModelCollection s_vDummy;
    m_pViewModelCollection = &s_vDummy;
}

void AssetEditorDialog::PointBinding::PopulateComboBox()
{
    for (const int nPoints : AchievementModel::ValidPoints())
    {
        ComboBox_AddString(m_hWnd, std::to_wstring(nPoints).c_str());
    }
}

void AssetEditorDialog::PointBinding::UpdateSelectedItem()
{
    if (!m_hWnd)
        return;

    const auto nPoints = GetValue(*m_pSelectedIdProperty);

    if (m_nCustomPointsIndex != -1)
    {
        if (nPoints == m_nCustomPoints)
            return;

        ComboBox_DeleteString(m_hWnd, m_nCustomPointsIndex);
        m_nCustomPointsIndex = -1;
        m_nCustomPoints = -1;
    }

    for (size_t nIndex = 0; nIndex < AchievementModel::ValidPoints().size(); ++nIndex)
    {
        if (AchievementModel::ValidPoints().at(nIndex) == nPoints)
        {
            ComboBox_SetCurSel(m_hWnd, nIndex);
            return;
        }
    }

    for (size_t nIndex = AchievementModel::ValidPoints().size(); nIndex > 0; --nIndex)
    {
        if (AchievementModel::ValidPoints().at(nIndex - 1) < nPoints)
        {
            m_nCustomPoints = nPoints;
            ComboBox_InsertString(m_hWnd, nIndex, std::to_wstring(nPoints).c_str());

            m_nCustomPointsIndex = gsl::narrow_cast<int>(nIndex);
            ComboBox_SetCurSel(m_hWnd, nIndex);
            break;
        }
    }
}

void AssetEditorDialog::PointBinding::OnValueChanged()
{
    const auto nIndex = ComboBox_GetCurSel(m_hWnd);
    if (m_nCustomPointsIndex == -1 || nIndex < m_nCustomPointsIndex)
    {
        const auto nPoints = AchievementModel::ValidPoints().at(nIndex);
        SetValue(*m_pSelectedIdProperty, nPoints);
    }
    else if (nIndex > m_nCustomPointsIndex)
    {
        const auto nPoints = AchievementModel::ValidPoints().at(gsl::narrow_cast<size_t>(nIndex) - 1);
        SetValue(*m_pSelectedIdProperty, nPoints);
    }
    else
    {
        SetValue(*m_pSelectedIdProperty, m_nCustomPoints);
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
    m_bindFormats(vmAssetEditor),
    m_bindAchievementTypes(vmAssetEditor),
    m_bindLeaderboardParts(vmAssetEditor),
    m_bindLowerIsBetter(vmAssetEditor),
    m_bindErrorIcon(vmAssetEditor),
    m_bindMeasuredAsPercent(vmAssetEditor.Trigger()),
    m_bindDebugHighlights(vmAssetEditor),
    m_bindPauseOnReset(vmAssetEditor),
    m_bindPauseOnTrigger(vmAssetEditor),
    m_bindActive(vmAssetEditor),
    m_bindDecimalPreferred(vmAssetEditor),
    m_bindGroups(vmAssetEditor.Trigger()),
    m_bindConditions(vmAssetEditor.Trigger())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::After, "Achievement Editor");
    m_bindWindow.AddChildViewModel(vmAssetEditor.Trigger());

    m_bindID.BindValue(AssetEditorViewModel::IDProperty);
    m_bindName.BindText(AssetEditorViewModel::NameProperty);
    m_bindDescription.BindText(AssetEditorViewModel::DescriptionProperty);

    m_bindWindow.BindEnabled(IDC_RA_TITLE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DESCRIPTION, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_ACTIVE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_POINTS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_BADGENAME, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_BADGE_SPIN, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_UPLOAD_BADGE, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_HIGHLIGHTS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_PAUSE_ON_RESET, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_PAUSE_ON_TRIGGER, AssetEditorViewModel::IsTriggerProperty);
    m_bindWindow.BindEnabled(IDC_RA_LBX_GROUPS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_GROUP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DELETE_GROUP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_COPY_ALL, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_LBX_CONDITIONS, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_DELETE_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_COPY_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_PASTE_COND, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_COND_UP, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_COND_DOWN, AssetEditorViewModel::IsAssetLoadedProperty);
    m_bindWindow.BindLabel(IDC_RA_CHK_ACTIVE, AssetEditorViewModel::WaitingLabelProperty);
    m_bindWindow.BindLabel(IDC_RA_MEASURED, AssetEditorViewModel::MeasuredValueProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_MEASURED, AssetEditorViewModel::HasMeasuredProperty);
    m_bindWindow.BindVisible(IDC_RA_MEASURED, AssetEditorViewModel::HasMeasuredProperty);

    // achievement only fields
    m_bindBadge.BindText(AssetEditorViewModel::BadgeProperty);
    m_bindBadge.SetWrapAround(true);
    m_bindBadgeImage.BindImage(AssetEditorViewModel::BadgeProperty, ra::ui::ImageType::Badge);
    m_bindPoints.BindSelectedItem(AssetEditorViewModel::PointsProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_POINTS, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_POINTS, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_CHK_AS_PERCENT, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_CHK_AS_PERCENT, AssetEditorViewModel::HasMeasuredProperty);
    m_bindMeasuredAsPercent.BindCheck(TriggerViewModel::IsMeasuredTrackedAsPercentProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_BADGENAME, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_BADGENAME, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_BADGE_SPIN, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_UPLOAD_BADGE, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_BADGEPIC, AssetEditorViewModel::IsAchievementProperty);
    m_bindAchievementTypes.BindItems(vmAssetEditor.AchievementTypes());
    m_bindAchievementTypes.BindSelectedItem(AssetEditorViewModel::AchievementTypeProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_TYPE, AssetEditorViewModel::IsAchievementProperty);
    m_bindWindow.BindVisible(IDC_RA_TYPE, AssetEditorViewModel::IsAchievementProperty);

    // leaderboard only fields
    m_bindFormats.BindItems(vmAssetEditor.Formats());
    m_bindFormats.BindSelectedItem(AssetEditorViewModel::ValueFormatProperty);
    m_bindLowerIsBetter.BindCheck(AssetEditorViewModel::LowerIsBetterProperty);
    m_bindWindow.BindLabel(IDC_RA_DISPLAY, AssetEditorViewModel::FormattedValueProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_FORMAT, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindVisible(IDC_RA_FORMAT, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindVisible(IDC_RA_CHK_LOWER_IS_BETTER, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindVisible(IDC_RA_LBL_DISPLAY, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindVisible(IDC_RA_DISPLAY, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindVisible(IDC_RA_LBX_LBOARD_PARTS, AssetEditorViewModel::IsLeaderboardProperty);
    m_bindWindow.BindLabel(IDC_RA_LBL_GROUPS, AssetEditorViewModel::GroupsHeaderProperty);

    auto pPartColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    pPartColumn->SetHeader(L"Part");
    pPartColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, 50);
    m_bindLeaderboardParts.BindColumn(0, std::move(pPartColumn));
    m_bindLeaderboardParts.BindIsSelected(ra::ui::viewmodels::LookupItemViewModel::IsSelectedProperty);
    m_bindLeaderboardParts.BindItems(vmAssetEditor.LeaderboardParts());
    m_bindLeaderboardParts.BindRowColor(AssetEditorViewModel::LeaderboardPartViewModel::ColorProperty);

    m_bindDebugHighlights.BindCheck(AssetEditorViewModel::DebugHighlightsEnabledProperty);
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
    m_bindGroups.BindRowColor(TriggerViewModel::GroupViewModel::ColorProperty);

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
    m_bindConditions.BindRowColor(TriggerConditionViewModel::RowColorProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_TITLE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ID, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_ID, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CHK_ACTIVE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DESCRIPTION, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBL_TYPE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_TYPE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBL_POINTS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_POINTS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LBL_FORMAT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_FORMAT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LBL_DISPLAY, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_DISPLAY, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CHK_LOWER_IS_BETTER, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LBL_MEASURED, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_MEASURED, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CHK_AS_PERCENT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LBL_BADGENAME, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGENAME, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGE_SPIN, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_UPLOAD_BADGE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BADGEPIC, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_LBOARD_PARTS, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_GROUPS, Anchor::Top | Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADD_GROUP, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_DELETE_GROUP, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_COPY_ALL, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_ERROR_INDICATOR, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CHK_HIGHLIGHTS, Anchor::Top | Anchor::Right);
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
    m_bindFormats.SetControl(*this, IDC_RA_FORMAT);
    m_bindLeaderboardParts.SetControl(*this, IDC_RA_LBX_LBOARD_PARTS);
    m_bindAchievementTypes.SetControl(*this, IDC_RA_TYPE);

    m_bindGroups.SetControl(*this, IDC_RA_LBX_GROUPS);
    m_bindConditions.SetControl(*this, IDC_RA_LBX_CONDITIONS);
    m_bindConditions.InitializeTooltips(std::chrono::seconds(30));

    m_bindErrorIcon.SetControl(*this, IDC_RA_ERROR_INDICATOR);
    m_bindMeasuredAsPercent.SetControl(*this, IDC_RA_CHK_AS_PERCENT);
    m_bindLowerIsBetter.SetControl(*this, IDC_RA_CHK_LOWER_IS_BETTER);
    m_bindDebugHighlights.SetControl(*this, IDC_RA_CHK_HIGHLIGHTS);
    m_bindPauseOnReset.SetControl(*this, IDC_RA_CHK_PAUSE_ON_RESET);
    m_bindPauseOnTrigger.SetControl(*this, IDC_RA_CHK_PAUSE_ON_TRIGGER);
    m_bindActive.SetControl(*this, IDC_RA_CHK_ACTIVE);
    m_bindDecimalPreferred.SetControl(*this, IDC_RA_CHK_SHOW_DECIMALS);

    m_hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

    if (m_hTooltip)
    {
        const auto hErrorIconControl = GetDlgItem(GetHWND(), IDC_RA_ERROR_INDICATOR);

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

    SendMessage(::GetDlgItem(GetHWND(), IDC_RA_TYPE), CB_SETDROPPEDWIDTH, 70, 0);
    SendMessage(::GetDlgItem(GetHWND(), IDC_RA_FORMAT), CB_SETDROPPEDWIDTH, 136, 0);

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
                            const auto& sError = (vmAssetEditor->HasAssetValidationError()) ?
                                vmAssetEditor->GetAssetValidationError() : vmAssetEditor->GetAssetValidationWarning();
                            GSL_SUPPRESS_TYPE3 pnmTtDispInfo->lpszText = const_cast<LPTSTR>(sError.c_str());
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
