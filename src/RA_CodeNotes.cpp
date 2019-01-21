#include "RA_CodeNotes.h"

#include "RA_AchievementSet.h" // RA_Achievement
#include "RA_Core.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\ILocalStorage.hh"

void CodeNotes::Clear() noexcept { m_CodeNotes.clear(); }

size_t CodeNotes::Load(unsigned int nID)
{
    Clear();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::CodeNotes, std::to_wstring(nID));
    if (pData == nullptr)
        return 0U;

    rapidjson::Document doc;
    if (!LoadDocument(doc, *pData.get()))
        return 0U;

    for (const auto& note : doc.GetArray())
    {
        if (note["Note"].IsNull())
            continue;

        const std::string& sNote = note["Note"].GetString();
        if (sNote.length() < 2U)
            continue;

        const std::string& sAddr{note["Address"].GetString()};
        const ra::ByteAddress nAddr{std::stoul(sAddr, nullptr, 16)};
        const std::string& sAuthor{note["User"].GetString()}; // Author?

        m_CodeNotes.try_emplace(nAddr, CodeNoteObj(sAuthor, sNote));
    }

    return m_CodeNotes.size();
}

BOOL CodeNotes::ReloadFromWeb(unsigned int nID)
{
    if (nID == 0)
        return FALSE;

    PostArgs args;
    args['g'] = std::to_string(nID);
    RAWeb::CreateThreadedHTTPRequest(RequestCodeNotes, args);
    return TRUE;
}

//	static
void CodeNotes::OnCodeNotesResponse(rapidjson::Document& doc)
{
    //	Persist then reload
    const auto nGameID = doc["GameID"].GetUint();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::CodeNotes, std::to_wstring(nGameID));
    if (pData != nullptr)
    {
        rapidjson::Document patchData;
        patchData.CopyFrom(doc["CodeNotes"], doc.GetAllocator());
        SaveDocument(patchData, *pData.get());
    }

    g_MemoryDialog.RepopulateMemNotesFromFile();
}

void CodeNotes::Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote)
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
        args['n'] = sNote;

        rapidjson::Document doc;
        if (RAWeb::DoBlockingRequest(RequestSubmitCodeNote, args, doc))
        {
            //	OK!
            MessageBeep(0xFFFFFFFF);
        }
        else
        {
            MessageBox(g_RAMainWnd, _T("Could not save note! Please check you are online and retry."), _T("Error!"), MB_OK | MB_ICONWARNING);
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
