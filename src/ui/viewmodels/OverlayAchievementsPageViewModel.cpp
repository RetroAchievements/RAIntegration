#include "OverlayAchievementsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchAchievementInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IClock.hh"

#include "ui\OverlayTheme.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::UnlockedAchievementsProperty("OverlayAchievementsPageViewModel", "UnlockedAchievements", L"");

const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::CreatedDateProperty("AchievementViewModel", "CreatedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::ModifiedDateProperty("AchievementViewModel", "ModifiedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::WonByProperty("AchievementViewModel", "WonBy", L"");

OverlayListPageViewModel::ItemViewModel& OverlayAchievementsPageViewModel::GetNextItem(size_t* nIndex)
{
    Expects(nIndex != nullptr);

    ItemViewModel* pvmAchievement = m_vItems.GetItemAt((*nIndex)++);
    if (pvmAchievement == nullptr)
    {
        pvmAchievement = &m_vItems.Add();
        Ensures(pvmAchievement != nullptr);
    }

    return *pvmAchievement;
}

static void SetHeader(OverlayListPageViewModel::ItemViewModel& vmItem, const std::wstring& sHeader)
{
    vmItem.SetId(0);
    vmItem.SetLabel(sHeader);
    vmItem.SetDetail(L"");
    vmItem.SetDisabled(false);
    vmItem.Image.ChangeReference(ra::ui::ImageType::None, "");
    vmItem.SetProgressValue(0U);
    vmItem.SetProgressMaximum(0U);
}

static void SetAchievement(OverlayListPageViewModel::ItemViewModel& vmItem, const ra::data::models::AchievementModel& vmAchievement)
{
    vmItem.SetId(vmAchievement.GetID());
    vmItem.SetLabel(ra::StringPrintf(L"%s (%s %s)", vmAchievement.GetName(),
        vmAchievement.GetPoints(), (vmAchievement.GetPoints() == 1) ? "point" : "points"));
    vmItem.SetDetail(vmAchievement.GetDescription());

    if (vmAchievement.IsActive())
    {
        if (vmAchievement.GetCategory() == ra::data::models::AssetCategory::Local)
        {
            // local achievements never appear disabled
            vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, ra::Narrow(vmAchievement.GetBadge()));
            vmItem.SetDisabled(false);
        }
        else
        {
            vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, ra::Narrow(vmAchievement.GetBadge()) + "_lock");
            vmItem.SetDisabled(true);
        }

        const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        const auto* pTrigger = pRuntime.GetAchievementTrigger(vmAchievement.GetID());
        if (pTrigger != nullptr)
        {
            vmItem.SetProgressMaximum(pTrigger->measured_target);
            vmItem.SetProgressValue(pTrigger->measured_value);
        }
        else
        {
            vmItem.SetProgressMaximum(0);
            vmItem.SetProgressValue(0);
        }
    }
    else if (vmAchievement.GetState() == ra::data::models::AssetState::Disabled)
    {
        vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, ra::Narrow(vmAchievement.GetBadge()) + "_lock");
        vmItem.SetDisabled(true);
        vmItem.SetProgressValue(0U);
        vmItem.SetProgressMaximum(0U);
    }
    else
    {
        vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, ra::Narrow(vmAchievement.GetBadge()));
        vmItem.SetDisabled(false);
        vmItem.SetProgressValue(0U);
        vmItem.SetProgressMaximum(0U);
    }
}

static bool AppearsInFilter(const ra::data::models::AchievementModel* vmAchievement)
{
    Expects(vmAchievement != nullptr);

    const auto nId = ra::to_signed(vmAchievement->GetID());
    const auto& vFilteredAssets = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetList.FilteredAssets();
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pAsset = vFilteredAssets.GetItemAt(nIndex);
        if (pAsset && pAsset->GetId() == nId && pAsset->GetType() == ra::data::models::AssetType::Achievement)
            return true;
    }

    return false;
}

void OverlayAchievementsPageViewModel::Refresh()
{
    m_sTitle = L"Achievements";
    m_sDetailTitle = L"Achievement Info";
    OverlayListPageViewModel::Refresh();

    // title
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    SetListTitle(pGameContext.GameTitle());

    // achievement list
    std::vector<const ra::data::models::AchievementModel*> vRecentAchievements;
    std::vector<const ra::data::models::AchievementModel*> vActiveChallengeAchievements;
    std::vector<const ra::data::models::AchievementModel*> vAlmostThereAchievements;
    std::vector<const ra::data::models::AchievementModel*> vUnofficialAchievements;
    std::vector<const ra::data::models::AchievementModel*> vLocalAchievements;
    std::vector<const ra::data::models::AchievementModel*> vLockedCoreAchievements;
    std::vector<const ra::data::models::AchievementModel*> vUnlockedCoreAchievements;
    std::vector<const ra::data::models::AchievementModel*> vUnsupportedCoreAchievements;

    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    const auto tNow = ra::services::ServiceLocator::Get<ra::services::IClock>().Now();

    for (gsl::index nIndex = 0; nIndex < ra::to_signed(pGameContext.Assets().Count()); ++nIndex)
    {
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (!pAsset || pAsset->GetType() != ra::data::models::AssetType::Achievement)
            continue;

        const auto* vmAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
        switch (vmAchievement->GetState())
        {
            case ra::data::models::AssetState::Inactive:
                break;

            case ra::data::models::AssetState::Triggered:
            {
                const auto tUnlock = vmAchievement->GetUnlockTime();
                const auto tElapsed = tNow - tUnlock;
                if (tElapsed < std::chrono::minutes(10))
                {
                    gsl::index nInsert = 0;
                    while (nInsert < ra::to_signed(vRecentAchievements.size()) && vRecentAchievements.at(nInsert)->GetUnlockTime() > tUnlock)
                        nInsert++;
                    vRecentAchievements.insert(vRecentAchievements.begin() + nInsert, vmAchievement);
                    continue;
                }
                break;
            }

            case ra::data::models::AssetState::Primed:
            {
                vActiveChallengeAchievements.push_back(vmAchievement);
                continue;
            }

            default:
            {
                const auto* pTrigger = pRuntime.GetAchievementTrigger(vmAchievement->GetID());
                if (pTrigger != nullptr && pTrigger->measured_target > 0)
                {
                    const auto nProgress = pTrigger->measured_value * 100 / pTrigger->measured_target;
                    if (nProgress >= 80)
                    {
                        vAlmostThereAchievements.push_back(vmAchievement);
                        continue;
                    }
                }
                break;
            }
        }

        switch (vmAchievement->GetCategory())
        {
            case ra::data::models::AssetCategory::Local:
                if (AppearsInFilter(vmAchievement))
                    vLocalAchievements.push_back(vmAchievement);
                break;

            case ra::data::models::AssetCategory::Unofficial:
                if (AppearsInFilter(vmAchievement))
                    vUnofficialAchievements.push_back(vmAchievement);
                break;

            default:
                if (vmAchievement->IsActive())
                    vLockedCoreAchievements.push_back(vmAchievement);
                else if (vmAchievement->GetState() == ra::data::models::AssetState::Disabled)
                    vUnsupportedCoreAchievements.push_back(vmAchievement);
                else
                    vUnlockedCoreAchievements.push_back(vmAchievement);
                break;
        }
    }

    unsigned int nMaxPts = 0;
    unsigned int nUserPts = 0;
    unsigned int nUserCompleted = 0;
    size_t nIndex = 0;
    size_t nNumberOfAchievements = 0;
    size_t nNumberOfCoreAchievements = 0;

    m_vItems.BeginUpdate();

    if (!vActiveChallengeAchievements.empty())
    {
        auto& pvmHeader = GetNextItem(&nIndex);
        SetHeader(pvmHeader, L"Active Challenges");

        for (const auto* vmAchievement : vActiveChallengeAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);

            const auto nPoints = vmAchievement->GetPoints();
            nMaxPts += nPoints;

            if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
                ++nNumberOfCoreAchievements;
        }

        nNumberOfAchievements += vRecentAchievements.size();
    }

    if (!vRecentAchievements.empty())
    {
        auto& pvmHeader = GetNextItem(&nIndex);
        SetHeader(pvmHeader, L"Recently Unlocked");

        for (const auto* vmAchievement : vRecentAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);

            const auto nPoints = vmAchievement->GetPoints();
            nMaxPts += nPoints;

            if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
            {
                nUserPts += nPoints;
                ++nUserCompleted;
                ++nNumberOfCoreAchievements;
            }
        }

        nNumberOfAchievements += vRecentAchievements.size();
    }

    if (!vAlmostThereAchievements.empty())
    {
        auto& pvmHeader = GetNextItem(&nIndex);
        SetHeader(pvmHeader, L"Almost There");

        for (const auto* vmAchievement : vAlmostThereAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);
            nMaxPts += vmAchievement->GetPoints();

            if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Core)
                ++nNumberOfCoreAchievements;
        }

        nNumberOfAchievements += vAlmostThereAchievements.size();
    }

    if (!vLocalAchievements.empty())
    {
        if (nIndex > 0 || !vUnofficialAchievements.empty() || !vLockedCoreAchievements.empty() || !vUnlockedCoreAchievements.empty())
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Local");
        }

        for (const auto* vmAchievement : vLocalAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);
            nMaxPts += vmAchievement->GetPoints();
        }

        nNumberOfAchievements += vLocalAchievements.size();
    }

    if (!vUnofficialAchievements.empty())
    {
        if (nIndex > 0 || !vLockedCoreAchievements.empty() || !vUnlockedCoreAchievements.empty())
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Unofficial");
        }

        for (const auto* vmAchievement : vUnofficialAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);
            nMaxPts += vmAchievement->GetPoints();
        }

        nNumberOfAchievements += vUnofficialAchievements.size();
    }

    if (!vLockedCoreAchievements.empty())
    {
        if (nIndex > 0)
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Locked");
        }

        for (const auto* vmAchievement : vLockedCoreAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);
            nMaxPts += vmAchievement->GetPoints();
        }

        nNumberOfAchievements += vLockedCoreAchievements.size();
        nNumberOfCoreAchievements += vLockedCoreAchievements.size();
    }

    if (!vUnsupportedCoreAchievements.empty())
    {
        auto& pvmHeader = GetNextItem(&nIndex);
        SetHeader(pvmHeader, L"Unsupported");

        for (const auto* vmAchievement : vUnsupportedCoreAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);
            nMaxPts += vmAchievement->GetPoints();
        }

        nNumberOfAchievements += vUnsupportedCoreAchievements.size();
        nNumberOfCoreAchievements += vUnsupportedCoreAchievements.size();
    }

    if (vUnlockedCoreAchievements.size() > 0)
    {
        if (nIndex > 0)
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Unlocked");
        }

        for (const auto* vmAchievement : vUnlockedCoreAchievements)
        {
            auto& pvmAchievement = GetNextItem(&nIndex);
            SetAchievement(pvmAchievement, *vmAchievement);

            const auto nPoints = vmAchievement->GetPoints();
            nMaxPts += nPoints;

            nUserPts += nPoints;
            ++nUserCompleted;
        }

        nNumberOfAchievements += vUnlockedCoreAchievements.size();
        nNumberOfCoreAchievements += vUnlockedCoreAchievements.size();
    }

    while (m_vItems.Count() > nIndex)
        m_vItems.RemoveAt(m_vItems.Count() - 1);

    m_vItems.EndUpdate();

    // summary
    if (nNumberOfAchievements == 0)
        m_sSummary = L"No achievements present";
    else if (nNumberOfCoreAchievements > 0)
        m_sSummary = ra::StringPrintf(L"%u of %u won (%u/%u)", nUserCompleted, nNumberOfCoreAchievements, nUserPts, nMaxPts);
    else
        m_sSummary = ra::StringPrintf(L"%u achievements present", nNumberOfAchievements);

    // playtime
    if (pGameContext.GameId() == 0)
    {
        SetSummary(m_sSummary);
    }
    else
    {
        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
        SetSummary(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
        m_fElapsed = static_cast<double>(nPlayTimeSeconds.count() % 60);
    }

    EnsureSelectedItemIndexValid();
}

bool OverlayAchievementsPageViewModel::Update(double fElapsed)
{
    const bool bUpdated = OverlayListPageViewModel::Update(fElapsed);

    if (m_fElapsed < 60.0)
        return bUpdated;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
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
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
    constexpr auto nAchievementSize = 64;

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

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pAchievement = pGameContext.Assets().FindAchievement(vmItem.GetId());
    if (pAchievement == nullptr)
        return;

    auto& vmAchievement = m_vAchievementDetails.emplace(vmItem.GetId(), AchievementViewModel{}).first->second;
    vmAchievement.SetCreatedDate(ra::Widen(ra::FormatDateTime(pAchievement->GetCreationTime())));
    vmAchievement.SetModifiedDate(ra::Widen(ra::FormatDateTime(pAchievement->GetUpdatedTime())));

    if (pAchievement->GetCategory() == ra::data::models::AssetCategory::Local)
    {
        vmAchievement.SetWonBy(L"Local Achievement");
        return;
    }

    ra::api::FetchAchievementInfo::Request request;
    request.AchievementId = pAchievement->GetID();
    request.FirstEntry = 1;
    request.NumEntries = 10;
    request.CallAsync([this, nId = pAchievement->GetID()](const ra::api::FetchAchievementInfo::Response& response)
    {
        const auto pIter = m_vAchievementDetails.find(nId);
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

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
        for (const auto& pWinner : response.Entries)
        {
            auto& vmWinner = vmAchievement.RecentWinners.Add();
            vmWinner.SetLabel(ra::Widen(pWinner.User));
            vmWinner.SetDetail(ra::StringPrintf(L"%s (%s)", ra::FormatDateTime(pWinner.DateAwarded), ra::FormatDateRecent(pWinner.DateAwarded)));

            if (pWinner.User == sUsername)
                vmWinner.SetDisabled(true);
        }

        ForceRedraw();
    });
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
