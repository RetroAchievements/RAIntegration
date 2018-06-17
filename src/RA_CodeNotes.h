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
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
        CodeNoteObj(_In_ const std::string& sAuthor, _In_ const std::string& sNote) noexcept :
            m_sAuthor(sAuthor), m_sNote(sNote)
        {
        }
#pragma warning(pop)

        ~CodeNoteObj() noexcept = default;
        CodeNoteObj(const CodeNoteObj&) = delete;
        CodeNoteObj& operator=(const CodeNoteObj&) = delete;
        CodeNoteObj(CodeNoteObj&&) noexcept = default;
        CodeNoteObj& operator=(CodeNoteObj&&) noexcept = default;

    public:
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
        const std::string& Author() const { return m_sAuthor; }
        const std::string& Note() const { return m_sNote; }
        void SetNote(const std::string& sNote) { m_sNote = sNote; }
#pragma warning(pop)

    private:
        // move assignment was implicitly defined as deleted, if you can't move you surely can't copy
        // put we don't need to copy
        std::string m_sAuthor;
        std::string m_sNote;
    };

public:
    void Clear();

    BOOL Save(const std::string& sFile);
    size_t Load(const std::string& sFile);

    BOOL ReloadFromWeb(GameID nID);
    static void OnCodeNotesResponse(Document& doc);

    void Add(const ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote);
    BOOL Remove(const ByteAddress& nAddr);

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    const CodeNoteObj* FindCodeNote(const ByteAddress& nAddr) const
    {
        std::map<ByteAddress, CodeNoteObj>::const_iterator iter = m_CodeNotes.find(nAddr);
        return(iter != m_CodeNotes.end()) ? &iter->second : nullptr;
    }

    // NB: We are severely limiting this API by using nonstandard function names
    //     if you wanted to use a ranged for CodeNotes, it would have be begin and end respectively

    std::map<ByteAddress, CodeNoteObj>::const_iterator FirstNote() const { return m_CodeNotes.begin(); }
    std::map<ByteAddress, CodeNoteObj>::const_iterator EndOfNotes() const { return m_CodeNotes.end(); }
#pragma warning(pop)


private:
    std::map<ByteAddress, CodeNoteObj> m_CodeNotes;
};

#endif // !RA_CODENOTES_H
