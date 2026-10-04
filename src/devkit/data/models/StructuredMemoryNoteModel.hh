#ifndef RA_DATA_MODELS_STRUCTUREDMEMORYNOTEMODEL_H
#define RA_DATA_MODELS_STRUCTUREDMEMORYNOTEMODEL_H
#pragma once

#include "MemoryNoteModel.hh"

#include <functional>

namespace ra {
namespace data {
namespace models {

class StructuredMemoryNoteModel : public MemoryNoteModel
{
public:
    StructuredMemoryNoteModel(std::wstring_view svNote, uint32_t nBytes, Memory::Format nMemFormat, Memory::Size nMemSize, MemoryNoteType nType) noexcept
        : MemoryNoteModel(svNote, nBytes, nMemFormat, nMemSize, nType)
    {
    }
    ~StructuredMemoryNoteModel() = default;
    StructuredMemoryNoteModel(const StructuredMemoryNoteModel&) noexcept = delete;
    StructuredMemoryNoteModel& operator=(const StructuredMemoryNoteModel&) noexcept = delete;
    StructuredMemoryNoteModel(StructuredMemoryNoteModel&&) noexcept = default;
    StructuredMemoryNoteModel& operator=(StructuredMemoryNoteModel&&) noexcept = default;

    /// <summary>
    /// Gets the full note.
    /// </summary>
    std::wstring GetNote() const override;

    /// <summary>
    /// Gets the last known address of the first byte of the structure.
    /// </summary>
    ra::data::ByteAddress GetBaseAddress() const noexcept { return m_nBaseAddress; }

    /// <summary>
    /// Get the subnote for the field at the specified offset.
    /// </summary>
    /// <returns>Requested subnote, <c>null</c> if not found.</returns>
    virtual const MemoryNoteModel* GetNoteAtOffset(int nOffset) const;

    /// <summary>
    /// Returns the note containing the specified offset.
    /// </summary>
    /// <returns>
    /// If not found, the return value's second parameter will be nullptr.
    /// </returns>
    const MemoryNoteModel* GetNoteContainingOffset(int nOffset) const;

    /// <summary>
    /// Gets the subnote for the specified address.
    /// </summary>
    /// <returns>A pair representing the nested note and its actual address, or an empty pair if not found.</returns>
    Reference GetNoteAtAddress(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Builds a reference chain to the specified address.
    /// </summary>
    /// <returns><c>true</c> if a chain was built, <c>false</c> if the specified address could not be reached from this note.</returns>
    bool GetChainTo(std::vector<Reference>& vChain, ra::data::ByteAddress nSearchAddress) const override;

    /// <summary>
    /// Builds a reference chain to the specified address.
    /// </summary>
    /// <returns><c>true</c> if a chain was built, <c>false</c> if the specified address could not be reached from this note.</returns>
    bool GetChainTo(std::vector<Reference>& vChain, const MemoryNoteModel& pNote) const override;

    /// <summary>
    /// Gets whether or not the note has structured data in its field list.
    /// </summary>
    bool HasNestedStructures() const noexcept { return m_bHasNestedStructures; }

    /// <summary>
    /// Calls the provided callback for each subnote.
    /// </summary>
    /// <remarks>This function is not implicitly recursive. If you want to traverse the children of each match, do so in the callback</remarks>
    void EnumerateOffsetNotes(std::function<bool(const MemoryNoteModel::Reference&)> fCallback, ra::data::ByteAddress nBaseAddress = 0) const;

    virtual void ExtractIndirectNotes(std::wstring_view svParentIndent);

protected:
    virtual bool EnumerateOffsetNotesImpl(ra::data::ByteAddress nBaseAddress, std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const;

    void DetermineIndent(std::wstring_view svParentIndent);
    static void RemoveIndents(std::wstring& sNote, std::wstring_view svIndent);

    ra::data::ByteAddress m_nBaseAddress = 0xFFFFFFFF;            // base address of structured data
    uint32_t m_nOffsetRange = 0;                                  // highest offset captured within the structured data
    uint32_t m_nHeaderLength = 0;                                 // length of note text not associated to OffsetNotes
    bool m_bHasNestedStructures = false;                          // true if there are nested structures
    std::wstring_view m_svIndent;                                 // the substring that should be removed from each
    std::vector<std::unique_ptr<MemoryNoteModel>> m_vOffsetNotes; // subnotes for structured data
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_STRUCTUREDMEMORYNOTEMODEL_H
