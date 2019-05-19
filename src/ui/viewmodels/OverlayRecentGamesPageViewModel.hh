#ifndef RA_UI_OVERLAY_RECENT_GAMES_VIEWMODEL_H
#define RA_UI_OVERLAY_RECENT_GAMES_VIEWMODEL_H

#include "OverlayListPageViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayRecentGamesPageViewModel : public OverlayListPageViewModel
{
public:
    void Refresh() override;

protected:
    void UpdateGameEntry(ItemViewModel& pvmItem, const std::wstring& sGameName, const std::string& sGameBadge,
                         std::chrono::seconds nPlaytimeSeconds);
    void UpdateGameEntry(unsigned int nGameId, const std::wstring& sGameName, const std::string& sGameBadge);

    std::map<unsigned int, std::wstring> m_mGameNames;
    std::map<unsigned int, std::string> m_mGameBadges;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_RECENT_GAMES_VIEWMODEL_H
