#ifndef RA_CODENOTES_H
#define RA_CODENOTES_H
#pragma once

#include "ra_fwd.h"

class CodeNotes
{
public:
    class CodeNoteObj
    {
    public:
        explicit CodeNoteObj(const std::string& sAuthor, const std::string& sNote) : m_sAuthor{sAuthor}, m_sNote{sNote}
        {}

        const std::string& Author() const noexcept { return m_sAuthor; }
        const std::string& Note() const noexcept { return m_sNote; }

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

    _NODISCARD inline auto begin() const noexcept { return m_CodeNotes.cbegin(); }
    _NODISCARD inline auto end() const noexcept { return m_CodeNotes.cend(); }

private:
    std::map<ra::ByteAddress, CodeNoteObj> m_CodeNotes;
};

#endif // !RA_CODENOTES_H
