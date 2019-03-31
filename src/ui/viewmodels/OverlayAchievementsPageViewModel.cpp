#include "OverlayAchievementsPageViewModel.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::GameTitleProperty("OverlayAchievementsPageViewModel", "GameTitle", L"");
const StringModelProperty OverlayAchievementsPageViewModel::SummaryProperty("OverlayAchievementsPageViewModel", "Summary", L"");
const StringModelProperty OverlayAchievementsPageViewModel::PlayTimeProperty("OverlayAchievementsPageViewModel", "PlayTime", L"");
const IntModelProperty OverlayAchievementsPageViewModel::SelectedAchievementIndexProperty("OverlayAchievementsPageViewModel", "SelectedAchievementIndex", 0);

const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::TitleProperty("AchievementViewModel", "Title", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::DescriptionProperty("AchievementViewModel", "Description", L"");
const BoolModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::IsLockedProperty("AchievementViewModel", "IsLocked", false);

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

            pvmAchievement->SetTitle(ra::StringPrintf(L"%s (%s points)", pAchievement.Title(), pAchievement.Points()));
            pvmAchievement->SetDescription(ra::Widen(pAchievement.Description()));
            pvmAchievement->SetLocked(false);
            pvmAchievement->Image.ChangeReference(ra::ui::ImageType::Badge, pAchievement.BadgeImageURI());
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

    if (m_bImagesPending)
    {

    }

    if (m_fElapsed < 60.0)
        return false;

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
    gsl::index nAchievementIndex = 0;
    const auto nAchievementSize = 64;
    const auto nAchievementSpacing = 8;
    const unsigned int nVisibleAchievements = (nHeight + nAchievementSpacing) / (nAchievementSize + nAchievementSpacing);

    if (nSelectedAchievementIndex >= nVisibleAchievements)
        nAchievementIndex = nSelectedAchievementIndex - nVisibleAchievements + 1;

    if (nVisibleAchievements < m_vAchievements.Count())
    {
        RenderScrollBar(pSurface, nX, nY, nHeight, m_vAchievements.Count(), nVisibleAchievements, nAchievementIndex);
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

        bool bSelected = (nAchievementIndex == nSelectedAchievementIndex);
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

bool OverlayAchievementsPageViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (pInput.m_bDownPressed)
    {
        const auto nSelectedAchievementIndex = GetSelectedAchievementIndex();
        if (nSelectedAchievementIndex < ra::to_signed(m_vAchievements.Count()) - 1)
        {
            SetSelectedAchievementIndex(nSelectedAchievementIndex + 1);
            return true;
        }
    }
    else if (pInput.m_bUpPressed)
    {
        const auto nSelectedAchievementIndex = GetSelectedAchievementIndex();
        if (nSelectedAchievementIndex > 0)
        {
            SetSelectedAchievementIndex(nSelectedAchievementIndex - 1);
            return true;
        }
    }

    return false;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
