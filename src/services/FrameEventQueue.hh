#ifndef RA_SERVICES_FRAME_EVENT_QUEUE_HH
#define RA_SERVICES_FRAME_EVENT_QUEUE_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"

namespace ra {
namespace services {

class FrameEventQueue
{
public:
    GSL_SUPPRESS_F6 FrameEventQueue() = default;
    virtual ~FrameEventQueue() = default;

    FrameEventQueue(const FrameEventQueue&) noexcept = delete;
    FrameEventQueue& operator=(const FrameEventQueue&) noexcept = delete;
    FrameEventQueue(FrameEventQueue&&) noexcept = delete;
    FrameEventQueue& operator=(FrameEventQueue&&) noexcept = delete;

    void QueuePauseOnChange(const std::wstring& sMemDescription)
    {
        m_vMemChanges.insert(sMemDescription);
    }

    void QueuePauseOnReset(const std::wstring& sTriggerName)
    {
        m_vResetTriggers.push_back(sTriggerName);
    }

    void QueuePauseOnTrigger(const std::wstring& sTriggerName)
    {
        m_vTriggeredTriggers.push_back(sTriggerName);
    }

    void DoFrame();

protected:
    std::set<std::wstring> m_vMemChanges;
    std::vector<std::wstring> m_vResetTriggers;
    std::vector<std::wstring> m_vTriggeredTriggers;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FRAME_EVENT_QUEUE_HH
