#include "StructuredMemoryNoteModel.hh"

#include "PointerMemoryNoteModel.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace models {

std::wstring StructuredMemoryNoteModel::GetNote() const
{
    std::wstring sNote(m_svNote);
    RemoveIndents(sNote, m_svIndent);
    return sNote;
}

void StructuredMemoryNoteModel::RemoveIndents(std::wstring& sNote, std::wstring_view svIndent)
{
    if (svIndent.length() < 3)
        return;

    auto nIndex = sNote.rfind(svIndent);
    if (nIndex == std::wstring::npos)
        return;

    do
    {
        // Keep the newline and last character (either plus or pipe).
        auto nRemoveAt = nIndex + 1;
        auto nToRemove = svIndent.length() - 2;

        if (nIndex == 0)
        {
            // The leading newline indicates a blank summary. Don't output the blank summary or newline.
            nRemoveAt = nIndex;
            nToRemove++;
        }
        else if (nIndex == 1 && sNote.front() == '\r')
        {
            // The leading newline indicates a blank summary. Don't output the blank summary or newline.
            nRemoveAt = 0;
            nToRemove += 2;
        }

        sNote.erase(nRemoveAt, nToRemove);

        if (nRemoveAt == 0)
            break;

        nIndex = sNote.rfind(svIndent, nIndex - 1);
    } while (nIndex != std::wstring::npos);
}

void StructuredMemoryNoteModel::DetermineIndent(std::wstring_view svParentIndent)
{
    size_t nIndex = 0;
    size_t nPrefix = 0;
    wchar_t c = 0;

    // A child indent is at least as many non-alphanumeric characters at the start of a line as the
    // parent followed by a plus (pointer offset) or pipe (local offset) and then a numerical value.
    do
    {
        // Find a new line
        auto nNextIndex = m_svNote.find('\n', nIndex);
        if (nNextIndex == std::wstring::npos)
            return;

        // Count characters until we have at least as many as the parent
        nPrefix = svParentIndent.length() - 1;

        nIndex = nNextIndex + 1;
        while (nIndex < m_svNote.size() && !ra::util::String::IsAlNum(c = m_svNote.at(nIndex)))
        {
            if (c == '\n')
            {
                nNextIndex = nIndex;
                nPrefix = svParentIndent.length() - 1;
            }
            else if (nPrefix > 0)
            {
                --nPrefix;
            }
            else if (c == '+' || c == '|')
            {
                // Found a plus or pipe. If the next character is numeric, we have the child indent.
                ++nIndex;
                if (nIndex < m_svNote.size() && ra::util::String::IsDigit(m_svNote.at(nIndex)))
                {
                    m_nHeaderLength = gsl::narrow_cast<unsigned int>(nNextIndex);
                    while (m_nHeaderLength > 0 && ra::util::String::IsSpace(m_svNote.at(m_nHeaderLength - 1)))
                        --m_nHeaderLength;

                    m_svIndent = m_svNote.substr(nNextIndex, nIndex - nNextIndex);
                    return;
                }
            }

            ++nIndex;
        }
    } while (true);
}

GSL_SUPPRESS_R30 // left has to be a const ref to the unique_ptr because the function is used in lower_bound
GSL_SUPPRESS_R32 // left has to be a const ref to the unique_ptr because the function is used in lower_bound
static int CompareNoteAddresses(const std::unique_ptr<MemoryNoteModel>& left,
    ra::data::ByteAddress nAddress) noexcept
{
    return left->GetAddress() < nAddress;
}

void StructuredMemoryNoteModel::ExtractIndirectNotes(std::wstring_view svParentIndent)
{
    DetermineIndent(svParentIndent);
    if (m_svIndent.empty())
        return;

    // Skip over the first indent marker
    auto nIndex = m_svNote.find(m_svIndent, m_nHeaderLength) + m_svIndent.length();
    do
    {
        // The next indirect note starts when we find the next indent marker
        auto nNextIndex = m_svNote.find(m_svIndent, nIndex);
        auto nStopIndex = nNextIndex;

        if (nNextIndex != std::wstring::npos)
        {
            // Additional non-digit characters after the indent marker are a deeper indent. Ignore them for now.
            //
            //   [32-bit pointer] global data
            //   +0x20 [32-bit pointer] user data
            //   ++0x08 [16-bit] points
            //
            while (nNextIndex + m_svIndent.length() < m_svNote.length() &&
                !ra::util::String::IsDigit(m_svNote.at(nNextIndex + m_svIndent.length())))
            {
                nNextIndex = nStopIndex = m_svNote.find(m_svIndent, nNextIndex + m_svIndent.length());
                if (nNextIndex == std::wstring::npos)
                    break;
            }

            // Remove trailing whitespace
            if (nStopIndex != std::wstring::npos)
            {
                while (nStopIndex > 0)
                {
                    const wchar_t c = m_svNote.at(nStopIndex - 1);
                    if (!ra::util::String::IsSpace(c))
                        break;
                    --nStopIndex;
                }
            }
        }

        const auto svNextNote = m_svNote.substr(nIndex, nStopIndex - nIndex);

        // Extract the offset
        wchar_t* pEnd = nullptr;

        int nOffset = 0;
        try
        {
            if (svNextNote.length() > 2 && svNextNote.at(1) == 'x')
                nOffset = gsl::narrow_cast<int>(std::wcstoll(&svNextNote.at(2), &pEnd, 16));
            else
                nOffset = gsl::narrow_cast<int>(std::wcstoll(&svNextNote.at(0), &pEnd, 10));
        }
        catch (const std::exception&)
        {
            break;
        }

        // If there are any error processing offsets, don't treat this as a pointer note
        if (!pEnd || ra::util::String::IsAlNum(*pEnd))
        {
            m_vOffsetNotes.clear();
            return;
        }

        // Skip over [whitespace] [optional separator] [whitespace]
        const wchar_t* pStop = &svNextNote.at(0) + svNextNote.length();
        while (pEnd < pStop && ra::util::String::IsSpace(*pEnd) && *pEnd != '\n')
            ++pEnd;

        if (pEnd < pStop)
        {
            if (*pEnd == '\n')
            {
                // No separator. Found an unannotated note.
                // Keep the newline as it's part of the indent marker and replicates an empty summary.
                if (pEnd[-1] == '\r')
                    --pEnd;
            }
            else if (!ra::util::String::IsAlNum(*pEnd) && *pEnd != '[' && *pEnd != '(') // assume brackets are not a separator
            {
                // Found a separator. Skip it and any following whitespace.
                ++pEnd;

                while (pEnd < pStop && ra::util::String::IsSpace(*pEnd))
                    ++pEnd;
            }
        }
        const auto svNoteBody = svNextNote.substr(pEnd - &svNextNote.at(0));
        const auto nAddress = gsl::narrow_cast<ra::data::ByteAddress>(nOffset);

        // Create or merge the child note
        auto pIter = std::lower_bound(m_vOffsetNotes.begin(), m_vOffsetNotes.end(), nAddress, CompareNoteAddresses);
        if (pIter != m_vOffsetNotes.end() && (*pIter)->GetAddress() == nAddress)
        {
            // Can only merge if both notes are pointer notes
            if ((*pIter)->GetType() == MemoryNoteType::Pointer)
            {
                auto pOffsetNote = MemoryNoteModel::ParseOffsetNote(svNoteBody, m_svIndent);
                if (pOffsetNote->GetType() == MemoryNoteType::Pointer)
                {
                    auto* pOffsetPointerNote = dynamic_cast<PointerMemoryNoteModel*>(pOffsetNote.get());
                    Expects(pOffsetPointerNote != nullptr);

                    auto* pExistingPointerNote = dynamic_cast<PointerMemoryNoteModel*>(pIter->get());
                    Expects(pExistingPointerNote != nullptr);

                    auto* pMerged = PointerMemoryNoteModel::Merge(*pExistingPointerNote, *pOffsetPointerNote);
                    if (pIter->get() != pMerged)
                        pIter->reset(pMerged);
                }
            }
        }
        else
        {
            auto pOffsetNote = MemoryNoteModel::ParseOffsetNote(svNoteBody, m_svIndent);
            pOffsetNote->SetAddress(nAddress);

            m_bHasNestedStructures |= dynamic_cast<StructuredMemoryNoteModel*>(pOffsetNote.get()) != nullptr;

            const auto nRangeOffset = nOffset + pOffsetNote->GetBytes();
            m_nOffsetRange = std::max(m_nOffsetRange, nRangeOffset);

            m_vOffsetNotes.insert(pIter, std::move(pOffsetNote));
        }

        if (nNextIndex == std::string::npos)
            break;

        nIndex = nNextIndex + m_svIndent.length();
    } while (true);
}

void StructuredMemoryNoteModel::EnumerateOffsetNotes(
    std::function<bool(const MemoryNoteModel::Reference&)> fCallback, ra::data::ByteAddress nBaseAddress) const
{
    EnumerateOffsetNotesImpl(nBaseAddress, fCallback);
}

bool StructuredMemoryNoteModel::EnumerateOffsetNotesImpl(ra::data::ByteAddress nBaseAddress,
    std::function<bool(const MemoryNoteModel::Reference&)> fCallback) const
{
    for (const auto& pNote : m_vOffsetNotes)
    {
        const auto nNoteAddress = nBaseAddress + pNote->GetAddress();
        const MemoryNoteModel::Reference pMatch(pNote.get(), nNoteAddress, 0);
        if (!fCallback(pMatch))
            return false;
    }

    return true;
}

MemoryNoteModel::Reference StructuredMemoryNoteModel::GetNoteAtAddress(ra::data::ByteAddress nAddress) const
{
    std::vector<Reference> vMatches;
    if (GetChainTo(vMatches, nAddress) && vMatches.back().nAddress == nAddress)
        return vMatches.back();

    return {};
}

const MemoryNoteModel* StructuredMemoryNoteModel::GetNoteAtOffset(int nOffset) const
{
    // look for explicit offset match
    auto pIter = std::lower_bound(m_vOffsetNotes.begin(), m_vOffsetNotes.end(), ra::to_unsigned(nOffset), CompareNoteAddresses);
    if (pIter != m_vOffsetNotes.end() && (*pIter)->GetAddress() == ra::to_unsigned(nOffset))
        return pIter->get();

    return nullptr;
}

const MemoryNoteModel* StructuredMemoryNoteModel::GetNoteContainingOffset(int nOffset) const
{
    const auto nAddress = ra::to_unsigned(nOffset);
    auto pIter = std::lower_bound(m_vOffsetNotes.begin(), m_vOffsetNotes.end(), nAddress, CompareNoteAddresses);

    // exact match, return it
    if (pIter != m_vOffsetNotes.end() && (*pIter)->GetAddress() == nAddress)
        return pIter->get();

    // lower_bound returns the first item _after_ the search value. scan all items before
    // the found item to see if any of them contain the target address. have to scan
    // all items because a singular note may exist within a range.
    if (pIter != m_vOffsetNotes.begin())
    {
        do
        {
            --pIter;
            const auto* pMemoryNote = pIter->get();

            if (pMemoryNote && pMemoryNote->GetBytes() > 1 && pMemoryNote->GetBytes() + pMemoryNote->GetAddress() > nAddress)
                return pMemoryNote;

        } while (pIter != m_vOffsetNotes.begin());
    }

    return nullptr;
}

bool StructuredMemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nSearchAddress) const
{
    const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
    const auto nOffset = ra::to_signed(nSearchAddress - nBaseAddress);

    // check for direct note
    const auto* pNote = GetNoteContainingOffset(nOffset);
    if (pNote)
        return pNote->GetChainTo(vChain, nSearchAddress);

    // also check for derived memory notes
    if (m_bHasNestedStructures)
    {
        for (const auto& pMemoryNote : m_vOffsetNotes)
        {
            const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote.get());
            if (pStructuredNote && pStructuredNote->GetChainTo(vChain, nSearchAddress))
                return true;
        }
    }

    return false;
}

bool StructuredMemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, const MemoryNoteModel& pNote) const
{
    if (MemoryNoteModel::GetChainTo(vChain, pNote))
        return true;

    if (!IsAncestorOf(pNote))
        return false;

    const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
    for (const auto& pOffsetNote : m_vOffsetNotes)
    {
        if (pOffsetNote->IsAncestorOf(pNote))
        {
            vChain.emplace_back(this, nBaseAddress + m_nAddress, 0);
            if (pOffsetNote->GetChainTo(vChain, pNote))
                return true;

            vChain.pop_back();
        }
    }

    return false;
}

void StructuredMemoryNoteModel::UpdateBaseAddress(ra::data::ByteAddress nAddress, const ra::context::IEmulatorMemoryContext& pMemoryContext, NoteMovedFunction fNoteMovedCallback)
{
    for (const auto& pOffsetNote : m_vOffsetNotes)
    {
        auto* pStructuredNote = dynamic_cast<StructuredMemoryNoteModel*>(pOffsetNote.get());
        if (pStructuredNote)
            pStructuredNote->UpdateBaseAddress(nAddress + pOffsetNote->GetAddress(), pMemoryContext, fNoteMovedCallback);
    }

    if (m_nBaseAddress != nAddress)
    {
        if (fNoteMovedCallback)
            fNoteMovedCallback(m_nBaseAddress, nAddress, *this);

        m_nBaseAddress = nAddress;
    }
}

} // namespace models
} // namespace data
} // namespace ra
