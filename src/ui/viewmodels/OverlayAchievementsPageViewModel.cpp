#include "OverlayAchievementsPageViewModel.hh"

#include "api\FetchAchievementInfo.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::GameTitleProperty("OverlayAchievementsPageViewModel", "GameTitle", L"");
const StringModelProperty OverlayAchievementsPageViewModel::SummaryProperty("OverlayAchievementsPageViewModel", "Summary", L"");
const StringModelProperty OverlayAchievementsPageViewModel::PlayTimeProperty("OverlayAchievementsPageViewModel", "PlayTime", L"");
const IntModelProperty OverlayAchievementsPageViewModel::SelectedAchievementIndexProperty("OverlayAchievementsPageViewModel", "SelectedAchievementIndex", 0);

const StringModelProperty OverlayAchievementsPageViewModel::WinnerViewModel::UsernameProperty("WinnerViewModel", "UserName", L"");
const StringModelProperty OverlayAchievementsPageViewModel::WinnerViewModel::UnlockDateProperty("WinnerViewModel", "UnlockDate", L"");
const BoolModelProperty OverlayAchievementsPageViewModel::WinnerViewModel::IsUserProperty("WinnerViewModel", "IsUser", false);

const IntModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::AchievementIdProperty("AchievementViewModel", "AchievementId", 0);
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::TitleProperty("AchievementViewModel", "Title", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::DescriptionProperty("AchievementViewModel", "Description", L"");
const BoolModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::IsLockedProperty("AchievementViewModel", "IsLocked", false);
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::CreatedDateProperty("AchievementViewModel", "CreatedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::ModifiedDateProperty("AchievementViewModel", "ModifiedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::WonByProperty("AchievementViewModel", "WonBy", L"");

void OverlayAchievementsPageViewModel::Refresh()
{
    SetTitle(L"Achievements");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    // title
    SetGameTitle(pGameContext.GameTitle());

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
            AchievementViewModel* pvmAchievement = m_vAchievements.GetItemAt(nNumberOfAchievements);
            if (pvmAchievement == nullptr)
                pvmAchievement = &m_vAchievements.Add();

            pvmAchievement->SetAchievementId(pAchievement.ID());
            pvmAchievement->SetTitle(ra::StringPrintf(L"%s (%s points)", pAchievement.Title(), pAchievement.Points()));
            pvmAchievement->SetDescription(ra::Widen(pAchievement.Description()));
            pvmAchievement->SetLocked(false);
            pvmAchievement->Image.ChangeReference(ra::ui::ImageType::Badge, pAchievement.BadgeImageURI());
            pvmAchievement->SetCreatedDate(ra::Widen(ra::FormatDate(pAchievement.CreatedDate())));
            pvmAchievement->SetModifiedDate(ra::Widen(ra::FormatDate(pAchievement.ModifiedDate())));
            ++nNumberOfAchievements;

            if (pActiveAchievementType == AchievementSet::Type::Core)
            {
                nMaxPts += pAchievement.Points();
                if (pAchievement.Active())
                {
                    pvmAchievement->Image.ChangeReference(ra::ui::ImageType::Badge, pAchievement.BadgeImageURI() + "_lock");
                    pvmAchievement->SetLocked(true);
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

    while (m_vAchievements.Count() > nNumberOfAchievements)
        m_vAchievements.RemoveAt(m_vAchievements.Count() - 1);

    // summary
    if (nNumberOfAchievements == 0)
        SetSummary(L"No achievements present");
    else if (pActiveAchievementType == AchievementSet::Type::Core)
        SetSummary(ra::StringPrintf(L"%u of %u won (%u/%u)", nUserCompleted, nNumberOfAchievements, nUserPts, nMaxPts));
    else
        SetSummary(ra::StringPrintf(L"%u achievements present", nNumberOfAchievements));

    // playtime
    if (pGameContext.GameId() == 0)
    {
        SetPlayTime(L"");
    }
    else
    {
        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
        SetPlayTime(ra::StringPrintf(L"%dh%02dm", nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
        m_fElapsed = static_cast<double>(nPlayTimeSeconds.count() % 60);
    }
}

bool OverlayAchievementsPageViewModel::Update(double fElapsed)
{
    m_fElapsed += fElapsed;

    bool bUpdated = m_bRedraw;
    m_bRedraw = false;

    if (m_bImagesPending)
    {

    }

    if (m_fElapsed < 60.0)
        return bUpdated;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
    const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
    SetPlayTime(ra::StringPrintf(L"%dh%02dm", nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));

    do
    {
        m_fElapsed -= 60.0;
    } while (m_fElapsed > 60.0);

    return true;
}

void OverlayAchievementsPageViewModel::Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    if (m_bAchievementDetail)
        RenderDetail(pSurface, nX, nY, nWidth, nHeight);
    else
        RenderList(pSurface, nX, nY, nWidth, nHeight);
}

void OverlayAchievementsPageViewModel::RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();

    // title
    const auto& sGameTitle = GetGameTitle();
    if (!sGameTitle.empty())
    {
        const auto nTitleFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
        pSurface.WriteText(nX, nY, nTitleFont, pTheme.ColorOverlayText(), sGameTitle);
        nY += 34;
        nHeight -= 34;
    }

    // subtitle
    const auto nFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
    const auto& sPlayTime = GetPlayTime();
    const std::wstring sSummary = sPlayTime.empty() ? GetSummary() : ra::StringPrintf(L"%s - %s", GetSummary(), sPlayTime);
    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sSummary);
    nY += 30;
    nHeight -= 30;

    // scrollbar
    const auto nSelectedAchievementIndex = ra::to_unsigned(GetSelectedAchievementIndex());
    gsl::index nAchievementIndex = m_nScrollOffset;
    const auto nAchievementSize = 64;
    const auto nAchievementSpacing = 8;
    m_nVisibleAchievements = (nHeight + nAchievementSpacing) / (nAchievementSize + nAchievementSpacing);

    if (m_nVisibleAchievements < m_vAchievements.Count())
    {
        RenderScrollBar(pSurface, nX, nY, nHeight, m_vAchievements.Count(), m_nVisibleAchievements, nAchievementIndex);
        nX += 20;
    }

    // achievements list
    while (nHeight > nAchievementSize + nAchievementSpacing)
    {
        const auto* pAchievement = m_vAchievements.GetItemAt(nAchievementIndex);
        if (!pAchievement)
            break;

        pSurface.DrawImage(nX, nY, nAchievementSize, nAchievementSize, pAchievement->Image);

        ra::ui::Color nTextColor = pTheme.ColorOverlayText();

        bool bSelected = (nAchievementIndex == ra::to_signed(nSelectedAchievementIndex));
        if (bSelected)
        {
            pSurface.FillRectangle(nX + nAchievementSize, nY, nWidth - nX - nAchievementSize, nAchievementSize, pTheme.ColorOverlaySelectionBackground());
            nTextColor = pAchievement->IsLocked() ?
                pTheme.ColorOverlaySelectionDisabledText() : pTheme.ColorOverlaySelectionText();
        }
        else if (pAchievement->IsLocked())
        {
            nTextColor = pTheme.ColorOverlayDisabledText();
        }

        pSurface.WriteText(nX + nAchievementSize + 12, nY + 4, nFont, nTextColor, pAchievement->GetTitle());
        pSurface.WriteText(nX + nAchievementSize + 12, nY + 4 + 26, nSubFont, nTextColor, pAchievement->GetDescription());

        nAchievementIndex++;
        nY += nAchievementSize + nAchievementSpacing;
        nHeight -= nAchievementSize + nAchievementSpacing;
    }
}

void OverlayAchievementsPageViewModel::RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    const auto* pAchievement = m_vAchievements.GetItemAt(GetSelectedAchievementIndex());
    if (pAchievement == nullptr)
        return;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
    const auto nAchievementSize = 64;

    pSurface.DrawImage(nX, nY, nAchievementSize, nAchievementSize, pAchievement->Image);

    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4, nFont, pTheme.ColorOverlayText(), pAchievement->GetTitle());
    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4 + 26, nSubFont, pTheme.ColorOverlaySubText(), pAchievement->GetDescription());
    nY += nAchievementSize + 8;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), L"Created: " + pAchievement->GetCreatedDate());
    pSurface.WriteText(nX, nY + 26, nSubFont, pTheme.ColorOverlaySubText(), L"Modified: " + pAchievement->GetModifiedDate());
    nY += 60;

    const auto sWonBy = pAchievement->GetWonBy();
    if (sWonBy.empty())
        return;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sWonBy);
    pSurface.WriteText(nX, nY + 30, nFont, pTheme.ColorOverlayText(), L"Recent Winners:");
    nY += 60;

    gsl::index nIndex = 0;
    while (nY + 20 < nHeight)
    {
        const auto* pWinner = pAchievement->RecentWinners.GetItemAt(nIndex);
        if (pWinner == nullptr)
            break;

        const auto nColor = pWinner->IsUser() ? pTheme.ColorOverlaySelectionText() : pTheme.ColorOverlaySubText();
        pSurface.WriteText(nX, nY, nSubFont, nColor, pWinner->GetUsername());
        pSurface.WriteText(nX + 200, nY, nSubFont, nColor, pWinner->GetUnlockDate());

        ++nIndex;
        nY += 20;
    }
}

void OverlayAchievementsPageViewModel::FetchAchievementDetail(AchievementViewModel& vmAchievement)
{
    if (!vmAchievement.GetWonBy().empty()) // already populated
        return;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pAchievement = pGameContext.FindAchievement(vmAchievement.GetAchievementId());
    if (pAchievement == nullptr)
        return;

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
        auto* vmAchievement = m_vAchievements.GetItemAt(GetSelectedAchievementIndex());
        if (vmAchievement == nullptr || static_cast<ra::AchievementID>(vmAchievement->GetAchievementId()) != nId)
            return;

        if (!response.Succeeded())
        {
            vmAchievement->SetWonBy(ra::Widen(response.ErrorMessage));
            return;
        }

        vmAchievement->SetWonBy(ra::StringPrintf(L"Won by %u of %u (%1.0f%%)", response.EarnedBy, response.NumPlayers,
            static_cast<double>(response.EarnedBy * 100) / response.NumPlayers));

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
        for (const auto& pWinner : response.Entries)
        {
            auto& vmWinner = vmAchievement->RecentWinners.Add();
            vmWinner.SetUsername(ra::Widen(pWinner.User));
            vmWinner.SetUnlockDate(ra::StringPrintf(L"%s (%s)", ra::FormatDate(pWinner.DateAwarded), ra::FormatDateRecent(pWinner.DateAwarded)));

            if (pWinner.User == sUsername)
                vmWinner.SetUser(true);
        }

        m_bRedraw = true;
    });
}

bool OverlayAchievementsPageViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (pInput.m_bDownPressed)
    {
        const auto nSelectedAchievementIndex = GetSelectedAchievementIndex();
        if (nSelectedAchievementIndex < ra::to_signed(m_vAchievements.Count()) - 1)
        {
            SetSelectedAchievementIndex(nSelectedAchievementIndex + 1);

            if (nSelectedAchievementIndex + 1 - m_nScrollOffset == ra::to_signed(m_nVisibleAchievements))
                ++m_nScrollOffset;

            if (m_bAchievementDetail)
            {
                auto* vmAchievement = m_vAchievements.GetItemAt(GetSelectedAchievementIndex());
                if (vmAchievement != nullptr)
                    FetchAchievementDetail(*vmAchievement);
            }

            return true;
        }
    }
    else if (pInput.m_bUpPressed)
    {
        const auto nSelectedAchievementIndex = GetSelectedAchievementIndex();
        if (nSelectedAchievementIndex > 0)
        {
            SetSelectedAchievementIndex(nSelectedAchievementIndex - 1);

            if (nSelectedAchievementIndex == m_nScrollOffset)
                --m_nScrollOffset;

            if (m_bAchievementDetail)
            {
                auto* vmAchievement = m_vAchievements.GetItemAt(GetSelectedAchievementIndex());
                if (vmAchievement != nullptr)
                    FetchAchievementDetail(*vmAchievement);
            }

            return true;
        }
    }
    else if (pInput.m_bConfirmPressed)
    {
        if (!m_bAchievementDetail)
        {
            m_bAchievementDetail = true;
            SetTitle(L"Achievement Info");
            auto* vmAchievement = m_vAchievements.GetItemAt(GetSelectedAchievementIndex());
            if (vmAchievement != nullptr)
                FetchAchievementDetail(*vmAchievement);
            return true;
        }
    }
    else if (pInput.m_bCancelPressed)
    {
        if (m_bAchievementDetail)
        {
            m_bAchievementDetail = false;
            SetTitle(L"Achievements");
            return true;
        }
    }

    return false;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
