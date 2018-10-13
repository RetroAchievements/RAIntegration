#include "RichPresenceMonitorViewModel.hh"

#include "RA_GameData.h"
#include "RA_RichPresence.h"
#include "RA_StringUtils.h"

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
    auto nGameId = g_pCurrentGameData->GetGameID();
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
