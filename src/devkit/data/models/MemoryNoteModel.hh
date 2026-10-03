#ifndef RA_DATA_MODELS_MEMORYNOTEMODEL_H
#define RA_DATA_MODELS_MEMORYNOTEMODEL_H
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "data/Memory.hh"

#include "util/Compat.hh"

namespace ra {
namespace data {
namespace models {

enum class MemoryNoteType : uint8_t
{
    None = 0,
    Value,
    Pointer,
    Array,
};

class MemoryNoteModel
{
public:
    MemoryNoteModel(std::wstring_view svNote, uint32_t nBytes, Memory::Format nMemFormat, Memory::Size nMemSize, MemoryNoteType nType) noexcept
        : m_svNote(svNote),
          m_nBytes(nBytes),
          m_nMemFormat(nMemFormat),
          m_nMemSize(nMemSize),
          m_nType(nType)
    {
    }

	virtual ~MemoryNoteModel() = default;
    MemoryNoteModel(const MemoryNoteModel&) noexcept = delete;
    MemoryNoteModel& operator=(const MemoryNoteModel&) noexcept = delete;
    MemoryNoteModel(MemoryNoteModel&&) noexcept = default;
    MemoryNoteModel& operator=(MemoryNoteModel&&) noexcept = default;

    /// <summary>
    /// Creates a MemoryNoteModel from a note text.
    /// </summary>
    static std::unique_ptr<MemoryNoteModel> Parse(const std::wstring& sNote);

    /// <summary>
    /// Creates a nested MemoryNoteModel from a note text.
    /// </summary>
    static std::unique_ptr<MemoryNoteModel> ParseOffsetNote(std::wstring_view svNote, std::wstring_view svIndent);

    /// <summary>
    /// Gets the address/offset of the note.
    /// </summary>
    const ra::data::ByteAddress GetAddress() const noexcept { return m_nAddress; }

    /// <summary>
    /// Sets the address/offset of the note.
    /// </summary>
    void SetAddress(ra::data::ByteAddress nAddress) noexcept { m_nAddress = nAddress; }

    /// <summary>
    /// Gets the full note.
    /// </summary>
    virtual std::wstring GetNote() const { return std::wstring(m_svNote); }

    /// <summary>
    /// Returns true if GetNote would return a non-empty string.
    /// </summary>
    /// <remarks>Avoids constructing a wstring just to check for existance.</remarks>
    bool HasNote() const noexcept { return !m_svNote.empty(); }

    /// <summary>
    /// Sets the full note.
    /// </summary>
    void SetNote(const std::wstring_view& svNote) noexcept { m_svNote = svNote; }

    /// <summary>
    /// Gets the number of bytes that the note is associated to.
    /// </summary>
    const unsigned int GetBytes() const noexcept { return m_nBytes; }

    /// <summary>
    /// Gets the primary size of the note.
    /// </summary>
    const Memory::Size GetMemSize() const noexcept { return m_nMemSize; }

    /// <summary>
    /// Sets the primary size of the note.
    /// </summary>
    void SetMemSize(Memory::Size nMemSize) noexcept { m_nMemSize = nMemSize; }

    /// <summary>
    /// Gets whether values associated to the note should be shown in hexadecimal or decimal.
    /// </summary>
    const Memory::Format GetDefaultMemFormat() const noexcept { return m_nMemFormat; }

    /// <summary>
    /// Gets the non-enum/subnote portion of the note with any size information removed.
    /// </summary>
    std::wstring GetSummary() const
    {
        return TrimSize(GetFullSummaryStringView(), false);
    }

    /// <summary>
    /// Gets the non-enum/subnote portion of the note.
    /// </summary>
    std::wstring GetFullSummary() const
    {
        return std::wstring(GetFullSummaryStringView());
    }

    /// <summary>
    /// Gets the type of note.
    /// </summary>
    MemoryNoteType GetType() const noexcept { return m_nType; }

    typedef struct Reference
    {
        Reference() noexcept {}
        Reference(const MemoryNoteModel* pMemoryNote, ra::data::ByteAddress nAddress, uint32_t nElementIndex) noexcept
            : pMemoryNote(pMemoryNote), nAddress(nAddress), nElementIndex(nElementIndex)
        {
        }

        /// <summary>
        /// The matched note, <c>nullptr</c> if not matched.
        /// </summary>
        const MemoryNoteModel* pMemoryNote = nullptr;

        /// <summary>
        /// The address where the note applied.
        /// </summary>
        ra::data::ByteAddress nAddress = 0;

        /// <summary>
        /// The array offset (when pMemoryNote is an ArrayMemoryNoteModel).
        /// </summary>
        uint32_t nElementIndex = 0;
    } Reference;

    /// <summary>
    /// Builds a reference chain to the specified address.
    /// </summary>
    /// <returns><c>true</c> if a chain was built, <c>false</c> if the specified address could not be reached from this note.</returns>
    virtual bool GetChainTo(std::vector<Reference>& vChain, ra::data::ByteAddress nSearchAddress) const;

    /// <summary>
    /// Builds a reference chain to the specified address.
    /// </summary>
    /// <returns><c>true</c> if a chain was built, <c>false</c> if the specified address could not be reached from this note.</returns>
    virtual bool GetChainTo(std::vector<Reference>& vChain, const MemoryNoteModel& pNote) const;

    /// <summary>
    /// Determines if this note is an ancestor of the provided note.
    /// </summary>
    bool IsAncestorOf(const MemoryNoteModel& pPossibleDescendant) const noexcept
    {
        if (m_svNote.empty())
            return false;

        return pPossibleDescendant.m_svNote.data() >= m_svNote.data() &&
               (pPossibleDescendant.m_svNote.data() + pPossibleDescendant.m_svNote.size()) <= (m_svNote.data() + m_svNote.size());
    }

    /// <summary>
    /// Gets the address of the closest subnote before the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    virtual bool GetPreviousAddress(_UNUSED ra::data::ByteAddress nBeforeAddress, _UNUSED ra::data::ByteAddress& nPreviousAddress) const noexcept(false) { return false; }

    /// <summary>
    /// Gets the address of the closest subnote after the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    virtual bool GetNextAddress(_UNUSED ra::data::ByteAddress nAfterAddress, _UNUSED ra::data::ByteAddress& nNextAddress) const noexcept(false) { return false; }

    /// <summary>
    /// Removes the size annotation from a note string.
    /// </summary>
    /// <param name="sNote">The note string to process.</param>
    /// <param name="bKeepPointer"><c>true</c> to prefix the result with '[pointer]' if a pointer annotation was seen.</param>
    static std::wstring TrimSize(std::wstring_view sNote, bool bKeepPointer);

protected:
    /// <summary>
    /// Gets the non-enum/subnote portion of the note.
    /// </summary>
    virtual std::wstring_view GetFullSummaryStringView() const;

    uint32_t m_nAddress = 0;                             // The address/offset of the note.
    uint32_t m_nBytes = 1;                               // The number of bytes associated to the note.
    std::wstring_view m_svNote;                          // The contents of the note.
    MemoryNoteType m_nType = MemoryNoteType::None;       // The type of note (which subclass is implemented).
    Memory::Size m_nMemSize = Memory::Size::Unknown;     // The logical size of the note.
    Memory::Format m_nMemFormat = Memory::Format::Dec;   // Whether the note value should be displayed as hex or dec.

    enum EnumState : uint8_t {
        None,
        Hex,
        Dec,
        Bits,
        Unknown,
    };
    mutable EnumState m_nEnumState = EnumState::Unknown; // Information about specifically identified values for the note.
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_MEMORYNOTEMODEL_H
