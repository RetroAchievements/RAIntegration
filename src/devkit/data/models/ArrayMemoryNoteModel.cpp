#include "ArrayMemoryNoteModel.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace models {

const MemoryNoteModel* ArrayMemoryNoteModel::GetNoteAtOffset(int nOffset) const noexcept
{
    if (nOffset >= ra::to_signed(m_nBytes))
        return nullptr;

    nOffset %= m_nBytesPerElement;

    for (const auto& pOffsetNote : m_vOffsetNotes)
    {
        if (ra::to_signed(pOffsetNote->GetAddress()) == nOffset)
            return pOffsetNote.get();
    }

    return nullptr;
}

bool ArrayMemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nSearchAddress) const
{
    const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
    if (nSearchAddress < nBaseAddress + m_nAddress)
        return false;

    const auto nOffset = nSearchAddress - nBaseAddress - m_nAddress;
    if (nOffset >= m_nBytes)
        return false;

    const auto nElementIndex = nOffset / m_nBytesPerElement;

    // Include the offset to the element so base address calculations in the child will be correct.
    const auto nIndex = vChain.size();
    vChain.emplace_back(this, nBaseAddress + m_nAddress + nElementIndex * m_nBytesPerElement, 0);

    if (StructuredMemoryNoteModel::GetChainTo(vChain, nSearchAddress))
    {
        // Adjust the address back to the start of the array.
        auto& pReference = vChain.at(nIndex);
        pReference.nAddress = nBaseAddress + m_nAddress;
        pReference.nElementIndex = nElementIndex;
    }

    return true;
}

bool ArrayMemoryNoteModel::EnumerateOffsetNotesImpl(ra::data::ByteAddress nBaseAddress,
    std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const
{
    for (uint32_t nOffset = 0; nOffset < m_nBytes; nOffset += m_nBytesPerElement)
    {
        if (!StructuredMemoryNoteModel::EnumerateOffsetNotesImpl(nBaseAddress + nOffset, fCallback))
            return false;
    }

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
