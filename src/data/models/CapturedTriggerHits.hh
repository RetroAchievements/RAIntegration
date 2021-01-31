#ifndef RA_DATA_CAPTURED_TRIGGER_HITS_H
#define RA_DATA_CAPTURED_TRIGGER_HITS_H
#pragma once

#include "CapturedTriggerHits.hh"

#include <rcheevos\include\rcheevos.h>

namespace ra {
namespace data {
namespace models {

class CapturedTriggerHits
{
public:
    void Capture(const rc_trigger_t* pTrigger, const std::string& sTrigger);
    bool Restore(rc_trigger_t* pTrigger, const std::string& sTrigger) const;
    void Reset();

private:
    std::vector<unsigned> m_vCapturedHitCounts;
    std::string m_sCapturedHitsMD5;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_CAPTURED_TRIGGER_HITS_H
