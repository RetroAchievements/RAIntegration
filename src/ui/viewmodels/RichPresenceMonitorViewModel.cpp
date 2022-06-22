#include "RichPresenceMonitorViewModel.hh"

#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
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

void RichPresenceMonitorViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);
}

static time_t GetRichPresenceModified()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    const auto tLastModified = pLocalStorage.GetLastModified(ra::services::StorageItemType::RichPresence,
                                                             std::to_wstring(pGameContext.GameId()));
    return std::chrono::system_clock::to_time_t(tLastModified);
}

void RichPresenceMonitorViewModel::StartMonitoring()
{
    UpdateWindowTitle();

    switch (m_nState)
    {
        default:
        case MonitorState::None:
            // not monitoring - start doing so.
            m_nState = MonitorState::Active;
            m_tRichPresenceFileTime = GetRichPresenceModified();
            if (m_tRichPresenceFileTime)
            {
                auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
                auto* pRichPresence = pGameContext.Assets().FindRichPresence();
                if (pRichPresence != nullptr)
                {
                    pRichPresence->ReloadRichPresenceScript();

                    if (pRichPresence->GetChanges() != ra::data::models::AssetChanges::None &&
                        ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsFeatureEnabled(ra::services::Feature::Hardcore))
                    {
                        pRichPresence->Deactivate();
                    }
                    else
                    {
                        pRichPresence->Activate();
                    }
                }
            }
            UpdateDisplayString();
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
                auto* pRichPresence = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>().Assets().FindRichPresence();
                if (pRichPresence)
                    pRichPresence->ReloadRichPresenceScript();
                m_tRichPresenceFileTime = tRichPresenceFileTime;

                UpdateWindowTitle();
            }

            UpdateDisplayString();
            ScheduleUpdateDisplayString();
        }
    });
}

void RichPresenceMonitorViewModel::UpdateDisplayString()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        SetDisplayString(L"No game loaded.");
    }
    else
    {
        const auto* pRichPresence = pGameContext.Assets().FindRichPresence();
        if (!pRichPresence)
        {
            SetDisplayString(DisplayStringProperty.GetDefaultValue());
        }
        else if (pRichPresence->GetState() != ra::data::models::AssetState::Active)
        {
            SetDisplayString(L"Rich Presence not active.");
        }
        else
        {
            auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
            const std::wstring sDisplayString = ra::Widen(pRuntime.GetRichPresenceDisplayString());
            SetDisplayString(sDisplayString);

            if (m_nState == MonitorState::Static)
            {
                m_nState = MonitorState::Active;
                ScheduleUpdateDisplayString();
            }

            return;
        }
    }

    if (m_nState == MonitorState::Active)
        m_nState = MonitorState::Static;
}

void RichPresenceMonitorViewModel::UpdateWindowTitle()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pRichPresence = pGameContext.Assets().FindRichPresence();
    if (pRichPresence && pRichPresence->GetChanges() != ra::data::models::AssetChanges::None)
        SetWindowTitle(L"Rich Presence Monitor (local)");
    else
        SetWindowTitle(L"Rich Presence Monitor");
}

void RichPresenceMonitorViewModel::OnActiveGameChanged()
{
    if (IsVisible())
    {
        UpdateWindowTitle();
        UpdateDisplayString();
    }
}

void RichPresenceMonitorViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        if (args.tNewValue)
        {
            UpdateDisplayString();
            StartMonitoring();
        }
        else
        {
            StopMonitoring();
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
