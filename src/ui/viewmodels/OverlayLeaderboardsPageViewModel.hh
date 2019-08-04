#ifndef RA_UI_OVERLAY_LEADERBOARDS_VIEWMODEL_H
#define RA_UI_OVERLAY_LEADERBOARDS_VIEWMODEL_H

#include "OverlayListPageViewModel.hh"

#include "data\Types.hh"

#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayLeaderboardsPageViewModel : public OverlayListPageViewModel
{
public:
    void Refresh() override;

protected:
    void FetchItemDetail(ItemViewModel& vmItem) override;
    std::map<ra::LeaderboardID, ViewModelCollection<ItemViewModel>> m_vLeaderboardRanks;

private:
    void RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, _UNUSED int nWidth, int nHeight) const override;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_LEADERBOARDS_VIEWMODEL_H
