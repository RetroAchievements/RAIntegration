#ifndef RA_DATA_CAPTURED_TRIGGER_HITS_H
#define RA_DATA_CAPTURED_TRIGGER_HITS_H
#pragma once

namespace ra {
namespace data {
namespace models {

class CapturedTriggerHits
{
public:
    void Capture(const rc_trigger_t* pTrigger, const std::string& sTrigger);
    bool Restore(rc_trigger_t* pTrigger, const std::string& sTrigger) const;

    void Capture(const rc_value_t* pValue, const std::string& sValue);
    bool Restore(rc_value_t* pValue, const std::string& sValue) const;

    void Reset() noexcept;

private:
    std::vector<unsigned> m_vCapturedHitCounts;
    std::string m_sCapturedHitsMD5;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_CAPTURED_TRIGGER_HITS_H
