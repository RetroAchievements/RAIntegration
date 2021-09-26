#include "AssetEditorViewModel.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\LeaderboardModel.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\ImageReference.hh"
#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetEditorViewModel::IDProperty("AssetEditorViewModel", "ID", 0);
const StringModelProperty AssetEditorViewModel::NameProperty("AssetEditorViewModel", "Name", L"[No Achievement Loaded]");
const StringModelProperty AssetEditorViewModel::DescriptionProperty("AssetEditorViewModel", "Description", L"Open an achievement from the Achievements List");
const IntModelProperty AssetEditorViewModel::CategoryProperty("AssetEditorViewModel", "Category", ra::etoi(ra::data::models::AssetCategory::Core));
const IntModelProperty AssetEditorViewModel::StateProperty("AssetEditorViewModel", "State", ra::etoi(ra::data::models::AssetState::Inactive));
const BoolModelProperty AssetEditorViewModel::IsAchievementProperty("AssetEditorViewModel", "IsAchievement", true);
const IntModelProperty AssetEditorViewModel::PointsProperty("AssetEditorViewModel", "Points", 0);
const StringModelProperty AssetEditorViewModel::BadgeProperty("AssetEditorViewModel", "Badge", L"00000");
const BoolModelProperty AssetEditorViewModel::IsLeaderboardProperty("AssetEditorViewModel", "IsLeaderboard", false);
const IntModelProperty AssetEditorViewModel::ValueFormatProperty("AssetEditorViewModel", "ValueFormat", ra::etoi(ra::data::ValueFormat::Value));
const BoolModelProperty AssetEditorViewModel::LowerIsBetterProperty("AssetEditorViewModel", "LowerIsBetter", false);
const StringModelProperty AssetEditorViewModel::FormattedValueProperty("AssetEditorViewModel", "FormattedValue", L"0");
const BoolModelProperty AssetEditorViewModel::PauseOnResetProperty("AssetEditorViewModel", "PauseOnReset", false);
const BoolModelProperty AssetEditorViewModel::PauseOnTriggerProperty("AssetEditorViewModel", "PauseOnTrigger", false);
const BoolModelProperty AssetEditorViewModel::DebugHighlightsEnabledProperty("AssetEditorViewModel", "DebugHighlightsEnabled", false);
const BoolModelProperty AssetEditorViewModel::DecimalPreferredProperty("AssetEditorViewModel", "DecimalPreferred", false);
const BoolModelProperty AssetEditorViewModel::IsAssetLoadedProperty("AssetEditorViewModel", "IsAssetLoaded", false);
const BoolModelProperty AssetEditorViewModel::HasAssetValidationErrorProperty("AssetEditorViewModel", "HasAssetValidationError", false);
const StringModelProperty AssetEditorViewModel::AssetValidationErrorProperty("AssetEditorViewModel", "AssetValidationError", L"");
const BoolModelProperty AssetEditorViewModel::HasAssetValidationWarningProperty("AssetEditorViewModel", "HasAssetValidationWarning", false);
const StringModelProperty AssetEditorViewModel::AssetValidationWarningProperty("AssetEditorViewModel", "AssetValidationWarning", L"");
const BoolModelProperty AssetEditorViewModel::HasMeasuredProperty("AssetEditorViewModel", "HasMeasured", false);
const StringModelProperty AssetEditorViewModel::MeasuredValueProperty("AssetEditorViewModel", "MeasuredValue", L"[Not Active]");
const StringModelProperty AssetEditorViewModel::WaitingLabelProperty("AssetEditorViewModel", "WaitingLabel", L"Active");

AssetEditorViewModel::AssetEditorViewModel() noexcept
{
    SetWindowTitle(L"Achievement Editor");

    m_vmTrigger.AddNotifyTarget(*this);

    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Value), L"Value");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Score), L"Score");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Frames), L"Frames");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Centiseconds), L"Centiseconds");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Seconds), L"Seconds");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Minutes), L"Minutes");
}

AssetEditorViewModel::~AssetEditorViewModel()
{
    if (m_pAsset)
    {
        m_pAsset->RemoveNotifyTarget(*this);
        m_pAsset = nullptr;
    }
}

void AssetEditorViewModel::SelectBadgeFile()
{
    ui::viewmodels::FileDialogViewModel vmFile;
    vmFile.AddFileType(L"Image Files", L"*.png;*.gif;*.jpg;*.jpeg");
    vmFile.SetDefaultExtension(L"png");
    if (vmFile.ShowOpenFileDialog(*this) != DialogResult::OK)
        return;

    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    const auto sBadgeName = pImageRepository.StoreImage(ImageType::Badge, vmFile.GetFileName());
    if (sBadgeName.empty())
    {
        ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::StringPrintf(L"Error processing %s", vmFile.GetFileName()));
        return;
    }

    SetBadge(ra::Widen(sBadgeName));
}

void AssetEditorViewModel::LoadAsset(ra::data::models::AssetModelBase* pAsset)
{
    if (m_pAsset == pAsset)
        return;

    if (HasAssetValidationError())
    {
        if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Discard changes?",
            L"The currently loaded asset has an error that cannot be saved. If you switch to another asset, your changes will be lost.",
            ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
        {
            return;
        }

        m_pAsset->RemoveNotifyTarget(*this);
        m_pAsset->RestoreLocalCheckpoint();

        SetValue(AssetValidationErrorProperty, L"");
    }
    else if (m_pAsset)
    {
        m_pAsset->RemoveNotifyTarget(*this);
    }

    // null out internal pointer while binding to prevent change triggers
    m_pAsset = nullptr;

    if (pAsset)
    {
        pAsset->AddNotifyTarget(*this);

        SetName(pAsset->GetName());
        SetDescription(pAsset->GetDescription());
        SetState(pAsset->GetState());
        SetCategory(pAsset->GetCategory());
        SetValue(AssetValidationWarningProperty, pAsset->GetValidationError());

        // normally this happens in OnValueChanged, but only if m_pAsset is set, which doesn't happen until later
        // and we don't want the other stuff in OnValueChanged to run right now
        if (pAsset->GetState() == ra::data::models::AssetState::Waiting)
            SetValue(WaitingLabelProperty, L"Waiting");
        else
            SetValue(WaitingLabelProperty, WaitingLabelProperty.GetDefaultValue());

        if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
            SetValue(IDProperty, 0);
        else
            SetValue(IDProperty, pAsset->GetID());

        const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(pAsset);
        if (pLeaderboard != nullptr)
        {
            SetWindowTitle(L"Leaderboard Editor");
            SetValue(IsAchievementProperty, false);
            SetValue(IsLeaderboardProperty, true);
            SetValue(HasMeasuredProperty, true);

            SetValueFormat(pLeaderboard->GetValueFormat());
            SetLowerIsBetter(pLeaderboard->IsLowerBetter());
        }
        else
        {
            SetWindowTitle(L"Achievement Editor");
            SetValue(IsLeaderboardProperty, false);
            SetValue(IsAchievementProperty, true);

            const auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pAsset);
            if (pAchievement != nullptr)
            {
                SetPoints(pAchievement->GetPoints());
                SetBadge(pAchievement->GetBadge());
                SetPauseOnReset(pAchievement->IsPauseOnReset());
                SetPauseOnTrigger(pAchievement->IsPauseOnTrigger());

                auto* pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAsset->GetID());
                if (pTrigger != nullptr)
                {
                    Trigger().InitializeFrom(*pTrigger);
                }
                else
                {
                    Trigger().InitializeFrom(pAchievement->GetTrigger(), pAchievement->GetCapturedHits());
                    pTrigger = Trigger().GetTriggerFromString();
                }

                SetValue(HasMeasuredProperty, (pTrigger != nullptr && pTrigger->measured_target != 0));
            }
            else
            {
                ra::data::models::CapturedTriggerHits pCapturedHits;
                Trigger().InitializeFrom("", pCapturedHits);
                SetPauseOnReset(PauseOnResetProperty.GetDefaultValue());
                SetPauseOnTrigger(PauseOnTriggerProperty.GetDefaultValue());
                SetValue(HasMeasuredProperty, false);
            }
        }

        m_pAsset = pAsset;
        SetValue(IsAssetLoadedProperty, true);

        UpdateMeasuredValue();
    }
    else
    {
        SetValue(IsAssetLoadedProperty, false);
        SetValue(HasMeasuredProperty, false);

        SetName(NameProperty.GetDefaultValue());
        SetValue(IDProperty, IDProperty.GetDefaultValue());
        SetState(ra::itoe<ra::data::models::AssetState>(StateProperty.GetDefaultValue()));
        SetDescription(DescriptionProperty.GetDefaultValue());
        SetCategory(ra::itoe<ra::data::models::AssetCategory>(CategoryProperty.GetDefaultValue()));
        SetPoints(PointsProperty.GetDefaultValue());
        SetBadge(BadgeProperty.GetDefaultValue());
        SetPauseOnReset(PauseOnResetProperty.GetDefaultValue());
        SetPauseOnTrigger(PauseOnTriggerProperty.GetDefaultValue());
        SetWindowTitle(L"Achievement Editor");
        SetValue(WaitingLabelProperty, WaitingLabelProperty.GetDefaultValue());
        SetValue(AssetValidationWarningProperty, AssetValidationErrorProperty.GetDefaultValue());

        ra::data::models::CapturedTriggerHits pCapturedHits;
        Trigger().InitializeFrom("", pCapturedHits);
    }
}

void AssetEditorViewModel::OnDataModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::data::models::AchievementModel::PauseOnResetProperty)
        SetPauseOnReset(args.tNewValue);
    else if (args.Property == ra::data::models::AchievementModel::PauseOnTriggerProperty)
        SetPauseOnTrigger(args.tNewValue);
}

void AssetEditorViewModel::OnDataModelStringValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::data::models::AssetModelBase::DescriptionProperty)
        SetDescription(args.tNewValue);
    else if (args.Property == ra::data::models::AssetModelBase::NameProperty)
        SetName(args.tNewValue);
    else if (args.Property == ra::data::models::AchievementModel::BadgeProperty)
        SetBadge(args.tNewValue);
    else if (args.Property == ra::data::models::AssetModelBase::ValidationErrorProperty)
        SetValue(AssetValidationWarningProperty, args.tNewValue);
}

void AssetEditorViewModel::OnDataModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::data::models::AchievementModel::TriggerProperty)
        UpdateTriggerBinding();
    else if (args.Property == ra::data::models::AssetModelBase::StateProperty)
        SetState(ra::itoe<ra::data::models::AssetState>(args.tNewValue));
    else if (args.Property == ra::data::models::AssetModelBase::CategoryProperty)
        SetCategory(ra::itoe<ra::data::models::AssetCategory>(args.tNewValue));
    else if (args.Property == ra::data::models::AchievementModel::PointsProperty)
        SetPoints(args.tNewValue);
    else if (args.Property == ra::data::models::AssetModelBase::IDProperty)
        SetValue(IDProperty, args.tNewValue);
}

void AssetEditorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == TriggerViewModel::VersionProperty)
        OnTriggerChanged();
}

void AssetEditorViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        if (args.Property == PauseOnResetProperty)
            pAchievement->SetPauseOnReset(args.tNewValue);
        else if (args.Property == PauseOnTriggerProperty)
            pAchievement->SetPauseOnTrigger(args.tNewValue);
    }
    else
    {
        auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
        if (pLeaderboard != nullptr)
        {
            if (args.Property == LowerIsBetterProperty)
                pLeaderboard->SetLowerIsBetter(args.tNewValue);
        }
    }

    if (args.Property == DebugHighlightsEnabledProperty)
    {
        if (args.tNewValue)
            UpdateDebugHighlights();
        else
            m_vmTrigger.UpdateColors(nullptr);
    }
    else if (args.Property == DecimalPreferredProperty)
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        pConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, args.tNewValue);
    }
    else if (args.Property == IsVisibleProperty)
    {
        if (args.tNewValue)
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            SetDecimalPreferred(pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));

            if (m_pAsset)
                m_vmTrigger.DoFrame();
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void AssetEditorViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (m_pAsset != nullptr)
    {
        if (args.Property == DescriptionProperty)
        {
            m_pAsset->SetDescription(args.tNewValue);
        }
        else if (args.Property == NameProperty)
        {
            m_pAsset->SetName(args.tNewValue);
        }
        else
        {
            auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
            if (pAchievement != nullptr)
            {
                if (args.Property == BadgeProperty)
                    pAchievement->SetBadge(args.tNewValue);
            }
        }
    }

    if (args.Property == AssetValidationErrorProperty)
        SetValue(HasAssetValidationErrorProperty, !args.tNewValue.empty());
    else if (args.Property == AssetValidationWarningProperty)
        SetValue(HasAssetValidationWarningProperty, !args.tNewValue.empty());

    WindowViewModelBase::OnValueChanged(args);
}

void AssetEditorViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_pAsset != nullptr)
    {
        if (args.Property == StateProperty)
        {
            const auto nOldState = ra::itoe<ra::data::models::AssetState>(args.tOldValue);
            const auto nNewState = ra::itoe<ra::data::models::AssetState>(args.tNewValue);

            // make sure to update the hit counts in the frame where the achievement triggers
            // before it's deactivated, which might remove it from the runtime
            if (nNewState == ra::data::models::AssetState::Triggered)
                Trigger().DoFrame();

            // update the state of the asset
            m_pAsset->SetState(nNewState);

            // if the asset became active, rebind to the actual trigger
            if (ra::data::models::AssetModelBase::IsActive(nNewState))
            {
                if (!ra::data::models::AssetModelBase::IsActive(nOldState))
                    UpdateTriggerBinding();
            }

            if (nNewState == ra::data::models::AssetState::Waiting)
                SetValue(WaitingLabelProperty, L"Waiting");
            else if (nOldState == ra::data::models::AssetState::Waiting)
                SetValue(WaitingLabelProperty, WaitingLabelProperty.GetDefaultValue());

            UpdateAssetFrameValues();
        }
        else if (args.Property == CategoryProperty)
        {
            m_pAsset->SetCategory(ra::itoe<ra::data::models::AssetCategory>(args.tNewValue));
        }
        else
        {
            auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
            if (pAchievement != nullptr)
            {
                if (args.Property == PointsProperty)
                    pAchievement->SetPoints(args.tNewValue);
            }
            else
            {
                auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
                if (pLeaderboard != nullptr)
                {
                    if (args.Property == ValueFormatProperty)
                    {
                        pLeaderboard->SetValueFormat(ra::itoe<ra::data::ValueFormat>(args.tNewValue));
                        UpdateMeasuredValue();
                    }
                }
            }
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void AssetEditorViewModel::OnTriggerChanged()
{
    auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        const std::string& sTrigger = Trigger().Serialize();

        const int nSize = rc_trigger_size(sTrigger.c_str());
        if (nSize < 0)
        {
            SetValue(AssetValidationErrorProperty, ra::Widen(rc_error_str(nSize)));

            // if the achievement is active, we have to disable it
            if (pAchievement->IsActive())
                pAchievement->SetState(ra::data::models::AssetState::Inactive);

            // we need to update the trigger to mark the asset as modified, and so any attempt to reset/revert
            // the trigger will cause a PropertyChanged event. but we can't allow UpdateTriggerBinding to try
            // to process it because the trigger is not valid.
            m_bIgnoreTriggerUpdate = true;
            pAchievement->SetTrigger(sTrigger);
            m_bIgnoreTriggerUpdate = false;
        }
        else
        {
            SetValue(AssetValidationErrorProperty, L"");

            pAchievement->SetTrigger(sTrigger);

            // if trigger has actually changed, the code will call back through UpdateTriggerBinding to update the UI
        }
    }
}

void AssetEditorViewModel::UpdateTriggerBinding()
{
    if (m_bIgnoreTriggerUpdate)
        return;

    const auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        const rc_trigger_t* pTrigger = nullptr;
        if (pAchievement->IsActive())
            pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAchievement->GetID());

        if (pTrigger != nullptr)
        {
            Trigger().UpdateFrom(*pTrigger);
        }
        else
        {
            Trigger().UpdateFrom(pAchievement->GetTrigger());
            pTrigger = Trigger().GetTriggerFromString();
        }

        SetValue(AssetValidationErrorProperty, L"");
        SetValue(HasMeasuredProperty, (pTrigger && pTrigger->measured_target != 0));
    }

    if (AreDebugHighlightsEnabled())
        UpdateDebugHighlights();

    UpdateMeasuredValue();
}

void AssetEditorViewModel::DoFrame()
{
    if (m_pAsset == nullptr || !m_pAsset->IsActive())
        return;

    if (IsVisible())
        UpdateAssetFrameValues();
}

void AssetEditorViewModel::UpdateAssetFrameValues()
{
    m_vmTrigger.DoFrame();

    if (AreDebugHighlightsEnabled())
        UpdateDebugHighlights();

    UpdateMeasuredValue();
}

void AssetEditorViewModel::UpdateDebugHighlights()
{
    const auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        const rc_trigger_t* pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAchievement->GetID());
        m_vmTrigger.UpdateColors(pTrigger);
    }
}

void AssetEditorViewModel::UpdateMeasuredValue()
{
    if (HasMeasured())
    {
        if (!m_pAsset || !m_pAsset->IsActive())
        {
            SetValue(MeasuredValueProperty, MeasuredValueProperty.GetDefaultValue());
        }
        else
        {
            const auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
            if (pAchievement != nullptr)
            {
                auto* pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAchievement->GetID());
                if (!pTrigger)
                {
                    pTrigger = Trigger().GetTriggerFromString();
                    if (!pTrigger)
                        return;
                }

                SetValue(MeasuredValueProperty, ra::StringPrintf(L"%d/%d", pTrigger->measured_value, pTrigger->measured_target));
            }
            else
            {
                const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
                if (pLeaderboard != nullptr)
                {
                    auto* pLeaderboardDefinition = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetLeaderboardDefinition(pLeaderboard->GetID());
                    if (pLeaderboardDefinition)
                    {
                        const unsigned nValue = pLeaderboardDefinition->value.value.value;
                        SetValue(MeasuredValueProperty, std::to_wstring(nValue));

                        char buffer[16];
                        rc_format_value(buffer, sizeof(buffer), nValue, ra::etoi(ValueFormat()));
                        SetValue(FormattedValueProperty, ra::Widen(buffer));
                    }
                }
            }
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
