#include "AchievementViewModel.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AchievementViewModel::PointsProperty("AchievementViewModel", "Points", 5);
const StringModelProperty AchievementViewModel::BadgeProperty("AchievementViewModel", "Badge", L"00000");
const BoolModelProperty AchievementViewModel::PauseOnResetProperty("AchievementViewModel", "PauseOnReset", false);
const BoolModelProperty AchievementViewModel::PauseOnTriggerProperty("AchievementViewModel", "PauseOnTrigger", false);
const IntModelProperty AchievementViewModel::TriggerProperty("AchievementViewModel", "Trigger", ra::etoi(AssetChanges::None));

AchievementViewModel::AchievementViewModel() noexcept
{
    SetValue(TypeProperty, ra::etoi(AssetType::Achievement));

    SetTransactional(PointsProperty);
    SetTransactional(BadgeProperty);

    GSL_SUPPRESS_F6 AddAssetDefinition(m_pTrigger, TriggerProperty);
}

void AchievementViewModel::SelectBadgeFile()
{
    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Not implemented");
}

void AchievementViewModel::Serialize(ra::services::TextWriter& pWriter) const
{

}

} // namespace viewmodels
} // namespace ui
} // namespace ra
