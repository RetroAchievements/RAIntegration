#include "RichPresenceMonitorViewModel.hh"

#include "RA_RichPresence.h"

#include "data\GameContext.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty RichPresenceMonitorViewModel::DisplayStringProperty("RichPresenceMonitorViewModel", "Message", L"No Rich Presence defined.");

RichPresenceMonitorViewModel::RichPresenceMonitorViewModel() noexcept
{
    SetWindowTitle(L"Rich Presence Monitor");
}

void RichPresenceMonitorViewModel::UpdateDisplayString()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    auto nGameId = pGameContext.GameId();
    if (nGameId == 0)
    {
        SetDisplayString(L"No game loaded.");
    }
    else if (!g_RichPresenceInterpreter.Enabled())
    {
        SetDisplayString(DisplayStringProperty.GetDefaultValue());
    }
    else
    {
        std::wstring sDisplayString = ra::Widen(g_RichPresenceInterpreter.GetRichPresenceString());
        SetDisplayString(sDisplayString);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
