#ifndef RA_SERVICES_MOCK_FRAME_EVENT_QUEUE_HH
#define RA_SERVICES_MOCK_FRAME_EVENT_QUEUE_HH
#pragma once

#include "services\FrameEventQueue.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockFrameEventQueue : public FrameEventQueue
{
public:
    MockFrameEventQueue() noexcept : m_Override(this)
    {
    }

    size_t NumTriggeredTriggers() const noexcept { return m_vTriggeredTriggers.size(); }
    size_t NumResetTriggers() const noexcept { return m_vResetTriggers.size(); }
    size_t NumMemoryChanges() const noexcept { return m_vMemChanges.size(); }

    bool ContainsMemoryChange(const std::wstring& sChange) const
    {
        return (m_vMemChanges.find(sChange) != m_vMemChanges.end());
    }

    void Reset() noexcept
    {
        m_vTriggeredTriggers.clear();
        m_vResetTriggers.clear();
        m_vMemChanges.clear();
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::FrameEventQueue> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_FRAME_EVENT_QUEUE_HH
