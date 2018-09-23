#include "RA_CodeNotes.h"

#include <fstream>

#include "RA_Core.h"
#include "RA_httpthread.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_AchievementSet.h" // RA_Achievement
#include "RA_GameData.h"

#include <iomanip>
#include <boost/algorithm/string/replace.hpp>

void CodeNotes::Clear() noexcept { m_CodeNotes.clear(); }

size_t CodeNotes::Load(const std::wstring& sFile)
{
    Clear();

    std::ifstream ifile{ sFile };
    if (!ifile.is_open())
        return 0U;

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ ifile };
    doc.ParseStream(isw);

    if (doc.HasParseError())
        return 0U;

    ASSERT(doc["CodeNotes"].IsArray());

    const auto& NoteArray{ doc["CodeNotes"] };
    for (const auto& note : NoteArray.GetArray())
    {
        if (note["Note"].IsNull())
            continue;

        const std::string& sNote = note["Note"].GetString();
        if (sNote.length() < 2U)
            continue;

        const std::string& sAddr{ note["Address"].GetString() };
        auto nAddr{ static_cast<ra::ByteAddress>(std::stoul(sAddr, nullptr, 16)) };
        const std::string& sAuthor{ note["User"].GetString() }; // Author?

        m_CodeNotes.try_emplace(nAddr, CodeNoteObj{ sAuthor, sNote });
    }

    return m_CodeNotes.size();
}

BOOL CodeNotes::Save(const std::wstring& sFile)
{
    unsigned int nCharsRead = 0;

    FILE* pf = nullptr;
    if (_wfopen_s(&pf, sFile.c_str(), L"w") == 0)
    {
        std::string jsonHeader = "{\n   \"Success\":true,\n   \"CodeNotes\":[";
        std::string gameID = std::to_string(g_pCurrentGameData->GetGameID());
        std::string jsonFooter = "\n   ],\n   \"GameID\":" + gameID + "\n}";
        std::string User = "User";
        std::string Address = "Address";
        std::string Note = "Note";
        std::string Quote = "\"";
        std::string Comma = ",";
        std::string OpenBrace = "{";
        std::string CloseBrace = "}";
        std::string Colon = ":";
        std::string NewLine = "\n";
        std::string Indent = "   ";

        fwrite(jsonHeader.c_str(), sizeof(char), jsonHeader.length(), pf);

        std::map<ra::ByteAddress, CodeNoteObj>::const_iterator iter = FirstNote();
        while (iter != m_CodeNotes.end())
        {
            std::string siAddr = std::to_string(iter->first);
            int intAddr = std::stoi(siAddr);

            std::stringstream stream;
            stream << "0x"
                << std::setfill('0')
                << std::setw(6)
                << std::hex << intAddr;
            std::string sAddr(stream.str());

            std::string sAuthor = iter->second.Author();
            std::string sNote = iter->second.Note();

            boost::replace_all(sNote, "\\", "\\\\");
            boost::replace_all(sNote, "\b", "\\b");
            boost::replace_all(sNote, "\f", "\\f");
            boost::replace_all(sNote, "\n", "\\n");
            boost::replace_all(sNote, "\r", "\\r");
            boost::replace_all(sNote, "\t", "\\t");
            boost::replace_all(sNote, "\"", "\\\"");


            //Yes, I realize I could do this with RapidJson, but this was much easier and I don't intend to get this merged anyway
            fwrite(NewLine.c_str(), sizeof(char), NewLine.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(OpenBrace.c_str(), sizeof(char), OpenBrace.length(), pf);
            fwrite(NewLine.c_str(), sizeof(char), NewLine.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(User.c_str(), sizeof(char), User.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Colon.c_str(), sizeof(char), Colon.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(sAuthor.c_str(), sizeof(char), sAuthor.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Comma.c_str(), sizeof(char), Comma.length(), pf);
            fwrite(NewLine.c_str(), sizeof(char), NewLine.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Address.c_str(), sizeof(char), Address.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Colon.c_str(), sizeof(char), Colon.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(sAddr.c_str(), sizeof(char), sAddr.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Comma.c_str(), sizeof(char), Comma.length(), pf);
            fwrite(NewLine.c_str(), sizeof(char), NewLine.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Note.c_str(), sizeof(char), Note.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(Colon.c_str(), sizeof(char), Colon.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(sNote.c_str(), sizeof(char), sNote.length(), pf);
            fwrite(Quote.c_str(), sizeof(char), Quote.length(), pf);
            fwrite(NewLine.c_str(), sizeof(char), NewLine.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(Indent.c_str(), sizeof(char), Indent.length(), pf);
            fwrite(CloseBrace.c_str(), sizeof(char), CloseBrace.length(), pf);
            iter++;
            if (iter != m_CodeNotes.end()) {
                fwrite(Comma.c_str(), sizeof(char), Comma.length(), pf);
            }

        }
        fwrite(jsonFooter.c_str(), sizeof(char), jsonFooter.length(), pf);
        fclose(pf);
        return TRUE;
    }
    else
    {
        //	Create?
        MessageBox(g_RAMainWnd, _T("Could not save note! Please check conditional statement."), _T("Error!"), MB_OK | MB_ICONWARNING);
        return FALSE;
    }
}

BOOL CodeNotes::ReloadFromWeb(ra::GameID nID)
{
    if (nID == 0)
        return FALSE;

    /*PostArgs args;
    args['g'] = std::to_string(nID);
    RAWeb::CreateThreadedHTTPRequest(RequestCodeNotes, args);*/
    Load(std::to_wstring(nID) + L"-Notes2.txt");
    return TRUE;
}

//	static
void CodeNotes::OnCodeNotesResponse(rapidjson::Document& doc)
{
    //	Persist then reload
    const ra::GameID nGameID = doc["GameID"].GetUint();

    _WriteBufferToFile(g_sHomeDir + RA_DIR_DATA + std::to_wstring(nGameID) + L"-Notes2.txt", doc);

    g_MemoryDialog.RepopulateMemNotesFromFile();
}

void CodeNotes::Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
        m_CodeNotes.insert(std::map<ra::ByteAddress, CodeNoteObj>::value_type(nAddr, CodeNoteObj(sAuthor, sNote)));
    else
        m_CodeNotes.at(nAddr).SetNote(sNote);

    Save(g_sHomeDir + RA_DIR_DATA + std::to_wstring(g_pCurrentGameData->GetGameID()) + L"-Notes2.txt");

    //if (RAUsers::LocalUser().IsLoggedIn())
    //{
    //    PostArgs args;
    //    args['u'] = RAUsers::LocalUser().Username();
    //    args['t'] = RAUsers::LocalUser().Token();
    //    args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
    //    args['m'] = std::to_string(nAddr);
    //    args['n'] = sNote;

    //    rapidjson::Document doc;
    //    if (RAWeb::DoBlockingRequest(RequestSubmitCodeNote, args, doc))
    //    {
    //        //	OK!
    //        MessageBeep(0xFFFFFFFF);
    //    }
    //    else
    //    {
    //        MessageBox(g_RAMainWnd, _T("Could not save note! Please check you are online and retry."), _T("Error!"), MB_OK | MB_ICONWARNING);
    //    }
    //}
}

BOOL CodeNotes::Remove(const ra::ByteAddress& nAddr)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
    {
        RA_LOG("Already deleted this code note? (%u)", nAddr);
        return FALSE;
    }

    m_CodeNotes.erase(nAddr);

    Save(g_sHomeDir + RA_DIR_DATA + std::to_wstring(g_pCurrentGameData->GetGameID()) + L"-Notes2.txt");

    //if (RAUsers::LocalUser().IsLoggedIn())
    //{
    //    PostArgs args;
    //    args['u'] = RAUsers::LocalUser().Username();
    //    args['t'] = RAUsers::LocalUser().Token();
    //    args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
    //    args['m'] = std::to_string(nAddr);
    //    args['n'] = "";

    //    //	faf
    //    RAWeb::CreateThreadedHTTPRequest(RequestSubmitCodeNote, args);
    //}

    return TRUE;
}
