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
    rc_condition_t* pCondition = pCondSet->conditions;
    while (pCondition != nullptr)
    {
        pCondition->current_hits = vCapturedHitCounts.at(nIndex++);
        pCondition = pCondition->next;
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

    if (!m_vCapturedHitCounts.empty())
        RestoreHitCounts(pTrigger, m_vCapturedHitCounts);

    return true;
}

void CapturedTriggerHits::Reset()
{
    m_vCapturedHitCounts.clear();
    m_sCapturedHitsMD5.clear();
}

} // namespace models
} // namespace data
} // namespace ra
