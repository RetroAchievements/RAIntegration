#include "CapturedTriggerHits.hh"

#include "RA_md5factory.h"

namespace ra {
namespace data {
namespace models {

static void CaptureCondSetHitCounts(const rc_condset_t* pCondSet, std::vector<unsigned>& vCapturedHitCounts)
{
    const rc_condition_t* pCondition = pCondSet->conditions;
    while (pCondition != nullptr)
    {
        vCapturedHitCounts.push_back(pCondition->current_hits);
        pCondition = pCondition->next;
    }
}

static void CaptureHitCounts(const rc_trigger_t* pTrigger, std::vector<unsigned>& vCapturedHitCounts)
{
    if (pTrigger->requirement)
        CaptureCondSetHitCounts(pTrigger->requirement, vCapturedHitCounts);

    const rc_condset_t* pCondSet = pTrigger->alternative;
    while (pCondSet)
    {
        CaptureCondSetHitCounts(pCondSet, vCapturedHitCounts);
        pCondSet = pCondSet->next;
    }
}

static void RestoreCondSetHitCounts(rc_condset_t* pCondSet, const std::vector<unsigned>& vCapturedHitCounts, gsl::index& nIndex)
{
    if (vCapturedHitCounts.empty())
    {
        rc_condition_t* pCondition = pCondSet->conditions;
        while (pCondition != nullptr)
        {
            pCondition->current_hits = 0;
            pCondition = pCondition->next;
        }
    }
    else
    {
        rc_condition_t* pCondition = pCondSet->conditions;
        while (pCondition != nullptr)
        {
            pCondition->current_hits = vCapturedHitCounts.at(nIndex++);
            pCondition = pCondition->next;
        }
    }
}

static void RestoreHitCounts(rc_trigger_t* pTrigger, const std::vector<unsigned>& vCapturedHitCounts)
{
    gsl::index nIndex = 0;
    if (pTrigger->requirement)
        RestoreCondSetHitCounts(pTrigger->requirement, vCapturedHitCounts, nIndex);

    rc_condset_t* pCondSet = pTrigger->alternative;
    while (pCondSet)
    {
        RestoreCondSetHitCounts(pCondSet, vCapturedHitCounts, nIndex);
        pCondSet = pCondSet->next;
    }
}

void CapturedTriggerHits::Capture(const rc_trigger_t* pTrigger, const std::string& sTrigger)
{
    m_vCapturedHitCounts.clear();

    m_sCapturedHitsMD5 = RAGenerateMD5(sTrigger);

    if (pTrigger->has_hits)
        CaptureHitCounts(pTrigger, m_vCapturedHitCounts);
}

bool CapturedTriggerHits::Restore(rc_trigger_t* pTrigger, const std::string& sTrigger) const
{
    if (m_sCapturedHitsMD5.empty())
        return false;

    const auto sCapturedHitsMD5 = RAGenerateMD5(sTrigger);
    if (sCapturedHitsMD5 != m_sCapturedHitsMD5)
        return false;

    RestoreHitCounts(pTrigger, m_vCapturedHitCounts);
    pTrigger->has_hits = !m_vCapturedHitCounts.empty();

    return true;
}

void CapturedTriggerHits::Capture(const rc_value_t* pValue, const std::string& sValue)
{
    m_vCapturedHitCounts.clear();

    m_sCapturedHitsMD5 = RAGenerateMD5(sValue);

    for (const auto* conditions = pValue->conditions; conditions; conditions = conditions->next)
        CaptureCondSetHitCounts(conditions, m_vCapturedHitCounts);
}

bool CapturedTriggerHits::Restore(rc_value_t* pValue, const std::string& sValue) const
{
    if (m_sCapturedHitsMD5.empty())
        return false;

    const auto sCapturedHitsMD5 = RAGenerateMD5(sValue);
    if (sCapturedHitsMD5 != m_sCapturedHitsMD5)
        return false;

    gsl::index nIndex = 0;
    for (auto* conditions = pValue->conditions; conditions; conditions = conditions->next)
        RestoreCondSetHitCounts(conditions, m_vCapturedHitCounts, nIndex);

    return true;
}

void CapturedTriggerHits::Reset() noexcept
{
    m_vCapturedHitCounts.clear();
    m_sCapturedHitsMD5.clear();
}

} // namespace models
} // namespace data
} // namespace ra
