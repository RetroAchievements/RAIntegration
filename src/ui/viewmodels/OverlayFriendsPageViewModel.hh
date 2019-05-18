#ifndef RA_UI_OVERLAY_FRIENDS_VIEWMODEL_H
#define RA_UI_OVERLAY_FRIENDS_VIEWMODEL_H

#include "OverlayListPageViewModel.hh"

#include "services\IClock.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayFriendsPageViewModel : public OverlayListPageViewModel
{
public:
    void Refresh() override;

private:
    std::chrono::time_point<std::chrono::steady_clock> m_tLastUpdated;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_FRIENDS_VIEWMODEL_H
