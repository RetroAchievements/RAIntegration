#include "MemoryRegionsModel.hh"

#include "context/IEmulatorMemoryContext.hh"

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
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
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

        pWriter.Write(pMemoryContext.FormatAddress(pIter.GetStartAddress()));
        pWriter.Write("-");
        pWriter.Write(pMemoryContext.FormatAddress(pIter.GetEndAddress()));
        WriteQuoted(pWriter, pIter.GetDescription());
    }
}

bool MemoryRegionsModel::Deserialize(ra::util::Tokenizer& pTokenizer)
{
    const auto sStartAddress = pTokenizer.ReadTo('-');
    pTokenizer.Consume('-');
    const auto nStartAddress = Memory::ParseAddress(sStartAddress);

    const auto sEndAddress = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    const auto nEndAddress = Memory::ParseAddress(sEndAddress);

    std::wstring sDescription;
    if (!ReadQuoted(pTokenizer, sDescription))
        return false;

    AddCustomRegion(nStartAddress, nEndAddress, sDescription);
    return true;
}

void MemoryRegionsModel::AddCustomRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, const std::wstring& sLabel)
{
    m_vRegions.emplace_back(nStartAddress, nEndAddress, sLabel);

    // setting changes to Unpublished forces a write without attempting to consolidate modifications
    SetValue(ChangesProperty, ra::etoi(AssetChanges::Unpublished));
}

void MemoryRegionsModel::ResetCustomRegions()
{
    m_vRegions.clear();

    // nothing should be written if custom regions is empty
    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

bool MemoryRegionsModel::ParseFilterRange(const std::wstring& sRange, ra::data::ByteAddress& nStart, ra::data::ByteAddress& nEnd)
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    const auto nMax = gsl::narrow_cast<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize()) - 1;

    if (sRange.empty())
    {
        // no range specified, search all
        nStart = 0;
        nEnd = nMax;
        return true;
    }

    if (!Memory::TryParseAddressRange(sRange, nStart, nEnd))
        return false;

    if (nStart > nMax)
        return false;

    if (nEnd > nMax)
        nEnd = nMax;

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
