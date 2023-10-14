#include "RichPresenceMonitorViewModel.hh"

#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ILocalStorage.hh"
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

void RichPresenceMonitorViewModel::UpdateDisplayString()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto nGameId = pGameContext.GameId();
    if (nGameId == 0)
    {
        SetDisplayString(L"No game loaded.");
        return;
    }

    auto* pRichPresence = pGameContext.Assets().FindRichPresence();

    // check to see if the script was updated
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    const auto tLastModified = pLocalStorage.GetLastModified(ra::services::StorageItemType::RichPresence,
                                                             std::to_wstring(nGameId));
    const auto tRichPresenceFileTime = std::chrono::system_clock::to_time_t(tLastModified);
    if (tRichPresenceFileTime == m_tRichPresenceFileTime)
    {
        // not updated. if no model, set default string and bail
        if (!pRichPresence)
        {
            SetDisplayString(DisplayStringProperty.GetDefaultValue());
            return;
        }
    }
    else
    {
        m_tRichPresenceFileTime = tRichPresenceFileTime;

        if (!pRichPresence)
        {
            // new file detected, load it and activate
            auto pNewRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
            pNewRichPresence->CreateServerCheckpoint();
            pNewRichPresence->CreateLocalCheckpoint();
            pRichPresence = dynamic_cast<ra::data::models::RichPresenceModel*>(
                &pGameContext.Assets().Append(std::move(pNewRichPresence)));
        }

        // modified rich presence cannot be active in hardcore
        const bool bHardcore = ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsFeatureEnabled(
            ra::services::Feature::Hardcore);
        if (bHardcore)
            pRichPresence->Deactivate();

        // load the updated file
        pRichPresence->ReloadRichPresenceScript();

        // automatically activate if not in hardcore
        if (!bHardcore)
            pRichPresence->Activate();

        UpdateWindowTitle();
    }

    switch (pRichPresence->GetState())
    {
        case ra::data::models::AssetState::Active:
        case ra::data::models::AssetState::Disabled: // parse error, still display it
        {
            auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
            const std::wstring sDisplayString = ra::Widen(pRuntime.GetRichPresenceDisplayString());
            SetDisplayString(sDisplayString);
            return;
        }

        default:
            SetDisplayString(L"Rich Presence not active.");
            break;
    }
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
    UpdateDisplayString();
    UpdateWindowTitle();
}

void RichPresenceMonitorViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        if (args.tNewValue)
            UpdateDisplayString();
    }

    WindowViewModelBase::OnValueChanged(args);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
