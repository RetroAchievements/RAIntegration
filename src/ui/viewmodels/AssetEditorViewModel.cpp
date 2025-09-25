#include "AssetEditorViewModel.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\LeaderboardModel.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include "ui\EditorTheme.hh"
#include "ui\ImageReference.hh"
#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

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
const IntModelProperty AssetEditorViewModel::SelectedLeaderboardPartProperty("AssetEditorViewModel", "SelectedLeaderboardPart", ra::etoi(AssetEditorViewModel::LeaderboardPart::Start));
const StringModelProperty AssetEditorViewModel::GroupsHeaderProperty("AssetEditorViewModel", "GroupsHeader", L"Groups:");
const IntModelProperty AssetEditorViewModel::ValueFormatProperty("AssetEditorViewModel", "ValueFormat", ra::etoi(ra::data::ValueFormat::Value));
const BoolModelProperty AssetEditorViewModel::LowerIsBetterProperty("AssetEditorViewModel", "LowerIsBetter", false);
const StringModelProperty AssetEditorViewModel::FormattedValueProperty("AssetEditorViewModel", "FormattedValue", L"0");
const BoolModelProperty AssetEditorViewModel::PauseOnResetProperty("AssetEditorViewModel", "PauseOnReset", false);
const BoolModelProperty AssetEditorViewModel::PauseOnTriggerProperty("AssetEditorViewModel", "PauseOnTrigger", false);
const BoolModelProperty AssetEditorViewModel::IsTriggerProperty("AssetEditorViewModel", "IsTrigger", true);
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
const IntModelProperty AssetEditorViewModel::AchievementTypeProperty("AssetEditorViewModel", "AchievementType", ra::etoi(ra::data::models::AchievementType::None));
const IntModelProperty AssetEditorViewModel::LeaderboardPartViewModel::ColorProperty("LeaderboardPartViewModel", "Color", 0);

AssetEditorViewModel::AssetEditorViewModel() noexcept
{
    SetWindowTitle(L"Achievement Editor");

    m_vmTrigger.AddNotifyTarget(*this);

    m_vAchievementTypes.Add(ra::etoi(ra::data::models::AchievementType::None), L"");
    m_vAchievementTypes.Add(ra::etoi(ra::data::models::AchievementType::Missable), L"Missable");
    m_vAchievementTypes.Add(ra::etoi(ra::data::models::AchievementType::Progression), L"Progression");
    m_vAchievementTypes.Add(ra::etoi(ra::data::models::AchievementType::Win), L"Win");

    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Score), L"Score");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Frames), L"Time (Frames)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Centiseconds), L"Time (Centiseconds)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Seconds), L"Time (Seconds)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Minutes), L"Time (Minutes)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::SecondsAsMinutes), L"Time (Seconds as Minutes)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Value), L"Value");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::UnsignedValue), L"Value (Unsigned)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Tens), L"Value (Tens)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Hundreds), L"Value (Hundreds)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Thousands), L"Value (Thousands)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Fixed1), L"Value (Fixed1)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Fixed2), L"Value (Fixed2)");
    m_vFormats.Add(ra::etoi(ra::data::ValueFormat::Fixed3), L"Value (Fixed3)");

    m_vLeaderboardParts.Add(ra::etoi(LeaderboardPart::Start), L"Start");
    m_vLeaderboardParts.Add(ra::etoi(LeaderboardPart::Cancel), L"Cancel");
    m_vLeaderboardParts.Add(ra::etoi(LeaderboardPart::Submit), L"Submit");
    m_vLeaderboardParts.Add(ra::etoi(LeaderboardPart::Value), L"Value");
    m_vLeaderboardParts.GetItemAt(0)->SetSelected(true);
    m_vLeaderboardParts.AddNotifyTarget(*this);

    SetValue(IsTriggerProperty, false);
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

    const auto& pFileName = vmFile.GetFileName();
    auto& pFileSystemService = ra::services::ServiceLocator::GetMutable<ra::services::IFileSystem>();
    const auto pFile = pFileSystemService.OpenTextFile(pFileName);
    if (!pFile)
    {
        ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::StringPrintf(L"Could not read %s", pFileName));
        return;
    }
    byte pHeader[16]{};
    pFile->GetBytes(pHeader, sizeof(pHeader));

    auto sExtension = ra::Narrow(pFileSystemService.GetExtension(pFileName));
    ra::StringMakeLowercase(sExtension);
    bool bValid = false;
    if (sExtension == "png")
        bValid = (memcmp(&pHeader[1], "PNG", 3) == 0);
    else if (sExtension == "gif")
        bValid = (memcmp(&pHeader[0], "GIF8", 4) == 0);
    else /* jpg/jpeg */
        bValid = (memcmp(&pHeader[6], "JFIF", 5) == 0);

    if (!bValid)
    {
        ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::StringPrintf(L"File does not appear to be a valid %s image.", sExtension));
        return;
    }

    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    const auto sBadgeName = pImageRepository.StoreImage(ImageType::Badge, pFileName);
    if (sBadgeName.empty())
    {
        ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::StringPrintf(L"Error processing %s", pFileName));
        return;
    }

    SetBadge(ra::Widen(sBadgeName));
}

void AssetEditorViewModel::LoadAsset(ra::data::models::AssetModelBase* pAsset, bool bForce)
{
    if (m_pAsset == pAsset)
        return;

    if (!bForce && HasAssetValidationError() && m_pAsset->GetChanges() != ra::data::models::AssetChanges::Deleted)
    {
        if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Discard changes?",
            L"The currently loaded asset has an error that cannot be saved. If you switch to another asset, your changes will be lost.",
            ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
        {
            return;
        }

        m_pAsset->RemoveNotifyTarget(*this);
        m_pAsset->RestoreLocalCheckpoint();
    }
    else if (m_pAsset)
    {
        m_pAsset->RemoveNotifyTarget(*this);
    }

    SetValue(AssetValidationErrorProperty, L"");

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

        Trigger().SetIsValue(false); // UpdateLeaderboardTrigger will set this back to true if necessary
        SetValue(GroupsHeaderProperty, GroupsHeaderProperty.GetDefaultValue());

        const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(pAsset);
        if (pLeaderboard != nullptr)
        {
            SetWindowTitle(L"Leaderboard Editor");
            SetValue(IsAchievementProperty, false);
            SetValue(IsLeaderboardProperty, true);
            SetValue(HasMeasuredProperty, true);

            SetValueFormat(pLeaderboard->GetValueFormat());
            SetLowerIsBetter(pLeaderboard->IsLowerBetter());

            m_pAsset = pAsset; // must be set before calling UpdateLeaderboardTrigger
            UpdateLeaderboardTrigger();
        }
        else
        {
            SetWindowTitle(L"Achievement Editor");
            SetValue(IsLeaderboardProperty, false);
            SetValue(IsAchievementProperty, true);
            SetValue(IsTriggerProperty, true);

            const auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pAsset);
            if (pAchievement != nullptr)
            {
                SetPoints(pAchievement->GetPoints());
                SetBadge(pAchievement->GetBadge());
                SetAchievementType(pAchievement->GetAchievementType());
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

        if (AreDebugHighlightsEnabled())
            UpdateDebugHighlights();
    }
    else
    {
        SetValue(IsAssetLoadedProperty, false);
        SetValue(IsTriggerProperty, false);
        SetValue(HasMeasuredProperty, false);

        SetName(NameProperty.GetDefaultValue());
        SetValue(IDProperty, IDProperty.GetDefaultValue());
        SetState(ra::itoe<ra::data::models::AssetState>(StateProperty.GetDefaultValue()));
        SetDescription(DescriptionProperty.GetDefaultValue());
        SetCategory(ra::itoe<ra::data::models::AssetCategory>(CategoryProperty.GetDefaultValue()));
        SetPoints(PointsProperty.GetDefaultValue());
        SetBadge(BadgeProperty.GetDefaultValue());
        SetAchievementType(ra::itoe<ra::data::models::AchievementType>(AchievementTypeProperty.GetDefaultValue()));
        SetPauseOnReset(PauseOnResetProperty.GetDefaultValue());
        SetPauseOnTrigger(PauseOnTriggerProperty.GetDefaultValue());
        SetWindowTitle(L"Achievement Editor");
        SetValue(WaitingLabelProperty, WaitingLabelProperty.GetDefaultValue());
        SetValue(AssetValidationWarningProperty, AssetValidationErrorProperty.GetDefaultValue());

        ra::data::models::CapturedTriggerHits pCapturedHits;
        Trigger().InitializeFrom("", pCapturedHits);
    }

    OnTriggerChanged(true);
}

static constexpr ra::data::models::LeaderboardModel::LeaderboardParts LeaderboardPartToParts(AssetEditorViewModel::LeaderboardPart nPart) noexcept
{
    switch (nPart)
    {
        case AssetEditorViewModel::LeaderboardPart::Start:
            return ra::data::models::LeaderboardModel::LeaderboardParts::Start;
        case AssetEditorViewModel::LeaderboardPart::Submit:
            return ra::data::models::LeaderboardModel::LeaderboardParts::Submit;
        case AssetEditorViewModel::LeaderboardPart::Cancel:
            return ra::data::models::LeaderboardModel::LeaderboardParts::Cancel;
        case AssetEditorViewModel::LeaderboardPart::Value:
            return ra::data::models::LeaderboardModel::LeaderboardParts::Value;
        default:
            return ra::data::models::LeaderboardModel::LeaderboardParts::None;
    }
}

void AssetEditorViewModel::UpdateLeaderboardTrigger()
{
    const auto nPart = GetSelectedLeaderboardPart();
    if (nPart == LeaderboardPart::Value)
    {
        SetValue(IsTriggerProperty, false);
        SetValue(GroupsHeaderProperty, L"Max of:");
        Trigger().SetIsValue(true);
    }
    else
    {
        SetValue(IsTriggerProperty, true);
        SetValue(GroupsHeaderProperty, GroupsHeaderProperty.GetDefaultValue());
        Trigger().SetIsValue(false);
    }

    const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
    if (pLeaderboard != nullptr)
    {
        using namespace ra::bitwise_ops;

        SetPauseOnReset((pLeaderboard->GetPauseOnReset() & LeaderboardPartToParts(nPart)) != ra::data::models::LeaderboardModel::LeaderboardParts::None);
        SetPauseOnTrigger((pLeaderboard->GetPauseOnTrigger() & LeaderboardPartToParts(nPart)) != ra::data::models::LeaderboardModel::LeaderboardParts::None);
    }
    else
    {
        SetPauseOnReset(false);
        SetPauseOnTrigger(false);
    }

    const auto* pLeaderboardDefinition = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetLeaderboardDefinition(m_pAsset->GetID());
    if (pLeaderboardDefinition != nullptr)
    {
        switch (nPart)
        {
            case LeaderboardPart::Start:
                Trigger().InitializeFrom(pLeaderboardDefinition->start);
                return;
            case LeaderboardPart::Submit:
                Trigger().InitializeFrom(pLeaderboardDefinition->submit);
                return;
            case LeaderboardPart::Cancel:
                Trigger().InitializeFrom(pLeaderboardDefinition->cancel);
                return;
            case LeaderboardPart::Value:
                Trigger().InitializeFrom(pLeaderboardDefinition->value);
                return;
            default:
                break;
        }
    }
    else if (pLeaderboard != nullptr)
    {
        switch (nPart)
        {
            case LeaderboardPart::Start:
                Trigger().InitializeFrom(pLeaderboard->GetStartTrigger(), pLeaderboard->GetStartCapturedHits());
                return;
            case LeaderboardPart::Submit:
                Trigger().InitializeFrom(pLeaderboard->GetSubmitTrigger(), pLeaderboard->GetSubmitCapturedHits());
                return;
            case LeaderboardPart::Cancel:
                Trigger().InitializeFrom(pLeaderboard->GetCancelTrigger(), pLeaderboard->GetCancelCapturedHits());
                return;
            case LeaderboardPart::Value:
                Trigger().InitializeFrom(pLeaderboard->GetValueDefinition(), pLeaderboard->GetValueCapturedHits());
                return;
            default:
                break;
        }
    }

    ra::data::models::CapturedTriggerHits pCapturedHits;
    Trigger().InitializeFrom("", pCapturedHits);
}

void AssetEditorViewModel::OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == LookupItemViewModel::IsSelectedProperty)
    {
        for (gsl::index i = 0; i < ra::to_signed(m_vLeaderboardParts.Count()); ++i)
        {
            const auto* pItem = m_vLeaderboardParts.GetItemAt(i);
            Expects(pItem != nullptr);
            if (pItem->IsSelected())
            {
                SetValue(SelectedLeaderboardPartProperty, pItem->GetId());
                return;
            }
        }

        SetSelectedLeaderboardPart(LeaderboardPart::None);
    }
}

void AssetEditorViewModel::OnDataModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::data::models::AchievementModel::PauseOnResetProperty)
        SetPauseOnReset(args.tNewValue);
    else if (args.Property == ra::data::models::AchievementModel::PauseOnTriggerProperty)
        SetPauseOnTrigger(args.tNewValue);
    else if (args.Property == ra::data::models::LeaderboardModel::LowerIsBetterProperty)
        SetLowerIsBetter(args.tNewValue);
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
    if (args.Property == ra::data::models::AchievementModel::TriggerProperty ||
        args.Property == ra::data::models::LeaderboardModel::StartTriggerProperty ||
        args.Property == ra::data::models::LeaderboardModel::SubmitTriggerProperty ||
        args.Property == ra::data::models::LeaderboardModel::CancelTriggerProperty ||
        args.Property == ra::data::models::LeaderboardModel::ValueDefinitionProperty)
    {
        UpdateTriggerBinding();
    }
    else if (args.Property == ra::data::models::AssetModelBase::StateProperty)
        SetState(ra::itoe<ra::data::models::AssetState>(args.tNewValue));
    else if (args.Property == ra::data::models::AssetModelBase::CategoryProperty)
        SetCategory(ra::itoe<ra::data::models::AssetCategory>(args.tNewValue));
    else if (args.Property == ra::data::models::AchievementModel::PointsProperty)
        SetPoints(args.tNewValue);
    else if (args.Property == ra::data::models::AchievementModel::AchievementTypeProperty)
        SetAchievementType(ra::itoe<ra::data::models::AchievementType>(args.tNewValue));
    else if (args.Property == ra::data::models::LeaderboardModel::ValueFormatProperty)
        SetValueFormat(ra::itoe<ra::data::ValueFormat>(args.tNewValue));
    else if (args.Property == ra::data::models::AssetModelBase::IDProperty)
        SetValue(IDProperty, args.tNewValue);
}

void AssetEditorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == TriggerViewModel::VersionProperty)
    {
        OnTriggerChanged(false);
    }
    else if (args.Property == TriggerViewModel::SelectedGroupIndexProperty ||
             args.Property == TriggerViewModel::ScrollOffsetProperty)
    {
        if (m_pAsset && AreDebugHighlightsEnabled())
            UpdateDebugHighlights();
    }
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
            {
                pLeaderboard->SetLowerIsBetter(args.tNewValue);
            }
            else if (args.Property == PauseOnResetProperty)
            {
                using namespace ra::bitwise_ops;

                auto nPauseOnResetParts = pLeaderboard->GetPauseOnReset();
                const auto nSelectedPart = LeaderboardPartToParts(GetSelectedLeaderboardPart());
                if (args.tNewValue)
                    nPauseOnResetParts |= nSelectedPart;
                else
                    nPauseOnResetParts &= ~nSelectedPart;

                pLeaderboard->SetPauseOnReset(nPauseOnResetParts);
            }
            else if (args.Property == PauseOnTriggerProperty)
            {
                using namespace ra::bitwise_ops;

                auto nPauseOnTriggerParts = pLeaderboard->GetPauseOnTrigger();
                const auto nSelectedPart = LeaderboardPartToParts(GetSelectedLeaderboardPart());
                if (args.tNewValue)
                    nPauseOnTriggerParts |= nSelectedPart;
                else
                    nPauseOnTriggerParts &= ~nSelectedPart;

                pLeaderboard->SetPauseOnTrigger(nPauseOnTriggerParts);
            }
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

        m_vmTrigger.ToggleDecimal();
    }
    else if (args.Property == IsVisibleProperty)
    {
        if (args.tNewValue)
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            SetDecimalPreferred(pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));

            if (m_pAsset)
                DispatchMemoryRead([this] { m_vmTrigger.DoFrame(); });
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
            HandleStateChanged(ra::itoe<ra::data::models::AssetState>(args.tOldValue),
                               ra::itoe<ra::data::models::AssetState>(args.tNewValue));
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
                else if (args.Property == AchievementTypeProperty)
                    pAchievement->SetAchievementType(ra::itoe<ra::data::models::AchievementType>(args.tNewValue));
            }
            else
            {
                auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
                if (pLeaderboard != nullptr)
                {
                    if (args.Property == SelectedLeaderboardPartProperty)
                    {
                        // disable notifications while updating the selected group to prevent re-entrancy
                        m_vLeaderboardParts.RemoveNotifyTarget(*this);

                        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vLeaderboardParts.Count()); ++nIndex)
                        {
                            auto* pItem = m_vLeaderboardParts.GetItemAt(nIndex);
                            if (pItem != nullptr)
                                pItem->SetSelected(pItem->GetId() == args.tNewValue);
                        }

                        m_vLeaderboardParts.AddNotifyTarget(*this);

                        UpdateLeaderboardTrigger();
                    }
                    else if (args.Property == ValueFormatProperty)
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

void AssetEditorViewModel::HandleStateChanged(ra::data::models::AssetState nOldState, ra::data::models::AssetState nNewState)
{
    // make sure to update the hit counts in the frame where the achievement triggers
    // before it's deactivated, which might remove it from the runtime
    if (nNewState == ra::data::models::AssetState::Triggered)
        Trigger().DoFrame();

    // update the state of the asset
    m_pAsset->SetState(nNewState);

    // if the asset became active, rebind to the actual trigger
    const bool bIsActive = ra::data::models::AssetModelBase::IsActive(nNewState);
    const bool bWasActive = ra::data::models::AssetModelBase::IsActive(nOldState);
    if (bIsActive && !bWasActive)
    {
        UpdateTriggerBinding();

        const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        const auto bActivateSucceeded = IsAchievement()
            ? (pRuntime.GetAchievementTrigger(m_pAsset->GetID()) != nullptr)
            : (pRuntime.GetLeaderboardDefinition(m_pAsset->GetID()) != nullptr);

        if (!bActivateSucceeded)
        {
            MessageBoxViewModel::ShowErrorMessage(*this, L"Activation failed.");
            // NOTE: cannot set State back to Inactive here as the this code is triggered
            // by the binding and the binding disables watching the model while it's firing
            // its notifications. Instead, let Asset.DoFrame() deactivate the asset and
            // that will fire events that will cause the binding to uncheck itself.
            return;
        }
    }

    // if the state changed to/from Waiting, update the Waiting label
    if (nNewState == ra::data::models::AssetState::Waiting)
        SetValue(WaitingLabelProperty, L"Waiting");
    else if (nOldState == ra::data::models::AssetState::Waiting)
        SetValue(WaitingLabelProperty, WaitingLabelProperty.GetDefaultValue());

    // if the trigger is active, update the counters in the display list
    // otherwise, just update the measured value.
    if (bIsActive)
        UpdateAssetFrameValues();
    else
        UpdateMeasuredValue();

    // if the achievement changed between active and inactive, update the active achievements
    if (bIsActive != bWasActive)
    {
        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        if (IsAchievement())
            pRuntime.UpdateActiveAchievements();
        else
            pRuntime.UpdateActiveLeaderboards();
    }
}

static void AppendTrigger(std::string& sDefinition, const std::string& sTrigger)
{
    if (sTrigger.empty())
        sDefinition += "0=1";
    else
        sDefinition += sTrigger;
}

void AssetEditorViewModel::OnTriggerChanged(bool bIsLoading)
{
    const std::string& sTrigger = Trigger().Serialize();
    int nSize = 0;

    auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(m_pAsset);
    auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);

    // validate the new trigger before we update the asset. if it can't be deserialized, we can't commit it.
    if (pAchievement != nullptr)
    {
        nSize = rc_trigger_size(sTrigger.c_str());
    }
    else if (pLeaderboard != nullptr)
    {
        const auto nSelectedPart = GetSelectedLeaderboardPart();

        std::string sDefinition = "STA:";
        if (nSelectedPart == LeaderboardPart::Start)
            AppendTrigger(sDefinition, sTrigger);
        else
            AppendTrigger(sDefinition, pLeaderboard->GetStartTrigger());

        sDefinition.append("::SUB:");
        if (nSelectedPart == LeaderboardPart::Submit)
            AppendTrigger(sDefinition, sTrigger);
        else
            AppendTrigger(sDefinition, pLeaderboard->GetSubmitTrigger());

        sDefinition.append("::CAN:");
        if (nSelectedPart == LeaderboardPart::Cancel)
            AppendTrigger(sDefinition, sTrigger);
        else
            AppendTrigger(sDefinition, pLeaderboard->GetCancelTrigger());

        sDefinition.append("::VAL:");
        if (nSelectedPart == LeaderboardPart::Value)
            AppendTrigger(sDefinition, sTrigger);
        else
            AppendTrigger(sDefinition, pLeaderboard->GetValueDefinition());

        nSize = rc_lboard_size(sDefinition.c_str());
    }

    if (nSize < 0)
    {
        SetValue(AssetValidationErrorProperty, ra::Widen(rc_error_str(nSize)));

        // if the achievement is active, we have to disable it
        if (m_pAsset->IsActive())
            m_pAsset->SetState(ra::data::models::AssetState::Inactive);

        // we need to update the trigger to mark the asset as modified, and so any attempt to reset/revert
        // the trigger will cause a PropertyChanged event. but we can't allow UpdateTriggerBinding to try
        // to process it because the trigger is not valid.
        m_bIgnoreTriggerUpdate = true;
    }
    else
    {
        SetValue(AssetValidationErrorProperty, L"");
    }

    if (!bIsLoading)
    {
        if (pAchievement != nullptr)
        {
            pAchievement->SetTrigger(sTrigger);
        }
        else if (pLeaderboard != nullptr)
        {
            switch (GetSelectedLeaderboardPart())
            {
                case LeaderboardPart::Start:
                    pLeaderboard->SetStartTrigger(sTrigger);
                    break;
                case LeaderboardPart::Submit:
                    pLeaderboard->SetSubmitTrigger(sTrigger);
                    break;
                case LeaderboardPart::Cancel:
                    pLeaderboard->SetCancelTrigger(sTrigger);
                    break;
                case LeaderboardPart::Value:
                    pLeaderboard->SetValueDefinition(sTrigger);
                    break;
            }
        }
    }

    if (nSize < 0)
    {
        m_bIgnoreTriggerUpdate = false;
    }
    else
    {
        auto* pGroup = Trigger().Groups().GetItemAt(Trigger().GetSelectedGroupIndex());
        if (pGroup && pGroup->m_pConditionSet == nullptr)
        {
            // if the trigger didn't actually change, UpdateTriggerBinding won't get called, forcibly do so now.
            UpdateTriggerBinding();
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
    else
    {
        const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
        if (pLeaderboard != nullptr)
        {
            const rc_lboard_t* pLeaderboardDefinition = nullptr;
            if (pLeaderboard->IsActive())
                pLeaderboardDefinition = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetLeaderboardDefinition(pLeaderboard->GetID());

            if (pLeaderboardDefinition != nullptr)
            {
                switch (GetSelectedLeaderboardPart())
                {
                    case LeaderboardPart::Start:
                        Trigger().UpdateFrom(pLeaderboardDefinition->start);
                        break;
                    case LeaderboardPart::Submit:
                        Trigger().UpdateFrom(pLeaderboardDefinition->submit);
                        break;
                    case LeaderboardPart::Cancel:
                        Trigger().UpdateFrom(pLeaderboardDefinition->cancel);
                        break;
                    case LeaderboardPart::Value:
                        Trigger().UpdateFrom(pLeaderboardDefinition->value);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                switch (GetSelectedLeaderboardPart())
                {
                    case LeaderboardPart::Start:
                        Trigger().UpdateFrom(pLeaderboard->GetStartTrigger());
                        break;
                    case LeaderboardPart::Submit:
                        Trigger().UpdateFrom(pLeaderboard->GetSubmitTrigger());
                        break;
                    case LeaderboardPart::Cancel:
                        Trigger().UpdateFrom(pLeaderboard->GetCancelTrigger());
                        break;
                    case LeaderboardPart::Value:
                        Trigger().UpdateFrom(pLeaderboard->GetValueDefinition());
                        break;
                    default:
                        break;
                }
            }

            SetValue(AssetValidationErrorProperty, L"");
        }
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

static void UpdateLeaderboardPartColor(AssetEditorViewModel::LeaderboardPartViewModel& pPart,
    const rc_trigger_t& pTrigger, const ra::ui::EditorTheme& pTheme, const ra::ui::Color nDefaultColor)
{
    switch (pTrigger.state)
    {
        case RC_TRIGGER_STATE_PAUSED:
            pPart.SetColor(pTheme.ColorTriggerPauseTrue());
            break;
        case RC_TRIGGER_STATE_TRIGGERED:
            pPart.SetColor(pTheme.ColorTriggerIsTrue());
            break;
        default:
            if (pPart.m_nPreviousHits)
            {
                if (!pTrigger.has_hits)
                    pPart.SetColor(pTheme.ColorTriggerResetTrue());
                else
                    pPart.SetColor(nDefaultColor);
            }
            else
            {
                pPart.SetColor(nDefaultColor);
            }
            break;
    }

    pPart.m_nPreviousHits = pTrigger.has_hits;
}

static bool AreAllLeaderboardValuesPaused(const rc_condset_t* pValue) noexcept
{
    for (; pValue != nullptr; pValue = pValue->next)
    {
        if (!pValue->has_pause)
            return false;
        if (!pValue->is_paused)
            return false;
    }

    return true;
}

static void UpdateLeaderboardPartColors(ViewModelCollection<AssetEditorViewModel::LeaderboardPartViewModel>& vLeaderboardParts, const rc_lboard_t* pLeaderboard)
{
    const auto nDefaultColor = ra::ui::Color(ra::to_unsigned(AssetEditorViewModel::LeaderboardPartViewModel::ColorProperty.GetDefaultValue()));

    if (pLeaderboard == nullptr)
    {
        // no leaderboard, or leaderboard not active, disable highlights
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vLeaderboardParts.Count()); ++i)
        {
            auto* pLeaderboardPart = vLeaderboardParts.GetItemAt(i);
            if (pLeaderboardPart != nullptr)
                pLeaderboardPart->SetColor(nDefaultColor);
        }

        return;
    }

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    if (pLeaderboard->state == RC_LBOARD_STATE_STARTED)
        UpdateLeaderboardPartColor(*vLeaderboardParts.GetItemAt(0), pLeaderboard->start, pTheme, pTheme.ColorTriggerWasTrue());
    else
        UpdateLeaderboardPartColor(*vLeaderboardParts.GetItemAt(0), pLeaderboard->start, pTheme, nDefaultColor);

    UpdateLeaderboardPartColor(*vLeaderboardParts.GetItemAt(1), pLeaderboard->submit, pTheme, nDefaultColor);
    UpdateLeaderboardPartColor(*vLeaderboardParts.GetItemAt(2), pLeaderboard->cancel, pTheme, nDefaultColor);

    GSL_SUPPRESS_CON4
    {
        auto nValueColor = nDefaultColor;
        if (pLeaderboard->value.conditions != nullptr)
        {
            if (AreAllLeaderboardValuesPaused(pLeaderboard->value.conditions))
                nValueColor = pTheme.ColorTriggerPauseTrue();
        }

        vLeaderboardParts.GetItemAt(3)->SetColor(nValueColor);
    }
}

void AssetEditorViewModel::UpdateDebugHighlights()
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    const rc_trigger_t* pTrigger = nullptr;

    switch (m_pAsset->GetType())
    {
        case ra::data::models::AssetType::Achievement:
            pTrigger = pRuntime.GetAchievementTrigger(m_pAsset->GetID());
            break;

        case ra::data::models::AssetType::Leaderboard:
        {
            const auto* pLeaderboardDefinition = pRuntime.GetLeaderboardDefinition(m_pAsset->GetID());
            if (pLeaderboardDefinition == nullptr)
                break;

            UpdateLeaderboardPartColors(m_vLeaderboardParts, pLeaderboardDefinition);

            switch (GetSelectedLeaderboardPart())
            {
                case LeaderboardPart::Start:
                    pTrigger = &pLeaderboardDefinition->start;
                    break;
                case LeaderboardPart::Submit:
                    pTrigger = &pLeaderboardDefinition->submit;
                    break;
                case LeaderboardPart::Cancel:
                    pTrigger = &pLeaderboardDefinition->cancel;
                    break;
                case LeaderboardPart::Value:
                    // pTrigger = pLeaderboardDefinition->value.conditions;
                    break;
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    m_vmTrigger.UpdateColors(pTrigger);
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

                const auto nValue = (pTrigger->measured_value == RC_MEASURED_UNKNOWN) ? 0 : pTrigger->measured_value;
                SetValue(MeasuredValueProperty, ra::StringPrintf(L"%d/%d", nValue, pTrigger->measured_target));
            }
            else
            {
                const auto* pLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(m_pAsset);
                if (pLeaderboard != nullptr)
                {
                    const auto* pLeaderboardDefinition = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetLeaderboardDefinition(pLeaderboard->GetID());
                    if (pLeaderboardDefinition)
                    {
                        const unsigned nValue = pLeaderboardDefinition->value.value.value;
                        SetValue(MeasuredValueProperty, std::to_wstring(nValue));

                        char buffer[16];
                        rc_format_value(buffer, sizeof(buffer), nValue, ra::etoi(GetValueFormat()));
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
