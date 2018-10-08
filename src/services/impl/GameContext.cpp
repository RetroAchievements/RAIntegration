#include "GameContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"

namespace ra {
namespace services {
namespace impl {

void GameContext::LoadGame(unsigned int nGameId, const std::wstring& sGameTitle)
{
    if (nGameId == 0)
    {
        m_nGameId = 0;
        m_sGameTitle.clear();
        m_sGameHash.clear();
        return;
    }

    m_nGameId = nGameId;
    m_sGameTitle = sGameTitle;
}

} // namespace impl
} // namespace services
} // namespace ra
