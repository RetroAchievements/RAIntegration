#ifndef RA_DATA_MODELS_POINTERMEMORYNOTEMODEL_H
#define RA_DATA_MODELS_POINTERMEMORYNOTEMODEL_H
#pragma once

#include "StructuredMemoryNoteModel.hh"

#include "context/IEmulatorMemoryContext.hh"

#include <functional>

namespace ra {
namespace data {
namespace models {

class PointerMemoryNoteModel : public StructuredMemoryNoteModel
{
public:
    PointerMemoryNoteModel(std::wstring_view svNote, uint32_t nBytes, Memory::Format nMemFormat, Memory::Size nMemSize) noexcept
        : StructuredMemoryNoteModel(svNote, nBytes, nMemFormat, nMemSize, MemoryNoteType::Pointer)
    {
    }
    ~PointerMemoryNoteModel() = default;
    PointerMemoryNoteModel(const PointerMemoryNoteModel&) noexcept = delete;
    PointerMemoryNoteModel& operator=(const PointerMemoryNoteModel&) noexcept = delete;
    PointerMemoryNoteModel(PointerMemoryNoteModel&&) noexcept = default;
    PointerMemoryNoteModel& operator=(PointerMemoryNoteModel&&) noexcept = default;

    /// <summary>
    /// Gets whether or not the RawPointerValue has been set.
    /// </summary>
    bool HasRawPointerValue() const noexcept { return m_bPointerRead; }

    /// <summary>
    /// Gets the raw pointer value.
    /// </summary>
    uint32_t GetRawPointerValue() const noexcept { return m_nRawPointerValue; }

    typedef std::function<void(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const MemoryNoteModel&)> NoteMovedFunction;
    /// <summary>
    /// Updates the raw pointer value by reading from memory.
    /// </summary>
    /// <param name="nAddress">The address of the pointer data. For root pointers, this will be the note's address. For nested pointers, it will be the note's offset + the parent pointer's value.</param>
    /// <param name="pMemoryContext">Where to read the new value from.</param>
    /// <param name="fNoteMovedCallback">Function to call if the PointerAddress changes.</param>
    void UpdateRawPointerValue(ra::data::ByteAddress nAddress, const ra::context::IEmulatorMemoryContext& pMemoryContext, NoteMovedFunction fNoteMovedCallback);

    /// <summary>
    /// Get the subnote for the field at the specified offset.
    /// </summary>
    /// <returns>Requested subnote, <c>null</c> if not found.</returns>
    const MemoryNoteModel* GetNoteAtOffset(int nOffset) const override;

    /// <summary>
    /// Gets the subnote for the specified address.
    /// </summary>
    /// <returns>A pair representing the nested note and its actual address, or an empty pair if not found.</returns>
    bool GetChainTo(std::vector<MemoryNoteModel::Reference>& vMatches, ra::data::ByteAddress nSearchAddress) const override;

    /// <summary>
    /// Gets the address of the closest subnote before the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    bool GetPreviousAddress(ra::data::ByteAddress nBeforeAddress, ra::data::ByteAddress& nPreviousAddress) const override;

    /// <summary>
    /// Gets the address of the closest subnote after the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    bool GetNextAddress(ra::data::ByteAddress nAfterAddress, ra::data::ByteAddress& nNextAddress) const override;

    void ExtractIndirectNotes(std::wstring_view svParentIndent) override;

    /// <summary>
    /// Merges the offset notes from <paramref name="pFirst" /> and <paramref name="pSecond" /> into a single note.
    /// </summary>
    static PointerMemoryNoteModel* Merge(PointerMemoryNoteModel& pFirst, PointerMemoryNoteModel& pSecond);

    void MergeInto(PointerMemoryNoteModel& pOther);

private:
    bool EnumerateOffsetNotesImpl(ra::data::ByteAddress nBaseAddress, std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const override;

    uint32_t m_nRawPointerValue = 0xFFFFFFFF;             // last raw value of pointer captured
    bool m_bPointerRead = false;                          // true the first time RawPointerValue is updated

    enum OffsetType : uint8_t
    {
        None = 0,
        Converted, // PointerAddress will contain a converted address
        Overflow,  // offset exceeds RA address space, apply to RawPointerValue
    };
    OffsetType m_nOffsetType = OffsetType::None;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_POINTERMEMORYNOTEMODEL_H
