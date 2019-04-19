#include "RA_CodeNotes.h"

#include "RA_AchievementSet.h" // RA_Achievement
#include "RA_Core.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "api\FetchCodeNotes.hh"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\ILocalStorage.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

void CodeNotes::Clear() noexcept { m_CodeNotes.clear(); }

size_t CodeNotes::Load(unsigned int nID)
{
    return m_CodeNotes.size();
}

BOOL CodeNotes::ReloadFromWeb(unsigned int nID)
{
    m_CodeNotes.clear();

    if (nID == 0)
        return FALSE;

    ra::api::FetchCodeNotes::Request request;
    request.GameId = nID;
    request.CallAsync([this](const ra::api::FetchCodeNotes::Response& response)
    {
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download code notes",
                ra::Widen(response.ErrorMessage));
            return;
        }

        for (const auto& pNote : response.Notes)
            m_CodeNotes.try_emplace(pNote.Address, CodeNoteObj(pNote.Author, pNote.Note));

        g_MemoryDialog.RepopulateMemNotesFromFile();
    });

    return TRUE;
}

void CodeNotes::Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::wstring& sNote)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
        m_CodeNotes.try_emplace(nAddr, CodeNoteObj(sAuthor, sNote));
    else
        m_CodeNotes.at(nAddr).SetNote(sNote);

    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['g'] = std::to_string(pGameContext.GameId());
        args['m'] = std::to_string(nAddr);
        args['n'] = ra::Narrow(sNote);

        rapidjson::Document doc;
        if (RAWeb::DoBlockingRequest(RequestSubmitCodeNote, args, doc))
        {
            //	OK!
            MessageBeep(0xFFFFFFFF);
        }
        else
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Could not save note!", L"Please check that you are online and retry.");
        }
    }
}

BOOL CodeNotes::Remove(const ra::ByteAddress& nAddr)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
    {
        RA_LOG("Already deleted this code note? (%u)", nAddr);
        return FALSE;
    }

    m_CodeNotes.erase(nAddr);

    if (ra::services::ServiceLocator::Get<ra::data::UserContext>().IsLoggedIn())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['g'] = std::to_string(pGameContext.GameId());
        args['m'] = std::to_string(nAddr);
        args['n'] = "";

        //	faf
        RAWeb::CreateThreadedHTTPRequest(RequestSubmitCodeNote, args);
    }

    return TRUE;
}
