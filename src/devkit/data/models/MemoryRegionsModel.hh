#ifndef RA_DATA_MODELS_MEMORYREGIONSMODEL_H
#define RA_DATA_MODELS_MEMORYREGIONSMODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "data/MemoryRegion.hh"

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
    bool Deserialize(ra::util::Tokenizer&) override;

    /// <summary>
    /// Gets the custom regions.
    /// </summary>
    const std::vector<MemoryRegion>& CustomRegions() const noexcept { return m_vRegions; }

    /// <summary>
    /// Clears the custom regions collection.
    /// </summary>
    void ResetCustomRegions();

    /// <summary>
    /// Adds a custom region
    /// </summary>
    void AddCustomRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, const std::wstring& sLabel);

    
    static bool ParseFilterRange(const std::wstring& sRange, ra::data::ByteAddress& nStart, ra::data::ByteAddress& nEnd);

private:
    std::vector<MemoryRegion> m_vRegions;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_MEMORYREGIONSMODEL_H
