#include "FrameEventQueue.hh"

#include "RA_Defs.h"
#include "util\Strings.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace services {

void FrameEventQueue::DoFrame()
{
    std::wstring sPauseMessage;

    if (!m_vFunctions.empty())
    {
        for (auto& fFunction : m_vFunctions)
            fFunction();

        m_vFunctions.clear();
    }

    if (!m_vTriggeredTriggers.empty())
    {
        sPauseMessage.append(L"The following triggers have triggered:");
        for (const auto& pTriggerName : m_vTriggeredTriggers)
            sPauseMessage.append(ra::util::String::Printf(L"\n* %s", pTriggerName));

        m_vTriggeredTriggers.clear();
    }

    if (!m_vResetTriggers.empty())
    {
        if (!sPauseMessage.empty())
            sPauseMessage.append(L"\n");

        sPauseMessage.append(L"The following triggers have been reset:");
        for (const auto& pTriggerName : m_vResetTriggers)
            sPauseMessage.append(ra::util::String::Printf(L"\n* %s", pTriggerName));

        m_vResetTriggers.clear();
    }

    if (!m_vMemChanges.empty())
    {
        if (!sPauseMessage.empty())
            sPauseMessage.append(L"\n");

        sPauseMessage.append(L"The following bookmarks have changed:");
        for (const auto& pChange : m_vMemChanges)
            sPauseMessage.append(ra::util::String::Printf(L"\n* %s", pChange));

        m_vMemChanges.clear();
    }

    if (!sPauseMessage.empty())
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        pEmulatorContext.Pause();

        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"The emulator has been paused.", sPauseMessage);
    }
}

} // namespace services
} // namespace ra
