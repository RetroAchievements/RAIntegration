#ifndef RA_DATA_MODELS_ARRAYMEMORYNOTEMODEL_H
#define RA_DATA_MODELS_ARRAYMEMORYNOTEMODEL_H
#pragma once

#include "StructuredMemoryNoteModel.hh"

namespace ra {
namespace data {
namespace models {

class ArrayMemoryNoteModel : public StructuredMemoryNoteModel
{
public:
    ArrayMemoryNoteModel(std::wstring_view svNote, uint32_t nBytes, uint32_t nBytesPerElement) noexcept
        : StructuredMemoryNoteModel(svNote, nBytes, Memory::Format::Unknown, Memory::Size::Array, MemoryNoteType::Array),
          m_nBytesPerElement(nBytesPerElement ? nBytesPerElement : nBytes)
    {
    }
    ~ArrayMemoryNoteModel() = default;
    ArrayMemoryNoteModel(const ArrayMemoryNoteModel&) noexcept = delete;
    ArrayMemoryNoteModel& operator=(const ArrayMemoryNoteModel&) noexcept = delete;
    ArrayMemoryNoteModel(ArrayMemoryNoteModel&&) noexcept = default;
    ArrayMemoryNoteModel& operator=(ArrayMemoryNoteModel&&) noexcept = default;

    /// <summary>
    /// Gets the number of bytes for each element of the array.
    /// </summary>
    uint32_t GetElementSize() const noexcept { return m_nBytesPerElement; }

    /// <summary>
    /// Gets the number of elements in the array.
    /// </summary>
    uint32_t GetElementCount() const noexcept { return m_nBytesPerElement ? m_nBytes / m_nBytesPerElement : 0; }

    /// <summary>
    /// Get the subnote for the field at the specified offset.
    /// </summary>
    /// <returns>Requested subnote, <c>null</c> if not found.</returns>
    const MemoryNoteModel* GetNoteAtOffset(int nOffset) const noexcept override;

    /// <summary>
    /// Gets the subnote for the specified address.
    /// </summary>
    /// <returns>A pair representing the nested note and its actual address, or an empty pair if not found.</returns>
    bool GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nSearchAddress) const override;

private:
    bool EnumerateOffsetNotesImpl(ra::data::ByteAddress nBaseAddress, std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const override;

    uint32_t m_nBytesPerElement = 0;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_ARRAYMEMORYNOTEMODEL_H
