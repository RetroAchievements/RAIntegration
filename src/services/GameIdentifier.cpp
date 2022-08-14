#include "GameIdentifier.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include <rc_hash.h>

#include "api\ResolveHash.hh"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"
#include "data\context\SessionTracker.hh"

#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\UnknownGameViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace services {

static constexpr const wchar_t* KNOWN_HASHES_KEY = L"Hashes";

unsigned int GameIdentifier::IdentifyGame(const BYTE* pROM, size_t nROMSize)
{
    m_nPendingMode = ra::data::context::GameContext::Mode::Normal;

    const auto nConsoleId = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>().Id();
    if (nConsoleId == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Cannot identify game for unknown console.");
        return 0U;
    }

    if (pROM == nullptr || nROMSize == 0)
    {
        m_sPendingHash.clear();
        m_nPendingGameId = 0U;
        return 0U;
    }

    char hash[33];
    rc_hash_generate_from_buffer(hash, nConsoleId, pROM, nROMSize);

    return IdentifyHash(hash);
}

unsigned int GameIdentifier::IdentifyHash(const std::string& sHash)
{
    if (!ra::services::ServiceLocator::Get<ra::data::context::UserContext>().IsLoggedIn())
    {
        if (ra::services::ServiceLocator::Get<ra::services::IConfiguration>().
                IsFeatureEnabled(ra::services::Feature::Offline))
        {
            LoadKnownHashes(m_mKnownHashes);

            if (m_mKnownHashes.find(sHash) == m_mKnownHashes.end())
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                    L"Cannot load achievements",
                    L"This game was not previously identified and requires a connection to identify it.");
                return 0U;
            }
        }
        else
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                L"Cannot load achievements",
                L"You must be logged in to load achievements. Please reload the game after logging in.");
            return 0U;
        }
    }

    unsigned int nGameId = 0U;
    m_nPendingMode = ra::data::context::GameContext::Mode::Normal;

    const auto pIter = m_mKnownHashes.find(sHash);
    if (pIter != m_mKnownHashes.end())
    {
        RA_LOG_INFO("Using previously looked up game ID %u for hash %s", nGameId, sHash);
        nGameId = pIter->second;
    }
    else
    {
        ra::api::ResolveHash::Request request;
        request.Hash = sHash;

        const auto response = request.Call();
        if (response.Succeeded())
        {
            nGameId = response.GameId;
            if (nGameId == 0) // Unknown
            {
                RA_LOG_INFO("Could not identify game with hash %s", sHash);

                auto sEstimatedGameTitle = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().GetGameTitle();

                ra::ui::viewmodels::UnknownGameViewModel vmUnknownGame;
                vmUnknownGame.InitializeGameTitles();
                vmUnknownGame.SetSystemName(ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>().Name());
                vmUnknownGame.SetChecksum(ra::Widen(sHash));
                vmUnknownGame.SetEstimatedGameName(ra::Widen(sEstimatedGameTitle));
                vmUnknownGame.SetNewGameName(vmUnknownGame.GetEstimatedGameName());

                if (vmUnknownGame.ShowModal() == ra::ui::DialogResult::OK)
                {
                    nGameId = vmUnknownGame.GetSelectedGameId();

                    if (vmUnknownGame.GetTestMode())
                        m_nPendingMode = ra::data::context::GameContext::Mode::CompatibilityTest;
                }
            }
            else
            {
                RA_LOG_INFO("Successfully looked up game with ID %u", nGameId);
                m_mKnownHashes.insert_or_assign(sHash, nGameId);
            }
        }
        else
        {
            std::wstring sErrorMessage = ra::Widen(response.ErrorMessage);
            if (sErrorMessage.empty())
            {
                auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                sErrorMessage = ra::StringPrintf(L"Error from %s", pConfiguration.GetHostName());
            }

            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not identify game.", sErrorMessage);
        }
    }

    // store the hash and game id - will be used by _RA_ActivateGame (if called)
    m_sPendingHash = sHash;
    m_nPendingGameId = nGameId;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    if (nGameId != 0 && nGameId == pGameContext.GameId())
    {
        // same as currently loaded rom. assume user is switching disks and _RA_ActivateGame won't be called.
        // update the hash now. if it does get called, this is redundant.
        pGameContext.SetGameHash(sHash);
        pGameContext.SetMode(m_nPendingMode);
    }

    return nGameId;
}

void GameIdentifier::ActivateGame(unsigned int nGameId)
{
    if (nGameId != 0)
    {
        if (!ra::services::ServiceLocator::Get<ra::data::context::UserContext>().IsLoggedIn())
        {
            if (!ra::services::ServiceLocator::Get<ra::services::IConfiguration>().
                    IsFeatureEnabled(ra::services::Feature::Offline))
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                    L"Cannot load achievements",
                    L"You must be logged in to load achievements. Please reload the game after logging in.");
                return;
            }
        }

        RA_LOG_INFO("Loading game %u", nGameId);

        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.ClearPopups();

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.LoadGame(nGameId, m_nPendingMode);
        pGameContext.SetGameHash((nGameId == m_nPendingGameId) ? m_sPendingHash : "");

        ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>().BeginSession(nGameId);

        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            bool bShowHardcorePrompt = false;
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::NonHardcoreWarning))
                bShowHardcorePrompt = pGameContext.Assets().HasCoreAssets();

            if (bShowHardcorePrompt)
            {
                ra::ui::viewmodels::MessageBoxViewModel vmWarning;
                vmWarning.SetHeader(L"Enable Hardcore mode?");
                vmWarning.SetMessage(L"You are loading a game with achievements and do not currently have hardcore mode enabled.");
                vmWarning.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
                vmWarning.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

                if (vmWarning.ShowModal() == ra::ui::DialogResult::Yes)
                    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().EnableHardcoreMode(false);
            }
            else
            {
                const bool bLeaderboardsEnabled = pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards);

                ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
                    L"Playing in Softcore Mode",
                    bLeaderboardsEnabled ? L"Leaderboard entries will not be submitted." : L"");
            }
        }
    }
    else
    {
        RA_LOG_INFO("Unloading current game");

        ra::services::ServiceLocator::GetMutable<ra::data::context::SessionTracker>().EndSession();

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.LoadGame(0U, m_nPendingMode);
        pGameContext.SetGameHash((m_nPendingGameId == 0) ? m_sPendingHash : "");
    }

    ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>().ResetMemoryModified();
}

void GameIdentifier::IdentifyAndActivateGame(const BYTE* pROM, size_t nROMSize)
{
    const auto nGameId = IdentifyGame(pROM, nROMSize);
    ActivateGame(nGameId);

    if (nGameId == 0 && pROM && nROMSize)
    {
        // game did not resolve, but still want to display "Playing GAMENAME" in Rich Presence
        auto sEstimatedGameTitle = ra::Widen(ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().GetGameTitle());
        if (sEstimatedGameTitle.empty())
            sEstimatedGameTitle = L"Unknown";

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.SetGameTitle(sEstimatedGameTitle);
    }
}

void GameIdentifier::LoadKnownHashes(std::map<std::string, unsigned>& mHashes)
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pFile = pLocalStorage.ReadText(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY);
    if (pFile != nullptr)
    {
        std::string sLine;
        while (pFile->GetLine(sLine))
        {
            ra::Tokenizer pTokenizer(sLine);
            auto sHash = pTokenizer.ReadTo('=');
            if (sHash.length() == 32)
            {
                pTokenizer.Advance(); // '='
                auto nID = pTokenizer.ReadNumber();
                mHashes[sHash] = nID;
            }
        }
    }
}

void GameIdentifier::SaveKnownHashes(std::map<std::string, unsigned>& mHashes)
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pFile = pLocalStorage.WriteText(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY);
    if (pFile != nullptr)
    {
        std::string sLine;
        for (const auto& pPair : mHashes)
        {
            sLine = ra::StringPrintf("%s=%u", pPair.first, pPair.second);
            pFile->WriteLine(sLine);
        }
    }
}

void GameIdentifier::SaveKnownHashes() const
{
    if (m_mKnownHashes.empty())
        return;

    bool bChanged = false;
    std::map<std::string, unsigned> mHashes;
    LoadKnownHashes(mHashes);

    for (const auto& pPair : m_mKnownHashes)
    {
        const auto pIter = mHashes.find(pPair.first);
        if (pIter == mHashes.end() || pIter->second != pPair.second)
        {
            mHashes[pPair.first] = pPair.second;
            bChanged = true;
        }
    }

    if (bChanged)
        SaveKnownHashes(mHashes);
}

} // namespace services
} // namespace ra
