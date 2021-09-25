#include "TriggerValidation.hh"

#include "RA_StringUtils.h"

#include <rcheevos/include/rc_runtime_types.h>

namespace ra {
namespace data {
namespace models {

static bool ValidateCondSet(const rc_condset_t* pCondSet, std::wstring& sError)
{
    bool bInAddHits = false;
    bool bIsCombining = false;

    int nIndex = 1;
    const rc_condition_t* pCondition = pCondSet->conditions;
    while (pCondition != nullptr)
    {
        switch (pCondition->type)
        {
            case RC_CONDITION_ADD_HITS:
            case RC_CONDITION_SUB_HITS:
                bIsCombining = true;
                bInAddHits = true;
                break;

            case RC_CONDITION_ADD_SOURCE:
            case RC_CONDITION_SUB_SOURCE:
            case RC_CONDITION_ADD_ADDRESS:
            case RC_CONDITION_AND_NEXT:
            case RC_CONDITION_OR_NEXT:
            case RC_CONDITION_RESET_NEXT_IF:
                bIsCombining = true;
                break;

            default:
                if (bInAddHits)
                {
                    if (pCondition->required_hits == 0)
                    {
                        sError = ra::StringPrintf(L"Condition %u: Final condition in AddHits chain must have a hit target.", nIndex);
                        return false;
                    }

                    bInAddHits = false;
                }

                bIsCombining = false;
                break;
        }

        ++nIndex;
        pCondition = pCondition->next;
    }

    if (bIsCombining)
    {
        sError = L"Final condition type expects another condition to follow.";
        return false;
    }

    return true;
}

static bool ValidateTrigger(const rc_trigger_t* pTrigger, std::wstring& sError)
{
    if (pTrigger->requirement && !ValidateCondSet(pTrigger->requirement, sError))
    {
        if (pTrigger->alternative != nullptr)
            sError.insert(0, L"Core: ");

        return false;
    }

    int nAlt = 1;
    const rc_condset_t* pCondSet = pTrigger->alternative;
    while (pCondSet != nullptr)
    {
        if (!ValidateCondSet(pCondSet, sError))
        {
            sError.insert(0, L"Alt : ");
            sError.insert(4, std::to_wstring(nAlt));
            return false;
        }

        ++nAlt;
        pCondSet = pCondSet->next;
    }

    return true;
}

bool TriggerValidation::Validate(const std::string& sTrigger, std::wstring& sError)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize < 0)
    {
        sError = ra::Widen(rc_error_str(nSize));
        return false;
    }

    std::string sTriggerBuffer;
    sTriggerBuffer.resize(nSize);
    const auto* pTrigger = rc_parse_trigger(sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);

    return ValidateTrigger(pTrigger, sError);
}

} // namespace models
} // namespace data
} // namespace ra
