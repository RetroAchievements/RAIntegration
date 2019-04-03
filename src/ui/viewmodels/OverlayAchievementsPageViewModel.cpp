#include "OverlayAchievementsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchAchievementInfo.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::UnlockedAchievementsProperty("OverlayAchievementsPageViewModel", "UnlockedAchievements", L"");

const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::CreatedDateProperty("AchievementViewModel", "CreatedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::ModifiedDateProperty("AchievementViewModel", "ModifiedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::WonByProperty("AchievementViewModel", "WonBy", L"");

void OverlayAchievementsPageViewModel::Refresh()
{
    m_sTitle = L"Achievements";
    m_sDetailTitle = L"Achievement Info";
    OverlayListPageViewModel::Refresh();

    // title
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    SetListTitle(pGameContext.GameTitle());

    // achievement list
    const auto pActiveAchievementType = pGameContext.ActiveAchievementType();
    unsigned int nMaxPts = 0;
    unsigned int nUserPts = 0;
    unsigned int nUserCompleted = 0;
    size_t nNumberOfAchievements = 0;

    pGameContext.EnumerateAchievements([this, pActiveAchievementType, &nNumberOfAchievements,
        &nMaxPts, &nUserPts, &nUserCompleted](const Achievement& pAchievement)
    {
        if (pAchievement.Category() == ra::etoi(pActiveAchievementType))
        {
            ItemViewModel* pvmAchievement = m_vItems.GetItemAt(nNumberOfAchievements);
            if (pvmAchievement == nullptr)
            {
                pvmAchievement = &m_vItems.Add();
                Ensures(pvmAchievement != nullptr);
            }

            pvmAchievement->SetId(pAchievement.ID());
            pvmAchievement->SetLabel(ra::StringPrintf(L"%s (%s points)", pAchievement.Title(), pAchievement.Points()));
            pvmAchievement->SetDetail(ra::Widen(pAchievement.Description()));
            pvmAchievement->SetDisabled(false);
            pvmAchievement->Image.ChangeReference(ra::ui::ImageType::Badge, pAchievement.BadgeImageURI());
            ++nNumberOfAchievements;

            if (pActiveAchievementType == AchievementSet::Type::Core)
            {
                nMaxPts += pAchievement.Points();
                if (pAchievement.Active())
                {
                    pvmAchievement->Image.ChangeReference(ra::ui::ImageType::Badge, pAchievement.BadgeImageURI() + "_lock");
                    pvmAchievement->SetDisabled(true);
                }
                else
                {
                    nUserPts += pAchievement.Points();
                    ++nUserCompleted;
                }
            }
        }

        return true;
    });

    while (m_vItems.Count() > nNumberOfAchievements)
        m_vItems.RemoveAt(m_vItems.Count() - 1);

    // summary
    if (nNumberOfAchievements == 0)
        m_sSummary = L"No achievements present";
    else if (pActiveAchievementType == AchievementSet::Type::Core)
        m_sSummary = ra::StringPrintf(L"%u of %u won (%u/%u)", nUserCompleted, nNumberOfAchievements, nUserPts, nMaxPts);
    else
        m_sSummary = ra::StringPrintf(L"%u achievements present", nNumberOfAchievements);

    // playtime
    if (pGameContext.GameId() == 0)
    {
        SetSummary(m_sSummary);
    }
    else
    {
        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
        SetSummary(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
        m_fElapsed = static_cast<double>(nPlayTimeSeconds.count() % 60);
    }
}

bool OverlayAchievementsPageViewModel::Update(double fElapsed)
{
    const bool bUpdated = OverlayListPageViewModel::Update(fElapsed);

    if (m_fElapsed < 60.0)
        return bUpdated;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
    const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
    SetSummary(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));

    do
    {
        m_fElapsed -= 60.0;
    } while (m_fElapsed > 60.0);

    return true;
}

void OverlayAchievementsPageViewModel::RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, _UNUSED int nWidth, int nHeight) const
{
    const auto* pAchievement = m_vItems.GetItemAt(GetSelectedItemIndex());
    if (pAchievement == nullptr)
        return;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
    const auto nAchievementSize = 64;

    pSurface.DrawImage(nX, nY, nAchievementSize, nAchievementSize, pAchievement->Image);

    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4, nFont, pTheme.ColorOverlayText(), pAchievement->GetLabel());
    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4 + 26, nSubFont, pTheme.ColorOverlaySubText(), pAchievement->GetDetail());
    nY += nAchievementSize + 8;

    const auto pDetail = m_vAchievementDetails.find(pAchievement->GetId());
    if (pDetail == m_vAchievementDetails.end())
        return;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), L"Created: " + pDetail->second.GetCreatedDate());
    pSurface.WriteText(nX, nY + 26, nSubFont, pTheme.ColorOverlaySubText(), L"Modified: " + pDetail->second.GetModifiedDate());
    nY += 60;

    const auto sWonBy = pDetail->second.GetWonBy();
    if (sWonBy.empty())
        return;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sWonBy);
    pSurface.WriteText(nX, nY + 30, nFont, pTheme.ColorOverlayText(), L"Recent Winners:");
    nY += 60;

    gsl::index nIndex = 0;
    while (nY + 20 < nHeight)
    {
        const auto* pWinner = pDetail->second.RecentWinners.GetItemAt(nIndex);
        if (pWinner == nullptr)
            break;

        const auto nColor = pWinner->IsDisabled() ? pTheme.ColorOverlaySelectionText() : pTheme.ColorOverlaySubText();
        pSurface.WriteText(nX, nY, nSubFont, nColor, pWinner->GetLabel());
        pSurface.WriteText(nX + 200, nY, nSubFont, nColor, pWinner->GetDetail());

        ++nIndex;
        nY += 20;
    }
}

void OverlayAchievementsPageViewModel::FetchItemDetail(ItemViewModel& vmItem)
{
    if (m_vAchievementDetails.find(vmItem.GetId()) != m_vAchievementDetails.end()) // already populated
        return;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pAchievement = pGameContext.FindAchievement(vmItem.GetId());
    if (pAchievement == nullptr)
        return;

    auto& vmAchievement = m_vAchievementDetails.emplace(vmItem.GetId(), AchievementViewModel{}).first->second;
    vmAchievement.SetCreatedDate(ra::Widen(ra::FormatDate(pAchievement->CreatedDate())));
    vmAchievement.SetModifiedDate(ra::Widen(ra::FormatDate(pAchievement->ModifiedDate())));

    if (pAchievement->Category() == ra::etoi(AchievementSet::Type::Local))
    {
        vmAchievement.SetWonBy(L"Local Achievement");
        return;
    }

    ra::api::FetchAchievementInfo::Request request;
    request.AchievementId = pAchievement->ID();
    request.FirstEntry = 1;
    request.NumEntries = 10;
    request.CallAsync([this, nId = pAchievement->ID()](const ra::api::FetchAchievementInfo::Response& response)
    {
        auto pIter = m_vAchievementDetails.find(nId);
        if (pIter == m_vAchievementDetails.end())
            return;

        auto& vmAchievement = pIter->second;
        if (!response.Succeeded())
        {
            vmAchievement.SetWonBy(ra::Widen(response.ErrorMessage));
            return;
        }

        vmAchievement.SetWonBy(ra::StringPrintf(L"Won by %u of %u (%1.0f%%)", response.EarnedBy, response.NumPlayers,
            (static_cast<double>(response.EarnedBy) * 100) / response.NumPlayers));

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
        for (const auto& pWinner : response.Entries)
        {
            auto& vmWinner = vmAchievement.RecentWinners.Add();
            vmWinner.SetLabel(ra::Widen(pWinner.User));
            vmWinner.SetDetail(ra::StringPrintf(L"%s (%s)", ra::FormatDate(pWinner.DateAwarded), ra::FormatDateRecent(pWinner.DateAwarded)));

            if (pWinner.User == sUsername)
                vmWinner.SetDisabled(true);
        }

        ForceRedraw();
    });
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
