#include "AchievementRuntime.hh"

#include "RA_Log.h"
#include "RA_MemManager.h"
#include "RA_StringUtils.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"

namespace ra {
namespace services {

static constexpr bool HasHitCounts(const rc_condset_t* pCondSet) noexcept
{
    rc_condition_t* condition = pCondSet->conditions;
    while (condition != nullptr)
    {
        if (condition->current_hits)
            return true;

        condition = condition->next;
    }

    return false;
}

static constexpr bool HasHitCounts(const rc_trigger_t* pTrigger) noexcept
{
    if (HasHitCounts(pTrigger->requirement))
        return true;

    rc_condset_t* pAlternate = pTrigger->alternative;
    while (pAlternate != nullptr)
    {
        if (HasHitCounts(pAlternate))
            return true;

        pAlternate = pAlternate->next;
    }

    return false;
}

void AchievementRuntime::Process(std::vector<Change>& changes) const noexcept
{
    for (auto& pAchievement : m_vActiveAchievements)
    {
        const bool bResult = rc_test_trigger(pAchievement.pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId});
    }

    for (auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        const bool bHasHits = HasHitCounts(pAchievement.pTrigger);

        const bool bResult = rc_test_trigger(pAchievement.pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId});
        else if (bHasHits && !HasHitCounts(pAchievement.pTrigger))
            changes.emplace_back(Change{ChangeType::AchievementReset, pAchievement.nId});
    }
}

} // namespace services
} // namespace ra
