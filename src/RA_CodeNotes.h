#ifndef RA_CODENOTES_H
#define RA_CODENOTES_H
#pragma once

#include "RA_Defs.h"

class CodeNotes
{
public:
    class CodeNoteObj
    {
    public:
        CodeNoteObj(const std::string& sAuthor, std::string sNote) :
            m_sAuthor(sAuthor), m_sNote(sNote)
        {
        }

    public:
        const std::string& Author() const { return m_sAuthor; }
        const std::string& Note() const { return m_sNote; }

        void SetNote(const std::string& sNote) { m_sNote = sNote; }

    private:
        const std::string m_sAuthor;
        std::string m_sNote;
    };

public:
    void Clear() noexcept;

    size_t Load(unsigned int nID);

    BOOL ReloadFromWeb(unsigned int nID);
    static void OnCodeNotesResponse(rapidjson::Document& doc);

    void Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote);
    BOOL Remove(const ra::ByteAddress& nAddr);

    const CodeNoteObj* FindCodeNote(const ra::ByteAddress& nAddr) const
    {
        std::map<ra::ByteAddress, CodeNoteObj>::const_iterator iter = m_CodeNotes.find(nAddr);
        return(iter != m_CodeNotes.end()) ? &iter->second : nullptr;
    }

    std::map<ra::ByteAddress, CodeNoteObj>::const_iterator FirstNote() const { return m_CodeNotes.begin(); }
    std::map<ra::ByteAddress, CodeNoteObj>::const_iterator EndOfNotes() const { return m_CodeNotes.end(); }

private:
    std::map<ra::ByteAddress, CodeNoteObj> m_CodeNotes;
};

#endif // !RA_CODENOTES_H
