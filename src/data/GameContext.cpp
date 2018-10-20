#include "GameContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "RA_RichPresence.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"

namespace ra {
namespace data {

void GameContext::LoadGame(unsigned int nGameId, const std::wstring& sGameTitle)
{
    if (nGameId == 0)
    {
        m_nGameId = 0;
        m_sGameTitle.clear();
        m_sGameHash.clear();
        m_pRichPresenceInterpreter.reset();
        return;
    }

    m_nGameId = nGameId;
    m_sGameTitle = sGameTitle;
}

bool GameContext::HasRichPresence() const
{
    return (m_pRichPresenceInterpreter != nullptr && m_pRichPresenceInterpreter->Enabled());
}

std::wstring GameContext::GetRichPresenceDisplayString() const
{
    if (m_pRichPresenceInterpreter == nullptr)
        return std::wstring(L"No Rich Presence defined.");

    return ra::Widen(m_pRichPresenceInterpreter->GetRichPresenceString());
}

void GameContext::ReloadRichPresenceScript()
{
    m_pRichPresenceInterpreter.reset(new RA_RichPresenceInterpreter());
    if (!m_pRichPresenceInterpreter->Load())
        m_pRichPresenceInterpreter.reset();
}

} // namespace data
} // namespace ra
