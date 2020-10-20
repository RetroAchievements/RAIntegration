#include "AssetEditorViewModel.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetEditorViewModel::IDProperty("AssetEditorViewModel", "ID", 0);
const StringModelProperty AssetEditorViewModel::NameProperty("AssetEditorViewModel", "Name", L"[No Asset Loaded]");
const StringModelProperty AssetEditorViewModel::DescriptionProperty("AssetEditorViewModel", "Description", L"Open an asset from the Assets List");
const IntModelProperty AssetEditorViewModel::CategoryProperty("AssetEditorViewModel", "Category", ra::etoi(AssetCategory::Core));
const IntModelProperty AssetEditorViewModel::StateProperty("AssetEditorViewModel", "State", ra::etoi(AssetState::Inactive));
const IntModelProperty AssetEditorViewModel::PointsProperty("AssetEditorViewModel", "Points", 0);
const StringModelProperty AssetEditorViewModel::BadgeProperty("AssetEditorViewModel", "Badge", L"00000");
const BoolModelProperty AssetEditorViewModel::PauseOnResetProperty("AssetEditorViewModel", "PauseOnReset", false);
const BoolModelProperty AssetEditorViewModel::PauseOnTriggerProperty("AssetEditorViewModel", "PauseOnTrigger", false);
const BoolModelProperty AssetEditorViewModel::AssetLoadedProperty("AssetEditorViewModel", "AssetLoaded", false);

AssetEditorViewModel::AssetEditorViewModel() noexcept
{
    SetWindowTitle(L"Asset Editor");

    m_vmTrigger.AddNotifyTarget(*this);
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
    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Not implemented");
}

void AssetEditorViewModel::LoadAsset(AssetViewModelBase* pAsset)
{
    if (m_pAsset == pAsset)
        return;

    if (m_pAsset)
        m_pAsset->RemoveNotifyTarget(*this);

    // null out internal pointer while binding to prevent change triggers
    m_pAsset = nullptr;

    if (pAsset)
    {
        pAsset->AddNotifyTarget(*this);

        SetName(pAsset->GetName());
        SetDescription(pAsset->GetDescription());
        SetState(pAsset->GetState());
        SetCategory(pAsset->GetCategory());

        if (pAsset->GetCategory() == AssetCategory::Local)
            SetValue(IDProperty, 0);
        else
            SetValue(IDProperty, pAsset->GetID());

        const auto* pAchievement = dynamic_cast<AchievementViewModel*>(pAsset);
        if (pAchievement != nullptr)
        {
            SetPoints(pAchievement->GetPoints());
            SetBadge(pAchievement->GetBadge());
            SetPauseOnReset(pAchievement->IsPauseOnReset());
            SetPauseOnTrigger(pAchievement->IsPauseOnTrigger());
            SetWindowTitle(L"Achievement Editor");

            const auto* pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAsset->GetID());
            if (pTrigger != nullptr)
                Trigger().InitializeFrom(*pTrigger);
            else
                Trigger().InitializeFrom(pAchievement->GetTrigger());
        }
        else
        {
            Trigger().InitializeFrom("");
        }

        m_pAsset = pAsset;
        SetValue(AssetLoadedProperty, true);
    }
    else
    {
        SetValue(AssetLoadedProperty, false);

        SetName(NameProperty.GetDefaultValue());
        SetValue(IDProperty, IDProperty.GetDefaultValue());
        SetState(ra::itoe<AssetState>(StateProperty.GetDefaultValue()));
        SetDescription(DescriptionProperty.GetDefaultValue());
        SetCategory(ra::itoe<AssetCategory>(CategoryProperty.GetDefaultValue()));
        SetPoints(PointsProperty.GetDefaultValue());
        SetBadge(BadgeProperty.GetDefaultValue());
        SetPauseOnReset(PauseOnResetProperty.GetDefaultValue());
        SetPauseOnTrigger(PauseOnTriggerProperty.GetDefaultValue());
        SetWindowTitle(L"Asset Editor");

        Trigger().InitializeFrom("");
    }
}

void AssetEditorViewModel::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == AchievementViewModel::PauseOnResetProperty)
        SetPauseOnReset(args.tNewValue);
    else if (args.Property == AchievementViewModel::PauseOnTriggerProperty)
        SetPauseOnTrigger(args.tNewValue);
}

void AssetEditorViewModel::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == AssetViewModelBase::DescriptionProperty)
        SetDescription(args.tNewValue);
    else if (args.Property == AssetViewModelBase::NameProperty)
        SetName(args.tNewValue);
    else if (args.Property == AchievementViewModel::BadgeProperty)
        SetBadge(args.tNewValue);
}

void AssetEditorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == TriggerViewModel::VersionProperty)
        OnTriggerChanged();
    else if (args.Property == AssetViewModelBase::StateProperty)
        SetState(ra::itoe<AssetState>(args.tNewValue));
    else if (args.Property == AssetViewModelBase::CategoryProperty)
        SetCategory(ra::itoe<AssetCategory>(args.tNewValue));
    else if (args.Property == AchievementViewModel::PointsProperty)
        SetPoints(args.tNewValue);
    else if (args.Property == AssetViewModelBase::IDProperty)
        SetValue(IDProperty, args.tNewValue);
}

void AssetEditorViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    auto* pAchievement = dynamic_cast<AchievementViewModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        if (args.Property == PauseOnResetProperty)
            pAchievement->SetPauseOnReset(args.tNewValue);
        else if (args.Property == PauseOnTriggerProperty)
            pAchievement->SetPauseOnTrigger(args.tNewValue);
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
            auto* pAchievement = dynamic_cast<AchievementViewModel*>(m_pAsset);
            if (pAchievement != nullptr)
            {
                if (args.Property == BadgeProperty)
                    pAchievement->SetBadge(args.tNewValue);
            }
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void AssetEditorViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == DialogResultProperty)
    {
        if (m_pAsset)
        {
            m_pAsset->RemoveNotifyTarget(*this);
            m_pAsset = nullptr;
        }
    }
    else if (m_pAsset != nullptr)
    {
        if (args.Property == StateProperty)
        {
            m_pAsset->SetState(ra::itoe<AssetState>(args.tNewValue));
            UpdateTriggerBinding();
        }
        else if (args.Property == CategoryProperty)
        {
            m_pAsset->SetCategory(ra::itoe<AssetCategory>(args.tNewValue));
        }
        else
        {
            auto* pAchievement = dynamic_cast<AchievementViewModel*>(m_pAsset);
            if (pAchievement != nullptr)
            {
                if (args.Property == PointsProperty)
                    pAchievement->SetPoints(args.tNewValue);
            }
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void AssetEditorViewModel::OnTriggerChanged() noexcept
{
    auto* pAchievement = dynamic_cast<AchievementViewModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        const std::string& sTrigger = Trigger().Serialize();
        pAchievement->SetTrigger(sTrigger);

        if (m_pAsset->IsActive())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            pRuntime.ActivateAchievement(GetID(), sTrigger);
            UpdateTriggerBinding();
        }
    }
}

void AssetEditorViewModel::UpdateTriggerBinding()
{
    const auto* pAchievement = dynamic_cast<AchievementViewModel*>(m_pAsset);
    if (pAchievement != nullptr)
    {
        const auto* pTrigger = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetAchievementTrigger(pAchievement->GetID());
        if (pTrigger != nullptr)
            Trigger().UpdateFrom(*pTrigger);
        else
            Trigger().UpdateFrom(pAchievement->GetTrigger());
    }
}

void AssetEditorViewModel::DoFrame()
{
    if (m_pAsset == nullptr || !m_pAsset->IsActive())
        return;

    m_vmTrigger.DoFrame();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
