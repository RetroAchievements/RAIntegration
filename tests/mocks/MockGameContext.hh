#ifndef RA_DATA_MOCK_GAMECONTEXT_HH
#define RA_DATA_MOCK_GAMECONTEXT_HH
#pragma once

#include "data\context\GameContext.hh"

#include "services\ServiceLocator.hh"

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
    void SetGameId(unsigned int nGameId) noexcept { m_nGameId = nGameId; }

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
            pRichPresence->SetScript(ra::StringPrintf("Display:\n%s\n", sValue));
            pRichPresence->CreateServerCheckpoint();
            pRichPresence->CreateLocalCheckpoint();
            Assets().Append(std::move(pRichPresence));
        }
        else
        {
            richPresence->SetScript(ra::StringPrintf("Display:\n%s\n", sValue));
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
                pRichPresence->SetScript(ra::StringPrintf("Display:\n%s\n", m_sRichPresenceDisplayString));
        }
    }

    void InitializeCodeNotes()
    {
        const auto nIndex = Assets().FindItemIndex(ra::data::models::AssetModelBase::TypeProperty,
                                                   ra::etoi(ra::data ::models::AssetType::CodeNotes));
        if (nIndex != -1)
            Assets().RemoveAt(nIndex);

        auto pCodeNotes = std::make_unique<MockCodeNotesModel>();
        pCodeNotes->Initialize(1U,
            [this](ra::ByteAddress nAddress, const std::wstring& sNote) {
                OnCodeNoteChanged(nAddress, sNote);
            });
        Assets().Append(std::move(pCodeNotes));
    }

    bool SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
    {
        auto* pCodeNotes = dynamic_cast<MockCodeNotesModel*>(Assets().FindCodeNotes());
        if (pCodeNotes == nullptr)
        {
            InitializeCodeNotes();
            pCodeNotes = dynamic_cast<MockCodeNotesModel*>(Assets().FindCodeNotes());
            Expects(pCodeNotes != nullptr);
        }

        pCodeNotes->SetServerCodeNote(nAddress, sNote);
        return true;
    }

    bool UpdateCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
    {
        auto* pCodeNotes = dynamic_cast<MockCodeNotesModel*>(Assets().FindCodeNotes());
        if (pCodeNotes == nullptr)
        {
            InitializeCodeNotes();
            pCodeNotes = dynamic_cast<MockCodeNotesModel*>(Assets().FindCodeNotes());
            Expects(pCodeNotes != nullptr);
        }

        pCodeNotes->SetCodeNote(nAddress, sNote);
        return true;
    }

private:
    class MockCodeNotesModel : public ra::data::models::CodeNotesModel
    {
    public:
        void Initialize(unsigned nGameId, CodeNoteChangedFunction fCodeNoteChanged)
        {
            m_nGameId = nGameId;
            m_fCodeNoteChanged = fCodeNoteChanged;
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
