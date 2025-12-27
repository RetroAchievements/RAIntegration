#include "MemoryRegionsModel.hh"

#include "RA_Defs.h"

#include "data/context/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace data {
namespace models {

MemoryRegionsModel::MemoryRegionsModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::MemoryRegions));
    GSL_SUPPRESS_F6 SetName(L"Memory Regions");
}

void MemoryRegionsModel::Serialize(ra::services::TextWriter& pWriter) const
{
    bool first = true;

    for (const auto& pIter : m_vRegions)
    {
        // "M0" of first region will be written by GameAssets
        if (first)
        {
            first = false;
            pWriter.Write(":");
        }
        else
        {
            pWriter.WriteLine();
            pWriter.Write("M0:");
        }

        pWriter.Write(ra::ByteAddressToString(pIter.nStartAddress));
        pWriter.Write("-");
        pWriter.Write(ra::ByteAddressToString(pIter.nEndAddress));
        WriteQuoted(pWriter, pIter.sLabel);
    }
}

bool MemoryRegionsModel::Deserialize(ra::Tokenizer& pTokenizer)
{
    const auto sStartAddress = pTokenizer.ReadTo('-');
    pTokenizer.Consume('-');
    const auto nStartAddress = ra::ByteAddressFromString(sStartAddress);

    const auto sEndAddress = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    const auto nEndAddress = ra::ByteAddressFromString(sEndAddress);

    std::wstring sDescription;
    if (!ReadQuoted(pTokenizer, sDescription))
        return false;

    AddCustomRegion(nStartAddress, nEndAddress, sDescription);
    return true;
}

void MemoryRegionsModel::AddCustomRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, const std::wstring& sLabel)
{
    MemoryRegion pRegion;
    pRegion.sLabel = sLabel;
    pRegion.nStartAddress = nStartAddress;
    pRegion.nEndAddress = nEndAddress;

    m_vRegions.push_back(pRegion);

    // setting changes to Unpublished forces a write without attempting to consolidate modifications
    SetValue(ChangesProperty, ra::etoi(AssetChanges::Unpublished));
}

void MemoryRegionsModel::ResetCustomRegions()
{
    m_vRegions.clear();

    // nothing should be written if custom regions is empty
    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

static bool ParseAddress(const wchar_t** pointer, ra::data::ByteAddress& address) noexcept
{
    if (pointer == nullptr)
        return false;

    const wchar_t* ptr = *pointer;
    if (ptr == nullptr)
        return false;

    if (*ptr == '$')
    {
        ++ptr;
    }
    else if (ptr[0] == '0' && ptr[1] == 'x')
    {
        ptr += 2;
    }

    const wchar_t* start = ptr;
    address = 0;
    while (*ptr)
    {
        if (*ptr >= '0' && *ptr <= '9')
        {
            address <<= 4;
            address += (*ptr - '0');
        }
        else if (*ptr >= 'a' && *ptr <= 'f')
        {
            address <<= 4;
            address += (*ptr - 'a' + 10);
        }
        else if (*ptr >= 'A' && *ptr <= 'F')
        {
            address <<= 4;
            address += (*ptr - 'A' + 10);
        }
        else
        {
            break;
        }

        ++ptr;
    }

    if (ptr == start)
        return false;

    *pointer = ptr;
    return true;
}

bool MemoryRegionsModel::ParseFilterRange(const std::wstring& sRange, _Out_ ra::data::ByteAddress& nStart, _Out_ ra::data::ByteAddress& nEnd)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nMax = gsl::narrow_cast<ra::data::ByteAddress>(pEmulatorContext.TotalMemorySize()) - 1;

    if (sRange.empty())
    {
        // no range specified, search all
        nStart = 0;
        nEnd = nMax;
        return true;
    }

    const wchar_t* ptr = sRange.data();
    if (!ptr || !ParseAddress(&ptr, nStart))
    {
        nStart = 0;
        nEnd = 0;
        return false;
    }

    while (iswspace(*ptr))
        ++ptr;

    if (*ptr == '\0')
    {
        nEnd = nStart;
        return true;
    }

    if (*ptr != '-')
    {
        nEnd = 0;
        return false;
    }

    ++ptr;
    while (iswspace(*ptr))
        ++ptr;

    if (!ParseAddress(&ptr, nEnd))
    {
        nEnd = 0;
        return false;
    }

    if (*ptr != '\0')
        return false;

    if (nStart > nMax)
        return false;

    if (nEnd > nMax)
        nEnd = nMax;
    else if (nEnd < nStart)
        std::swap(nStart, nEnd);

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
