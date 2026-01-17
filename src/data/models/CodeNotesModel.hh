#ifndef RA_DATA_CODE_NOTES_MODEL_H
#define RA_DATA_CODE_NOTES_MODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "data/models/CodeNoteModel.hh"

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

    bool IsShownInList() const noexcept override { return false; }

    typedef std::function<void(ra::data::ByteAddress nAddress, const std::wstring& sNewNote)> CodeNoteChangedFunction;
    typedef std::function<void(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNewNote)> CodeNoteMovedFunction;

    /// <summary>
    /// Repopulates with code notes from the server.
    /// </summary>
    /// <param name="nGameId">Unique identifier of the game to load code notes for.</param>
    /// <param name="fCodeNoteChanged">Callback to call when a code note is changed.</param>
    /// <param name="fCodeMovedChanged">Callback to call when a code note is moved.</param>
    /// <param name="callback">Callback to call when the loading completes.</param>
    void Refresh(unsigned int nGameId, CodeNoteChangedFunction fCodeNoteChanged, CodeNoteMovedFunction fCodeNoteMoved, std::function<void()> callback);

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>    
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const std::wstring* FindCodeNote(ra::data::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteModel(nAddress);
        return (pNote != nullptr) ? &pNote->GetNote() : nullptr;
    }

    /// <summary>
    /// Returns the note model associated with the specified address.
    /// </summary>
    /// <returns>The note model associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const CodeNoteModel* FindCodeNoteModel(ra::data::ByteAddress nAddress, bool bIncludeDerived = true) const;
    
    /// <summary>
    /// Returns the address of the first byte containing the specified code note.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress FindCodeNoteStart(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <returns>
    ///  The note associated to the address or neighboring addresses based on <paramref name="nSize" />.
    ///  Returns an empty string if no note is associated to the address.
    /// </returns>
    std::wstring FindCodeNote(ra::data::ByteAddress nAddress, Memory::Size nSize) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to look up.</param>
    /// <param name="sAuthor">The author associated to the address.</param>
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    /// <remarks>Does not find notes derived from pointers</remarks>
    const std::wstring* FindCodeNote(ra::data::ByteAddress nAddress, _Inout_ std::string& sAuthor) const;

    /// <summary>
    /// Returns the number of bytes associated to the code note at the specified address.
    /// </summary>
    /// <param name="nAddress">Address to query.</param>
    /// <returns>Number of bytes associated to the code note, or 0 if no note exists at the address.</returns>
    /// <remarks>Only works for the first byte of a multi-byte address.</remarks>
    unsigned GetCodeNoteBytes(ra::data::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteModel(nAddress);
        return (pNote == nullptr) ? 0 : pNote->GetBytes();
    }

    /// <summary>
    /// Returns the number of bytes associated to the code note at the specified address.
    /// </summary>
    /// <param name="nAddress">Address to query.</param>
    /// <returns>Memory::Size associated to the code note, or Unknown if no note exists at the address.</returns>
    /// <remarks>Only works for the first byte of a multi-byte address.</remarks>
    Memory::Size GetCodeNoteMemSize(ra::data::ByteAddress nAddress) const
    {
        const auto* pNote = FindCodeNoteModel(nAddress);
        return (pNote == nullptr) ? Memory::Size::Unknown : pNote->GetMemSize();
    }

    /// <summary>
    /// Returns the address of the real code note from which an indirect code note was derived.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found, or not an indirect note.
    /// </returns>
    ra::data::ByteAddress GetIndirectSource(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Returns the address of the next code note after the provided address.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress GetNextNoteAddress(ra::data::ByteAddress nAfterAddress, bool bIncludeDerived = false) const;

    /// <summary>
    /// Returns the address of the next code note before the provided address.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress GetPreviousNoteAddress(ra::data::ByteAddress nBeforeAddress, bool bIncludeDerived = false) const;

    /// <summary>
    /// Enumerates the code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateCodeNotes(std::function<bool(ra::data::ByteAddress nAddress, unsigned int nBytes, const std::wstring& sNote)> callback, bool bIncludeDerived = false) const
    {
        EnumerateCodeNotes([callback](ra::data::ByteAddress nAddress, const CodeNoteModel& pCodeNote)
        {
            return callback(nAddress, pCodeNote.GetBytes(), pCodeNote.GetNote());
        }, bIncludeDerived);
    }

    /// <summary>
    /// Enumerates the code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateCodeNotes(std::function<bool(ra::data::ByteAddress nAddress, const CodeNoteModel& pCodeNote)> callback, bool bIncludeDerived = false) const;

    /// <summary>
    /// Sets the note to associate with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to set the note for.</param>
    /// <param name="sNote">The new note for the address.</param>
    /// <returns><c>true</c> if the note was updated, </c>false</c> if an error occurred.</returns>
    void SetCodeNote(ra::data::ByteAddress nAddress, const std::wstring& sNote);

    /// <summary>
    /// Returns the number of known code notes (not including indirect notes).
    /// </summary>
    size_t CodeNoteCount() const noexcept { return m_vCodeNotes.size(); }

    /// <summary>
    /// Gets the address of the first code note.
    /// </summary>
    ra::data::ByteAddress FirstCodeNoteAddress() const noexcept
    {
        return (m_vCodeNotes.empty()) ? 0U : m_vCodeNotes.front()->GetAddress();
    }

    /// <summary>
    /// Enumerates the modified code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateModifiedCodeNotes(std::function<bool(ra::data::ByteAddress nAddress)> callback) const
    {
        for (const auto& pair : m_mOriginalCodeNotes)
        {
            if (!callback(pair.first))
                break;
        }
    }

    /// <summary>
    /// Gets whether or not the note for specified address has been modified.
    /// </summary>
    bool IsNoteModified(ra::data::ByteAddress nAddress) const
    {
        return m_mOriginalCodeNotes.find(nAddress) != m_mOriginalCodeNotes.end();
    }

    void SetServerCodeNote(ra::data::ByteAddress nAddress, const std::wstring& sNote);
    const std::wstring* GetServerCodeNote(ra::data::ByteAddress nAddress) const;
    const std::string* GetServerCodeNoteAuthor(ra::data::ByteAddress nAddress) const;

	void Serialize(ra::services::TextWriter&) const override;
    bool Deserialize(ra::Tokenizer&) override;

    void DoFrame() override;

protected:
    void AddCodeNote(ra::data::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote);
    void OnCodeNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote);

    std::vector<std::unique_ptr<CodeNoteModel>> m_vCodeNotes;
    std::map<ra::data::ByteAddress, std::pair<std::string, std::wstring>> m_mOriginalCodeNotes;

    std::map<ra::data::ByteAddress, std::wstring> m_mPendingCodeNotes;

    std::pair<ra::data::ByteAddress, const CodeNoteModel*> FindIndirectCodeNoteInternal(ra::data::ByteAddress nAddress) const;

    unsigned int m_nGameId = 0;
    bool m_bHasPointers = false;
    bool m_bRefreshing = false;

    CodeNoteChangedFunction m_fCodeNoteChanged;
    CodeNoteMovedFunction m_fCodeNoteMoved;

private:
    static std::wstring BuildCodeNoteSized(ra::data::ByteAddress nAddress, unsigned nCheckBytes, ra::data::ByteAddress nNoteAddress, const CodeNoteModel& pNote);

    mutable std::mutex m_oMutex;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_CODE_NOTES_MODEL_H
