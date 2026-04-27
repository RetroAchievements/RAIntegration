#ifndef RA_DATA_MOCK_GAMECONTEXT_HH
#define RA_DATA_MOCK_GAMECONTEXT_HH
#pragma once

#include "data\context\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

namespace ra {
namespace data {
namespace context {
namespace mocks {

class MockGameContext : public GameContext
{
public:
    MockGameContext() noexcept
        : m_Override(this)
    {
    }

    void LoadGame(unsigned int nGameId, const std::string&, ra::data::context::GameContext::Mode nMode) noexcept override
    {
        m_nGameId = nGameId;
        m_nMode = nMode;
        m_bWasLoaded = true;
    }

    bool WasLoaded() const noexcept { return m_bWasLoaded; }
    void ResetWasLoaded() noexcept { m_bWasLoaded = false; }

    /// <summary>
    /// Sets the unique identifier of the currently loaded game.
    /// </summary>
    void SetGameId(unsigned int nGameId) 
    {
        m_nGameId = m_nActiveGameId = nGameId;

        if (!m_vSubsets.empty())
        {
            const auto& pSubset = m_vSubsets.front();
            m_vSubsets.front() = Subset(nGameId, nGameId, pSubset.Title(), pSubset.Type());
        }
    }
    void SetActiveGameId(unsigned int nGameId) noexcept { m_nActiveGameId = nGameId; }

    void NotifyBeforeActiveGameChanged() { OnBeforeActiveGameChanged(); }
    void NotifyActiveGameChanged() { OnActiveGameChanged(); }

    void NotifyGameLoad() { BeginLoad(); EndLoad(); }

    /// <summary>
    /// Sets the rich presence display string.
    /// </summary>
    void SetRichPresenceDisplayString(std::wstring sValue)
    {
        auto* richPresence = Assets().FindRichPresence();
        if (richPresence == nullptr)
        {
            auto pRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
            pRichPresence->SetScript(ra::util::String::Printf("Display:\n%s\n", sValue));
            pRichPresence->CreateServerCheckpoint();
            pRichPresence->CreateLocalCheckpoint();
            Assets().Append(std::move(pRichPresence));
        }
        else
        {
            richPresence->SetScript(ra::util::String::Printf("Display:\n%s\n", sValue));
        }

        m_sRichPresenceDisplayString = sValue;
    }

    void SetRichPresenceFromFile(bool bValue)
    {
        auto* pRichPresence = Assets().FindRichPresence();
        if (pRichPresence)
        {
            if (bValue)
                pRichPresence->SetScript("Display:\nThis differs\n");
            else
                pRichPresence->SetScript(ra::util::String::Printf("Display:\n%s\n", m_sRichPresenceDisplayString));
        }
    }

    void InitializeNotes()
    {
        const auto nIndex = Assets().FindItemIndex(ra::data::models::AssetModelBase::TypeProperty,
                                                   ra::etoi(ra::data ::models::AssetType::MemoryNotes));
        if (nIndex != -1)
            Assets().RemoveAt(nIndex);

        auto pMemoryNotes = std::make_unique<MockMemoryNotesModel>();
        pMemoryNotes->Initialize(
            [this](ra::data::ByteAddress nAddress, const std::wstring& sNote) {
                // a note with pointer notation is expected to keep track of where each
                // pointed-at note exists. this normally occurs in DoFrame, but for
                // the unit tests, force the update immediately after the note is updated
                auto* pMemoryNotes = dynamic_cast<MockMemoryNotesModel*>(Assets().FindMemoryNotes());
                if (pMemoryNotes)
                {
                    GSL_SUPPRESS_TYPE3
                    auto* pNote = const_cast<ra::data::models::MemoryNoteModel*>(pMemoryNotes->FindMemoryNoteModel(nAddress, false));
                    if (pNote && pNote->IsPointer())
                    {
                        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
                        pNote->UpdateRawPointerValue(nAddress, pMemoryContext, nullptr);
                    }
                }

                OnMemoryNoteChanged(nAddress, sNote);
            },
            [this](ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote) {
                OnMemoryNoteMoved(nOldAddress, nNewAddress, sNote);
            });
        Assets().Append(std::move(pMemoryNotes));
    }

    bool SetNote(ra::data::ByteAddress nAddress, const std::wstring& sNote)
    {
        auto* pMemoryNotes = dynamic_cast<MockMemoryNotesModel*>(Assets().FindMemoryNotes());
        if (pMemoryNotes == nullptr)
        {
            InitializeNotes();
            pMemoryNotes = dynamic_cast<MockMemoryNotesModel*>(Assets().FindMemoryNotes());
            Expects(pMemoryNotes != nullptr);
        }

        pMemoryNotes->SetServerNote(nAddress, sNote);
        return true;
    }

    bool UpdateMemoryNote(ra::data::ByteAddress nAddress, const std::wstring& sNote)
    {
        auto* pMemoryNotes = dynamic_cast<MockMemoryNotesModel*>(Assets().FindMemoryNotes());
        if (pMemoryNotes == nullptr)
        {
            InitializeNotes();
            pMemoryNotes = dynamic_cast<MockMemoryNotesModel*>(Assets().FindMemoryNotes());
            Expects(pMemoryNotes != nullptr);
        }

        pMemoryNotes->SetNote(nAddress, sNote);
        return true;
    }

    GSL_SUPPRESS_C128
    void InitializeFromAchievementRuntime();

    void MockSubset(uint32_t nSubsetId, const std::string& sName, SubsetType nType = SubsetType::Bonus)
    {
        MockSubset(nSubsetId, nSubsetId, sName, nType);
    }

    void MockSubset(uint32_t nGameId, uint32_t nAchievementSetId, const std::string& sName, SubsetType nType = SubsetType::Bonus)
    {
        m_vSubsets.emplace_back(nAchievementSetId, nGameId, ra::util::String::Widen(sName), nType);
    }

private:
    class MockMemoryNotesModel : public ra::data::models::MemoryNotesModel
    {
    public:
        void Initialize(MemoryNoteChangedFunction fMemoryNoteChanged, MemoryNoteMovedFunction fMemoryNoteMoved)
        {
            m_fMemoryNoteChanged = fMemoryNoteChanged;
            m_fMemoryNoteMoved = fMemoryNoteMoved;
        }
    };

    ra::services::ServiceLocator::ServiceOverride<ra::data::context::GameContext> m_Override;

    std::wstring m_sRichPresenceDisplayString;
    bool m_bHasActiveAchievements{ false };
    bool m_bWasLoaded{ false };
};

} // namespace mocks
} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_GAMECONTEXT_HH
