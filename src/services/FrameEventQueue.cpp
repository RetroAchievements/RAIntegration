#include "FrameEventQueue.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace services {

void FrameEventQueue::QueuePauseOnChange(MemSize nSize, ra::ByteAddress nAddress)
{
    for (const auto& pChange : m_vMemChanges)
    {
        if (pChange.nAddress == nAddress && pChange.nSize == nSize)
            return;
    }

    m_vMemChanges.emplace_back(nAddress, nSize);
}

void FrameEventQueue::DoFrame()
{
    std::wstring sPauseMessage;

    if (!m_vTriggeredTriggers.empty())
    {
        sPauseMessage.append(L"The following triggers have fired:");
        for (const auto& pTriggerName : m_vTriggeredTriggers)
            sPauseMessage.append(ra::StringPrintf(L"\n* %s", pTriggerName));

        m_vTriggeredTriggers.clear();
    }

    if (!m_vResetTriggers.empty())
    {
        if (!sPauseMessage.empty())
            sPauseMessage.append(L"\n");

        sPauseMessage.append(L"The following triggers have been reset:");
        for (const auto& pTriggerName : m_vResetTriggers)
            sPauseMessage.append(ra::StringPrintf(L"\n* %s", pTriggerName));

        m_vResetTriggers.clear();
    }

    if (!m_vMemChanges.empty())
    {
        if (!sPauseMessage.empty())
            sPauseMessage.append(L"\n");

        sPauseMessage.append(L"The following bookmarks have changed:");

        const auto& pAssetEditor = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetEditor;
        for (const auto& pChange : m_vMemChanges)
        {
            sPauseMessage.append(ra::StringPrintf(L"\n* %s %s",
                pAssetEditor.Trigger().OperandSizes().GetLabelForId(ra::etoi(pChange.nSize)),
                ra::ByteAddressToString(pChange.nAddress)));
        }

        m_vMemChanges.clear();
    }

    if (!sPauseMessage.empty())
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
        pEmulatorContext.Pause();

        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"The emulator has been paused.", sPauseMessage);
    }
}

} // namespace services
} // namespace ra
