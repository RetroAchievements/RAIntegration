#include "RichPresenceMonitorViewModel.hh"

#include "RA_RichPresence.h"
#include "RA_StringUtils.h"

#include "data\GameContext.hh"

#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty RichPresenceMonitorViewModel::DisplayStringProperty("RichPresenceMonitorViewModel", "Message", L"No Rich Presence defined.");

RichPresenceMonitorViewModel::RichPresenceMonitorViewModel() noexcept
{
    SetWindowTitle(L"Rich Presence Monitor");
}

static time_t GetRichPresenceModified()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    const auto tLastModified = pLocalStorage.GetLastModified(ra::services::StorageItemType::RichPresence,
                                                             std::to_wstring(pGameContext.GameId()));
    return std::chrono::system_clock::to_time_t(tLastModified);
}

void RichPresenceMonitorViewModel::StartMonitoring()
{
    switch (m_nState)
    {
        default:
        case MonitorState::None:
            // not monitoring - start doing so.
            m_nState = MonitorState::Active;
            UpdateDisplayString();
            m_tRichPresenceFileTime = GetRichPresenceModified();
            ScheduleUpdateDisplayString();
            break;

        case MonitorState::Deactivated:
            // asked to stop, but haven't called callback yet, resurrect.
            m_nState = MonitorState::Active;
            break;

        case MonitorState::Static:
            // monitoring, but message is static, update it.
            UpdateDisplayString();
            break;

        case MonitorState::Active:
            // already monitoring, do nothing.
            break;
    }
}

void RichPresenceMonitorViewModel::StopMonitoring() noexcept
{
    switch (m_nState)
    {
        default:
        case MonitorState::Active:
            // monitoring, notify callback to stop rescheduling.
            m_nState = MonitorState::Deactivated;
            break;

        case MonitorState::Deactivated:
            // already asked to stop, but haven't processed it yet.
            break;

        case MonitorState::Static:
            // monitoring, but not updating the message, immediately transition to not monitoring
            m_nState = MonitorState::None;
            break;

        case MonitorState::None:
            // not monitoring, do nothing.
            break;
    }
}

void RichPresenceMonitorViewModel::ScheduleUpdateDisplayString()
{
    if (m_nState != MonitorState::Active)
        return;

    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(std::chrono::seconds(1), [this]()
    {
        if (m_nState == MonitorState::Deactivated)
        {
            // asked to stop, do so now
            m_nState = MonitorState::None;
        }
        else
        {
            // check to see if the script was updated
            const time_t tRichPresenceFileTime = GetRichPresenceModified();
            if (tRichPresenceFileTime != m_tRichPresenceFileTime)
            {
                ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().ReloadRichPresenceScript();
                m_tRichPresenceFileTime = tRichPresenceFileTime;
            }

            UpdateDisplayString();
            ScheduleUpdateDisplayString();
        }
    });
}

void RichPresenceMonitorViewModel::UpdateDisplayString()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        SetDisplayString(L"No game loaded.");

        if (m_nState == MonitorState::Active)
            m_nState = MonitorState::Static;
    }
    else if (!pGameContext.HasRichPresence())
    {
        SetDisplayString(DisplayStringProperty.GetDefaultValue());

        if (m_nState == MonitorState::Active)
            m_nState = MonitorState::Static;
    }
    else
    {
        std::wstring sDisplayString = ra::Widen(pGameContext.GetRichPresenceDisplayString());
        SetDisplayString(sDisplayString);

        if (m_nState == MonitorState::Static)
        {
            m_nState = MonitorState::Active;
            ScheduleUpdateDisplayString();
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
