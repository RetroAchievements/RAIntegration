#include "RA_CodeNotes.h"

#include "RA_Core.h"
#include "RA_httpthread.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_AchievementSet.h" // RA_Achievement
#include "RA_GameData.h"

void CodeNotes::Clear()
{
    m_CodeNotes.clear();
}

size_t CodeNotes::Load(const std::string& sFile)
{
    Clear();

    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    FILE* pf = nullptr;
    if (fopen_s(&pf, sFile.c_str(), "rb") == 0)
    {
        Document doc;
        FileStream fs{ pf };
        doc.ParseStream(fs);

        if (!doc.HasParseError())
        {
            ASSERT(doc["CodeNotes"].IsArray());

            const Value& NoteArray = doc["CodeNotes"];

            for (SizeType i = 0; i < NoteArray.Size(); ++i)
            {
                const Value& NextNote = NoteArray[i];
                if (NextNote["Note"].IsNull())
                    continue;

                const std::string& sNote = NextNote["Note"].GetString();
                if (sNote.length() < 2)
                    continue;

                const std::string& sAddr = NextNote["Address"].GetString();
                ra::ByteAddress nAddr = static_cast<ra::ByteAddress>(std::strtoul(sAddr.c_str(), nullptr, 16));
                const std::string& sAuthor = NextNote["User"].GetString();	//	Author?

                m_CodeNotes.insert(std::map<ra::ByteAddress, CodeNoteObj>::value_type(nAddr, CodeNoteObj(sAuthor, sNote)));
            }
        }
        fclose(pf);
    }

    return m_CodeNotes.size();
}

BOOL CodeNotes::Save(_UNUSED const std::string& sFile)
{
    return FALSE;
    //	All saving should be cloud-based!
}

BOOL CodeNotes::ReloadFromWeb(ra::GameID nID)
{
    if (nID == 0)
        return FALSE;

    PostArgs args;
    args['g'] = std::to_string(nID);
    RAWeb::CreateThreadedHTTPRequest(RequestCodeNotes, args);
    return TRUE;
}

//	static
void CodeNotes::OnCodeNotesResponse(Document& doc)
{
    //	Persist then reload
    const ra::GameID nGameID = doc["GameID"].GetUint();

    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    _WriteBufferToFile(std::string(RA_DIR_DATA) + std::to_string(nGameID) + "-Notes2.txt", doc);

    g_MemoryDialog.RepopulateMemNotesFromFile();
}

void CodeNotes::Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
        m_CodeNotes.insert(std::map<ra::ByteAddress, CodeNoteObj>::value_type(nAddr, CodeNoteObj(sAuthor, sNote)));
    else
        m_CodeNotes.at(nAddr).SetNote(sNote);

    if (RAUsers::LocalUser().IsLoggedIn())
    {
        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
        args['m'] = std::to_string(nAddr);
        args['n'] = sNote;

        Document doc;
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
        RA_LOG("Already deleted this code note? (%d), nAddr ");
        return FALSE;
    }

    m_CodeNotes.erase(nAddr);

    if (RAUsers::LocalUser().IsLoggedIn())
    {
        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
        args['m'] = std::to_string(nAddr);
        args['n'] = "";

        //	faf
        RAWeb::CreateThreadedHTTPRequest(RequestSubmitCodeNote, args);
    }

    return TRUE;
}
