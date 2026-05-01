#ifndef RA_DATA_MODELS_MEMORYNOTESMODEL_H
#define RA_DATA_MODELS_MEMORYNOTESMODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "data/models/MemoryNoteModel.hh"

namespace ra {
namespace data {
namespace models {

class MemoryNotesModel : public AssetModelBase
{
public:
	MemoryNotesModel() noexcept;
	~MemoryNotesModel() = default;
	MemoryNotesModel(const MemoryNotesModel&) noexcept = delete;
	MemoryNotesModel& operator=(const MemoryNotesModel&) noexcept = delete;
	MemoryNotesModel(MemoryNotesModel&&) noexcept = delete;
	MemoryNotesModel& operator=(MemoryNotesModel&&) noexcept = delete;

    typedef std::function<void(ra::data::ByteAddress nAddress, const std::wstring& sNewNote)> MemoryNoteChangedFunction;
    typedef std::function<void(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNewNote)> MemoryNoteMovedFunction;

    /// <summary>
    /// Repopulates with memory notes from the server.
    /// </summary>
    /// <param name="nGameId">Unique identifier of the game to load memory notes for.</param>
    /// <param name="fMemoryNoteChanged">Callback to call when a memory note is changed.</param>
    /// <param name="fMemoryNoteMoved">Callback to call when a memory note is moved.</param>
    /// <param name="callback">Callback to call when the loading completes.</param>
    void Refresh(unsigned int nGameId, MemoryNoteChangedFunction fMemoryNoteChanged, MemoryNoteMovedFunction fMemoryNoteMoved, std::function<void()> callback);

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>    
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const std::wstring* FindNote(ra::data::ByteAddress nAddress) const
    {
        const auto* pNote = FindMemoryNoteModel(nAddress);
        return (pNote != nullptr) ? &pNote->GetNote() : nullptr;
    }

    /// <summary>
    /// Returns the note model associated with the specified address.
    /// </summary>
    /// <returns>The note model associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const MemoryNoteModel* FindMemoryNoteModel(ra::data::ByteAddress nAddress, bool bIncludeDerived = true) const;
    
    /// <summary>
    /// Returns the address of the first byte containing the specified memory note.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress FindNoteStart(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <returns>
    ///  The note associated to the address or neighboring addresses based on <paramref name="nSize" />.
    ///  Returns an empty string if no note is associated to the address.
    /// </returns>
    std::wstring FindNote(ra::data::ByteAddress nAddress, Memory::Size nSize) const;

    /// <summary>
    /// Returns the address of the real memory note from which an indirect memory note was derived.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found, or not an indirect note.
    /// </returns>
    ra::data::ByteAddress GetIndirectSource(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Returns the address of the next memory note after the provided address.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress GetNextNoteAddress(ra::data::ByteAddress nAfterAddress, bool bIncludeDerived = false) const;

    /// <summary>
    /// Returns the address of the next memory note before the provided address.
    /// </summary>
    /// <returns>
    ///  Returns 0xFFFFFFFF if not found.
    /// </returns>
    ra::data::ByteAddress GetPreviousNoteAddress(ra::data::ByteAddress nBeforeAddress, bool bIncludeDerived = false) const;

    /// <summary>
    /// Enumerates the memory notes.
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known memory note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateMemoryNotes(std::function<bool(ra::data::ByteAddress nAddress, const MemoryNoteModel& pMemoryNote)> callback, bool bIncludeDerived = false) const;

    /// <summary>
    /// Sets the note to associate with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to set the note for.</param>
    /// <param name="sNote">The new note for the address.</param>
    /// <returns><c>true</c> if the note was updated, </c>false</c> if an error occurred.</returns>
    void SetNote(ra::data::ByteAddress nAddress, const std::wstring& sNote);

    /// <summary>
    /// Returns the number of known memory notes (not including indirect notes).
    /// </summary>
    size_t NoteCount() const noexcept { return m_vMemoryNotes.size(); }

    /// <summary>
    /// Gets the address of the first memory note.
    /// </summary>
    ra::data::ByteAddress FirstNoteAddress() const noexcept
    {
        return (m_vMemoryNotes.empty()) ? 0U : m_vMemoryNotes.front()->GetAddress();
    }

    /// <summary>
    /// Enumerates the modified memory notes.
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known memory note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateModifiedNotes(std::function<bool(ra::data::ByteAddress nAddress)> callback) const
    {
        for (const auto& pair : m_mOriginalNotes)
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
        return m_mOriginalNotes.find(nAddress) != m_mOriginalNotes.end();
    }

    void SetServerNote(ra::data::ByteAddress nAddress, const std::wstring& sNote);
    const std::wstring* GetServerNote(ra::data::ByteAddress nAddress) const;
    const std::string* GetServerNoteAuthor(ra::data::ByteAddress nAddress) const;

	void Serialize(ra::services::TextWriter&) const override;
    bool Deserialize(ra::util::Tokenizer&) override;

    void DoFrame() override;

    bool IsShownInList() const noexcept override { return false; }

protected:
    void AddMemoryNote(ra::data::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote);
    void OnMemoryNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote);

    std::vector<std::unique_ptr<MemoryNoteModel>> m_vMemoryNotes;
    std::map<ra::data::ByteAddress, std::pair<std::string, std::wstring>> m_mOriginalNotes;

    std::map<ra::data::ByteAddress, std::wstring> m_mPendingNotes;

    std::pair<ra::data::ByteAddress, const MemoryNoteModel*> FindIndirectMemoryNoteInternal(ra::data::ByteAddress nAddress) const;

    bool m_bHasPointers = false;
    bool m_bRefreshing = false;

    MemoryNoteChangedFunction m_fMemoryNoteChanged;
    MemoryNoteMovedFunction m_fMemoryNoteMoved;

private:
    static std::wstring BuildNoteForAddress(ra::data::ByteAddress nAddress, unsigned nCheckBytes, ra::data::ByteAddress nNoteAddress, const MemoryNoteModel& pNote);

    mutable std::mutex m_oMutex;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_MEMORYNOTESMODEL_H
