#ifndef RA_DATA_MEMORY_REGIONS_MODEL_H
#define RA_DATA_MEMORY_REGIONS_MODEL_H
#pragma once

#include "AssetModelBase.hh"

#include "data/Types.hh"

namespace ra {
namespace data {
namespace models {

class MemoryRegionsModel : public AssetModelBase
{
public:
    MemoryRegionsModel() noexcept;
	~MemoryRegionsModel() = default;
    MemoryRegionsModel(const MemoryRegionsModel&) noexcept = delete;
    MemoryRegionsModel& operator=(const MemoryRegionsModel&) noexcept = delete;
    MemoryRegionsModel(MemoryRegionsModel&&) noexcept = delete;
    MemoryRegionsModel& operator=(MemoryRegionsModel&&) noexcept = delete;

    bool IsShownInList() const noexcept override { return false; }

	void Serialize(ra::services::TextWriter&) const override;
    bool Deserialize(ra::Tokenizer&) override;

    struct MemoryRegion
    {
        std::wstring sLabel;
        ra::ByteAddress nStartAddress = 0;
        ra::ByteAddress nEndAddress = 0;
    };

    const std::vector<MemoryRegion>& CustomRegions() const noexcept { return m_vRegions; }

    void ResetCustomRegions();
    void AddCustomRegion(ra::ByteAddress nStartAddress, ra::ByteAddress nEndAddress, const std::wstring& sLabel);

    static bool ParseFilterRange(const std::wstring& sRange, _Out_ ra::ByteAddress& nStart, _Out_ ra::ByteAddress& nEnd);

protected:

private:
    std::vector<MemoryRegion> m_vRegions;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MEMORY_REGIONS_MODEL_H
