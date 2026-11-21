#include "ArrayMemoryNoteModel.hh"

#include "PointerMemoryNoteModel.hh"
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

bool ArrayMemoryNoteModel::GetNestedChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nBaseAddress, ra::data::ByteAddress nSearchAddress) const
{
    if (!m_bHasNestedStructures)
        return false;

    const auto nElementCount = GetElementCount();

    nBaseAddress += m_nAddress;
    vChain.emplace_back(this, nBaseAddress, 0);

    for (const auto& pMemoryNote : m_vOffsetNotes)
    {
        const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote.get());
        if (!pStructuredNote)
            continue;

        const auto nIndex = vChain.size();
        vChain.emplace_back(pStructuredNote, nBaseAddress, 0);

        const auto* pPointerNote = dynamic_cast<const PointerMemoryNoteModel*>(pMemoryNote.get());
        if (pPointerNote)
        {
            // If it's a pointer note, evaluate the pointers and recurse.
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();

            for (uint32_t nElementIndex = 0; nElementIndex < nElementCount; ++nElementIndex)
            {
                uint32_t nPointerAddress = pMemoryContext.ReadMemory(nBaseAddress, GetMemSize());
                if (nPointerAddress != 0)
                {
                    nPointerAddress = pPointerNote->RawPointerToByteAddress(nPointerAddress);

                    if (nSearchAddress >= nPointerAddress)
                    {
                        const auto nOffset = nSearchAddress - nPointerAddress;
                        const auto* pOffsetNote = pPointerNote->GetNoteAtOffset(nOffset);
                        if (pOffsetNote)
                        {
                            vChain.emplace_back(pOffsetNote, nPointerAddress + pOffsetNote->GetAddress(), 0);
                            return true;
                        }
                    }

                    if (pPointerNote->HasNestedStructures())
                    {
                        bool bFound = false;
                        pPointerNote->EnumerateOffsetNotes([&bFound, &vChain, nSearchAddress]
                            (const MemoryNoteModel& pOffsetNote)
                            {
                                const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(&pOffsetNote);
                                if (pStructuredNote && pStructuredNote->GetChainTo(vChain, nSearchAddress))
                                {
                                    bFound = true;
                                    return false;
                                }

                                return true;
                            });

                        if (bFound)
                            return true;
                    }
                }

                nBaseAddress += m_nBytesPerElement;
                vChain.at(nIndex).nAddress = nBaseAddress;
                vChain.at(nIndex).nElementIndex++;
            }
        }
        else
        {
            for (uint32_t nElementIndex = 0; nElementIndex < nElementCount; ++nElementIndex)
            {
                if (pStructuredNote->GetChainTo(vChain, nSearchAddress))
                    return true;

                vChain.at(nIndex).nAddress += m_nBytesPerElement;
                vChain.at(nIndex).nElementIndex++;
            }
        }

        vChain.pop_back();
    }

    vChain.pop_back();
    return false;
}

bool ArrayMemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nSearchAddress) const
{
    const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
    if (nSearchAddress < nBaseAddress + m_nAddress)
        return GetNestedChainTo(vChain, nBaseAddress, nSearchAddress);

    const auto nOffset = nSearchAddress - nBaseAddress - m_nAddress;
    if (nOffset >= m_nBytes)
        return GetNestedChainTo(vChain, nBaseAddress, nSearchAddress);

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
