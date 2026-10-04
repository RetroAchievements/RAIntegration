#include "PointerMemoryNoteModel.hh"

#include "context/IConsoleContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace models {

void PointerMemoryNoteModel::ExtractIndirectNotes(std::wstring_view svParentIndent)
{
    StructuredMemoryNoteModel::ExtractIndirectNotes(svParentIndent);

    // assume anything annotated as a 32-bit pointer will read a real (non-translated) address and
    // flag it to be converted to an RA address when evaluating indirect notes in DoFrame()
    if (m_nMemSize == Memory::Size::ThirtyTwoBit || m_nMemSize == Memory::Size::ThirtyTwoBitBigEndian)
    {
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto nMaxAddress = pMemoryContext.TotalMemorySize();
        const auto nUnderflowMinAddress = 0xFFFFFFFF - nMaxAddress + 1;

        m_nOffsetType = OffsetType::Converted;

        // if any offset exceeds the available memory for the system, assume the user is leveraging
        // overflow math instead of masking, and don't attempt to translate the addresses.
        for (const auto& pNote : m_vOffsetNotes)
        {
            if (pNote->GetAddress() >= nMaxAddress && pNote->GetAddress() <= nUnderflowMinAddress)
            {
                m_nOffsetType = OffsetType::Overflow;
                break;
            }
        }
    }
}

static ra::data::ByteAddress ConvertPointer(ra::data::ByteAddress nAddress)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    const auto nConvertedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
    if (nConvertedAddress != 0xFFFFFFFF)
        nAddress = nConvertedAddress;

    return nAddress;
}

void PointerMemoryNoteModel::UpdateBaseAddress(ra::data::ByteAddress nAddress, const ra::context::IEmulatorMemoryContext& pMemoryContext,
                                               NoteMovedFunction fNoteMovedCallback)
{
    const auto nOldAddress = m_nBaseAddress;
    ra::data::ByteAddress nNewAddress = 0;

    // Evaluate the pointer to determine the new address.
    m_bPointerRead = true;

    const uint32_t nValue = pMemoryContext.ReadMemory(nAddress, GetMemSize());
    if (nValue == m_nRawPointerValue)
    {
        nNewAddress = m_nBaseAddress;
    }
    else
    {
        m_nRawPointerValue = nValue;

        nNewAddress = (m_nOffsetType == OffsetType::Converted) ? ConvertPointer(nValue) : nValue;

        if (nNewAddress != nOldAddress)
        {
            m_nBaseAddress = nNewAddress;
            if (fNoteMovedCallback)
            {
                for (const auto& pNote : m_vOffsetNotes)
                {
                    // Pointers are handled below.
                    const auto* pStructuredNote = dynamic_cast<StructuredMemoryNoteModel*>(pNote.get());
                    if (pStructuredNote == nullptr)
                        fNoteMovedCallback(nOldAddress + pNote->GetAddress(), nNewAddress + pNote->GetAddress(), *pNote);
                }
            }
        }
    }

    if (m_bHasNestedStructures)
    {
        for (auto& pNote : m_vOffsetNotes)
        {
            auto* pStructuredNote = dynamic_cast<StructuredMemoryNoteModel*>(pNote.get());
            if (pStructuredNote)
                pStructuredNote->UpdateBaseAddress(nNewAddress + pNote->GetAddress(), pMemoryContext, fNoteMovedCallback);
        }
    }
}

const MemoryNoteModel* PointerMemoryNoteModel::GetNoteAtOffset(int nOffset) const
{
    // look for explicit offset match
    for (const auto& pOffsetNote : m_vOffsetNotes)
    {
        if (ra::to_signed(pOffsetNote->GetAddress()) == nOffset)
            return pOffsetNote.get();
    }

    if (m_nOffsetType == OffsetType::Overflow)
    {
        // direct offset not found, look for converted offset
        const auto nConvertedAddress = ConvertPointer(m_nRawPointerValue);
        nOffset += nConvertedAddress - m_nRawPointerValue;

        for (const auto& pOffsetNote : m_vOffsetNotes)
        {
            if (ra::to_signed(pOffsetNote->GetAddress()) == nOffset)
                return pOffsetNote.get();
        }
    }

    return nullptr;
}

bool PointerMemoryNoteModel::GetPreviousAddress(ra::data::ByteAddress nBeforeAddress, ra::data::ByteAddress& nPreviousAddress) const
{
    const auto nPointerAddress = m_nBaseAddress;
    const auto nConvertedAddress = (m_nOffsetType == OffsetType::Overflow)
        ? ConvertPointer(nPointerAddress) : nPointerAddress;

    if (nConvertedAddress > nBeforeAddress)
        return false;

    bool bResult = false;
    nPreviousAddress = 0;
    for (const auto& pOffset : m_vOffsetNotes)
    {
        const auto nOffsetAddress = nPointerAddress + pOffset->GetAddress();
        if (nOffsetAddress < nBeforeAddress && nOffsetAddress > nPreviousAddress)
        {
            nPreviousAddress = nOffsetAddress;
            bResult = true;
        }
    }

    return bResult;
}

bool PointerMemoryNoteModel::GetNextAddress(ra::data::ByteAddress nAfterAddress, ra::data::ByteAddress& nNextAddress) const
{
    const auto nPointerAddress = m_nBaseAddress;
    const auto nConvertedAddress = (m_nOffsetType == OffsetType::Overflow)
        ? ConvertPointer(nPointerAddress) : nPointerAddress;

    if (nConvertedAddress + m_nOffsetRange < nAfterAddress)
        return false;

    bool bResult = false;
    nNextAddress = 0xFFFFFFFF;
    for (const auto& pOffset : m_vOffsetNotes)
    {
        const auto nOffsetAddress = nPointerAddress + pOffset->GetAddress();
        if (nOffsetAddress > nAfterAddress && nOffsetAddress < nNextAddress)
        {
            nNextAddress = nOffsetAddress;
            bResult = true;
        }
    }

    return bResult;
}

class MergedPointerMemoryNoteModel : public PointerMemoryNoteModel
{
public:
    MergedPointerMemoryNoteModel(PointerMemoryNoteModel& pSource) noexcept
        : PointerMemoryNoteModel({}, pSource.GetBytes(), pSource.GetDefaultMemFormat(), pSource.GetMemSize())
    {
        m_nAddress = pSource.GetAddress();
    }

    void AddPart(std::wstring_view svPart)
    {
        m_vParts.push_back(svPart);

        if (m_svNote.empty())
        {
            m_svNote = svPart;
        }
        else
        {
            const auto* pPartBegin = &svPart.front();
            const auto* pPartEnd = &svPart.back();
            const auto* pNoteBegin = &m_svNote.front();
            const auto* pNoteEnd = &m_svNote.back();
            if (pPartBegin < pNoteBegin)
                m_svNote = std::wstring_view(pPartBegin, pNoteEnd - pPartBegin);
            else
                m_svNote = std::wstring_view(pNoteBegin, pPartEnd - pNoteBegin);
        }
    }

    std::wstring GetNote() const override
    {
        std::wstring sNote;

        for (const auto svPart : m_vParts)
            sNote.append(svPart);

        RemoveIndents(sNote, m_svIndent);
        return sNote;
    }

private:
    std::vector<std::wstring_view> m_vParts;
};

void PointerMemoryNoteModel::MergeInto(PointerMemoryNoteModel& pOther)
{
    auto* pMergedOther = dynamic_cast<MergedPointerMemoryNoteModel*>(&pOther);
    Expects(pMergedOther != nullptr);
    pMergedOther->AddPart(m_svNote);

    pOther.m_bHasNestedStructures |= m_bHasNestedStructures;
    pOther.m_nOffsetRange = std::max(pOther.m_nOffsetRange, m_nOffsetRange);
    pOther.m_svIndent = m_svIndent;

    for (auto& pOffsetNote : m_vOffsetNotes)
    {
        const auto nAddress = pOffsetNote->GetAddress();

        std::unique_ptr<MemoryNoteModel>* pExistingNote = nullptr;
        for (auto& pOtherNote : pOther.m_vOffsetNotes)
        {
            if (pOtherNote->GetAddress() == nAddress)
            {
                pExistingNote = &pOffsetNote;
                break;
            }
        }

        if (pExistingNote != nullptr)
        {
            // Can only merge if both notes are pointer notes
            if ((*pExistingNote)->GetType() == MemoryNoteType::Pointer
                && pOffsetNote->GetType() == MemoryNoteType::Pointer)
            {
                auto* pOffsetPointerNote = dynamic_cast<PointerMemoryNoteModel*>(pOffsetNote.get());
                Expects(pOffsetPointerNote != nullptr);

                auto* pExistingPointerNote = dynamic_cast<PointerMemoryNoteModel*>(pExistingNote->get());
                Expects(pExistingPointerNote != nullptr);

                auto* pMerged = PointerMemoryNoteModel::Merge(*pExistingPointerNote, *pOffsetPointerNote);
                if (pExistingNote->get() != pMerged)
                    pExistingNote->reset(pMerged);
            }
        }
        else
        {
            pOther.m_vOffsetNotes.push_back(std::move(pOffsetNote));
        }
    }

    m_vOffsetNotes.clear();
}

PointerMemoryNoteModel* PointerMemoryNoteModel::Merge(PointerMemoryNoteModel& pFirst, PointerMemoryNoteModel& pSecond)
{
    auto* pMerged = dynamic_cast<MergedPointerMemoryNoteModel*>(&pFirst);
    if (!pMerged)
    {
        pMerged = new MergedPointerMemoryNoteModel(pFirst);
        pFirst.MergeInto(*pMerged);
    }

    pSecond.MergeInto(*pMerged);
    return pMerged;
}

bool PointerMemoryNoteModel::EnumerateOffsetNotesImpl(ra::data::ByteAddress,
    std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const
{
    if (m_nOffsetType == OffsetType::Overflow)
        return StructuredMemoryNoteModel::EnumerateOffsetNotesImpl(m_nRawPointerValue, fCallback);
    else
        return StructuredMemoryNoteModel::EnumerateOffsetNotesImpl(m_nBaseAddress, fCallback);
}

bool PointerMemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vMatches, ra::data::ByteAddress nSearchAddress) const
{
    const auto nIndex = vMatches.size();
    const auto nBaseAddress = vMatches.empty() ? 0 : vMatches.back().nAddress;
    vMatches.emplace_back(this, m_nBaseAddress, 0);

    // Assume null is not a valid pointer address
    const bool bResult = m_nRawPointerValue == 0 ? false :
        StructuredMemoryNoteModel::GetChainTo(vMatches, nSearchAddress);

    // The address of the pointer node was the pointer value to assist in finding the child notes.
    // Change it back to the address of the pointer now.
    vMatches.at(nIndex).nAddress = nBaseAddress + m_nAddress;

    // Did we match a nested note?
    if (bResult)
        return true;

    // Did we match the pointer note itself?
    if (nSearchAddress >= nBaseAddress + m_nAddress && nSearchAddress < nBaseAddress + m_nAddress + m_nBytes)
        return true;

    vMatches.pop_back();
    return false;
}

} // namespace models
} // namespace data
} // namespace ra
