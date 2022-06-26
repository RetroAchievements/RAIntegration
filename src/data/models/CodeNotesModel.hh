#ifndef RA_DATA_CODE_NOTES_MODEL_H
#define RA_DATA_CODE_NOTES_MODEL_H
#pragma once

#include "AssetModelBase.hh"

#include "data/Types.hh"

namespace ra {
namespace data {
namespace models {

class CodeNotesModel : public AssetModelBase
{
public:
	CodeNotesModel() noexcept;
	~CodeNotesModel() = default;
	CodeNotesModel(const CodeNotesModel&) noexcept = delete;
	CodeNotesModel& operator=(const CodeNotesModel&) noexcept = delete;
	CodeNotesModel(CodeNotesModel&&) noexcept = delete;
	CodeNotesModel& operator=(CodeNotesModel&&) noexcept = delete;

    bool IsHidden() const override { return true; }

    typedef std::function<void(ra::ByteAddress nAddress, const std::wstring& sNewNote)> CodeNoteChangedFunction;

    /// <summary>
    /// Repopulates with code notes from the server.
    /// </summary>
    /// <param name="nGameId">Unique identifier of the game to load code notes for.</param>
    /// <param name="fCodeNoteChanged">Callback to call when a code note is changed.</param>
    /// <param name="callback">Callback to call when the loading completes.</param>
    void Refresh(unsigned int nGameId, CodeNoteChangedFunction fCodeNoteChanged, std::function<void()> callback);

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>    
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const std::wstring* FindCodeNote(ra::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteInternal(nAddress);
        return (pNote != nullptr) ? &pNote->Note : nullptr;
    }
    
    /// <summary>
    /// Returns the address of the first byte containing the specified code note.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::ByteAddress FindCodeNoteStart(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <returns>
    ///  The note associated to the address or neighboring addresses based on <paramref name="nSize" />.
    ///  Returns an empty string if no note is associated to the address.
    /// </returns>
    std::wstring FindCodeNote(ra::ByteAddress nAddress, MemSize nSize) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to look up.</param>
    /// <param name="sAuthor">The author associated to the address.</param>
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    /// <remarks>Does not find notes derived from pointers</remarks>
    const std::wstring* FindCodeNote(ra::ByteAddress nAddress, _Inout_ std::string& sAuthor) const;

    /// <summary>
    /// Returns the number of bytes associated to the code note at the specified address.
    /// </summary>
    /// <param name="nAddress">Address to query.</param>
    /// <returns>Number of bytes associated to the code note, or 0 if no note exists at the address.</returns>
    /// <remarks>Only works for the first byte of a multi-byte address.</remarks>
    unsigned GetCodeNoteBytes(ra::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteInternal(nAddress);
        return (pNote == nullptr) ? 0 : pNote->Bytes;
    }

    /// <summary>
    /// Returns the number of bytes associated to the code note at the specified address.
    /// </summary>
    /// <param name="nAddress">Address to query.</param>
    /// <returns>Number of bytes associated to the code note, or 0 if no note exists at the address.</returns>
    /// <remarks>Only works for the first byte of a multi-byte address.</remarks>
    MemSize GetCodeNoteMemSize(ra::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteInternal(nAddress);
        return (pNote == nullptr) ? MemSize::Unknown : pNote->MemSize;
    }

    /// <summary>
    /// Enumerates the code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress)> callback) const
    {
        EnumerateCodeNotes([callback](ra::ByteAddress nAddress, const CodeNote&)
        {
            return callback(nAddress);
        });
    }

    /// <summary>
    /// Enumerates the code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress, unsigned int nBytes, const std::wstring& sNote)> callback) const
    {
        EnumerateCodeNotes([callback](ra::ByteAddress nAddress, const CodeNote& pCodeNote)
        {
            return callback(nAddress, pCodeNote.Bytes, pCodeNote.Note);
        });
    }

    /// <summary>
    /// Sets the note to associate with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to set the note for.</param>
    /// <param name="sNote">The new note for the address.</param>
    /// <returns><c>true</c> if the note was updated, </c>false</c> if an error occurred.</returns>
    virtual bool SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote);

    /// <summary>
    /// Deletes the note associated with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to delete the note for.</param>
    /// <returns><c>true</c> if the note was deleted, </c>false</c> if an error occurred.</returns>
    virtual bool DeleteCodeNote(ra::ByteAddress nAddress);

    /// <summary>
    /// Returns the number of known code notes
    /// </summary>
    size_t CodeNoteCount() const noexcept { return m_mCodeNotes.size(); }

	void Serialize(ra::services::TextWriter&) const noexcept override {}
	bool Deserialize(ra::Tokenizer&) noexcept override { return true; }

protected:
    void AddCodeNote(ra::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote);
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote);

    struct PointerData;

    struct CodeNote
    {
        std::string Author;
        std::wstring Note;
        unsigned int Bytes = 1;
        MemSize MemSize = MemSize::Unknown;
        std::unique_ptr<PointerData> PointerData;
    };

    struct OffsetCodeNote : public CodeNote
    {
        int Offset = 0;
    };

    struct PointerData
    {
        ra::ByteAddress PointerValue = 0;
        unsigned int OffsetRange = 0;
        std::vector<OffsetCodeNote> OffsetNotes;
    };

    std::map<ra::ByteAddress, CodeNote> m_mCodeNotes;

    const CodeNote* FindCodeNoteInternal(ra::ByteAddress nAddress) const;
    void EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNote& pCodeNote)> callback) const;
    static std::wstring BuildCodeNoteSized(ra::ByteAddress nAddress, unsigned nCheckBytes, ra::ByteAddress nNoteAddress, const CodeNote& pNote);

    unsigned int m_nGameId = 0;
    bool m_bHasPointers = false;

    CodeNoteChangedFunction m_fCodeNoteChanged;

private:
    static void ExtractSize(CodeNote& pNote);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_LOCAL_BADGES_MODEL_H
