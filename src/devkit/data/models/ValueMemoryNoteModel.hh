#ifndef RA_DATA_MODELS_VALUEMEMORYNOTEMODEL_H
#define RA_DATA_MODELS_VALUEMEMORYNOTEMODEL_H
#pragma once

#include "MemoryNoteModel.hh"

#include <functional>

namespace ra {
namespace data {
namespace models {

class ValueMemoryNoteModel : public MemoryNoteModel
{
public:
    ValueMemoryNoteModel(std::wstring_view svNote, uint32_t nBytes, Memory::Format nMemFormat, Memory::Size nMemSize) noexcept
        : MemoryNoteModel(svNote, nBytes, nMemFormat, nMemSize, MemoryNoteType::Value)
    {
    }
	~ValueMemoryNoteModel() = default;
    ValueMemoryNoteModel(const ValueMemoryNoteModel&) noexcept = delete;
    ValueMemoryNoteModel& operator=(const ValueMemoryNoteModel&) noexcept = delete;
    ValueMemoryNoteModel(ValueMemoryNoteModel&&) noexcept = default;
    ValueMemoryNoteModel& operator=(ValueMemoryNoteModel&&) noexcept = default;

    /// <summary>
    /// Gets the sub-note text associated with the specified bit(s).
    /// </summary>
    std::wstring_view GetSubNote(ra::data::Memory::Size nBits) const;

    /// <summary>
    /// Gets the sub-note text associated with the specified enum value.
    /// </summary>
    std::wstring_view GetEnumText(uint32_t nValue) const;

    void DeterminePreferredMemFormat();

protected:
    static EnumState DetermineEnumState(std::wstring_view svNote);
    std::wstring_view GetFullSummaryStringView() const override;

    static std::wstring_view MatchSubNote(std::wstring_view svNote, std::function<bool(std::wstring_view)> fMatch);
    static std::wstring_view GetValues(const std::wstring_view svLine);
    static size_t constexpr FindValueSplit(const std::wstring_view svLine) noexcept;
    static bool MatchEnumText(const std::wstring_view svValue, uint32_t nValue, bool isHex);
    static bool MatchBitsText(const std::wstring_view svValue, ra::data::Memory::Size nBits);
    static bool ParseBitRange(std::wstring_view svRange, uint32_t& nLow, uint32_t& nHigh);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_VALUEMEMORYNOTEMODEL_H
