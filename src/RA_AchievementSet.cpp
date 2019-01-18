#include "RA_AchievementSet.h"

#include "RA_Core.h"
#include "RA_Dlg_Achievement.h" // RA_httpthread.h
#include "RA_Dlg_AchEditor.h" // RA_httpthread.h
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_RichPresence.h"
#include "RA_md5factory.h"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

AchievementSet* g_pCoreAchievements = nullptr;
AchievementSet* g_pUnofficialAchievements = nullptr;
AchievementSet* g_pLocalAchievements = nullptr;

AchievementSet::Type g_nActiveAchievementSet = AchievementSet::Type::Core;
AchievementSet* g_pActiveAchievements = g_pCoreAchievements;

_Use_decl_annotations_
void RASetAchievementCollection(AchievementSet::Type Type)
{
    if (g_nActiveAchievementSet == Type)
        return;

    auto& pAchievementRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    for (size_t i = 0U; i < g_pActiveAchievements->NumAchievements(); ++i)
    {
        const auto& pAchievement = g_pActiveAchievements->GetAchievement(i);
        if (pAchievement.Active())
            pAchievementRuntime.DeactivateAchievement(pAchievement.ID());
    }

    g_nActiveAchievementSet = Type;
    switch (Type)
    {
        case AchievementSet::Type::Core:
            g_pActiveAchievements = g_pCoreAchievements;
            break;
        default:
        case AchievementSet::Type::Unofficial:
            g_pActiveAchievements = g_pUnofficialAchievements;
            break;
        case AchievementSet::Type::Local:
            g_pActiveAchievements = g_pLocalAchievements;
            break;
    }

    for (size_t i = 0U; i < g_pActiveAchievements->NumAchievements(); ++i)
    {
        auto& pAchievement = g_pActiveAchievements->GetAchievement(i);
        if (pAchievement.Active())
        {
            // deactivate and reactivate to re-register the achievement with the runtime
            pAchievement.SetActive(false);
            pAchievement.SetActive(true);
        }
    }
}

bool AchievementSet::RemoveAchievement(const Achievement* pAchievement)
{
    auto pIter = m_Achievements.begin();
    while (pIter != m_Achievements.end())
    {
        if (*pIter == pAchievement)
        {
            m_Achievements.erase(pIter);
            return true;
        }

        ++pIter;
    }

    return false;
}

Achievement* AchievementSet::Find(unsigned int nAchievementID) const noexcept
{
    for (auto pAchievement : m_Achievements)
        if (pAchievement && pAchievement->ID() == nAchievementID)
            return pAchievement;

    return nullptr;
}

size_t AchievementSet::GetAchievementIndex(const Achievement& Ach)
{
    for (size_t nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
    {
        if (&Ach == &GetAchievement(nOffset))
            return nOffset;
    }

    //	Not found
    return ra::to_unsigned(-1);
}

void AchievementSet::Clear() noexcept
{
    m_Achievements.clear();
}

void AchievementSet::Test()
{
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    if (pRuntime.IsPaused())
        return;

    for (auto pAchievement : m_Achievements)
    {
        if (pAchievement && pAchievement->Active())
            pAchievement->SetDirtyFlag(Achievement::DirtyFlags::Conditions);
    }

    std::vector<ra::services::AchievementRuntime::Change> vChanges;
    pRuntime.Process(vChanges);

    for (const auto& pChange : vChanges)
    {
        switch (pChange.nType)
        {
            case ra::services::AchievementRuntime::ChangeType::AchievementReset:
            {
                // we only watch for AchievementReset if PauseOnReset is set, so handle that now.
#ifndef RA_UTEST
                RA_CausePause();
#endif
                std::wstring sMessage = ra::StringPrintf(L"Pause on Reset: %s", Find(pChange.nId)->Title());
                ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                break;
            }

            case ra::services::AchievementRuntime::ChangeType::AchievementTriggered:
            {
                const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                pGameContext.AwardAchievement(pChange.nId);
                auto* pAchievement = pGameContext.FindAchievement(pChange.nId);
                if (!pAchievement)
                    break;

#ifndef RA_UTEST
                //	Reverse find where I am in the list:
                unsigned int nOffset = 0;
                for (nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset)
                {
                    if (pAchievement == &g_pActiveAchievements->GetAchievement(nOffset))
                        break;
                }

                ASSERT(nOffset < NumAchievements());
                if (nOffset < NumAchievements())
                {
                    g_AchievementsDialog.ReloadLBXData(nOffset);

                    if (g_AchievementEditorDialog.ActiveAchievement() == pAchievement)
                        g_AchievementEditorDialog.LoadAchievement(pAchievement, TRUE);
                }
#endif

                if (pAchievement->GetPauseOnTrigger())
                {
#ifndef RA_UTEST
                    RA_CausePause();
#endif
                    std::wstring sMessage = ra::StringPrintf(L"Pause on Trigger: %s", pAchievement->Title());
                    ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sMessage);
                }

                break;
            }
        }
    }
}

bool AchievementSet::HasUnsavedChanges() const noexcept
{
    for (auto pAchievement : m_Achievements)
    {
        if (pAchievement && pAchievement->Modified())
            return true;
    }

    return false;
}
