#ifndef RA_DATA_UTIL_ACHIEVEMENTLOGICSERIALIZER_HH
#define RA_DATA_UTIL_ACHIEVEMENTLOGICSERIALIZER_HH
#pragma once

#include "data/Memory.hh"
#include "data/Requirement.hh"

#include "data/models/MemoryNoteModel.hh"

namespace ra {
namespace data {
namespace util {

class AchievementLogicSerializer
{
public:
    GSL_SUPPRESS_F6 AchievementLogicSerializer() = default;
    virtual ~AchievementLogicSerializer() = default;

    AchievementLogicSerializer(const AchievementLogicSerializer&) noexcept = delete;
    AchievementLogicSerializer& operator=(const AchievementLogicSerializer&) noexcept = delete;
    AchievementLogicSerializer(AchievementLogicSerializer&&) noexcept = delete;
    AchievementLogicSerializer& operator=(AchievementLogicSerializer&&) noexcept = delete;

    /// <summary>
    /// Starts a new condition in the serialized logic.
    /// </summary>
    static void AppendConditionSeparator(std::string& sBuffer) { sBuffer.push_back('_'); }

    /// <summary>
    /// Appends a condition type prefix to the serialized logic.
    /// </summary>
    static void AppendConditionType(std::string& sBuffer, Requirement::Type nType);

    /// <summary>
    /// Appends an operand to the serialized logic.
    /// </summary>
    static void AppendOperand(std::string& sBuffer, Requirement::OperandType nType, Memory::Size nSize, uint32_t nValue);

    /// <summary>
    /// Appends an operand to the serialized logic.
    /// </summary>
    static void AppendOperand(std::string& sBuffer, Requirement::OperandType nType, Memory::Size nSize, float fValue);

    /// <summary>
    /// Appends an operator to the serialized logic.
    /// </summary>
    static void AppendOperator(std::string& sBuffer, Requirement::OperatorType nType);

    /// <summary>
    /// Appends an hit target suffix to the serialized logic.
    /// </summary>
    static void AppendHitTarget(std::string& sBuffer, uint32_t nTarget);

    /// <summary>
    /// Constructs the serialized logic to read memory at <paramref="pLeafNote"> using a pointer
    /// chain starting at <paramref="pRootNote">.
    /// </summary>
    /// <remarks>
    /// Result will include a Measured flag for immediate use in bookmarks.
    /// </remarks>
    static std::string BuildMemRefChain(const ra::data::models::MemoryNoteModel& pRootNote,
                                        const ra::data::models::MemoryNoteModel& pLeafNote);
};

} // namespace util
} // namespace data
} // namespace ra

#endif // !RA_DATA_UTIL_ACHIEVEMENTLOGICSERIALIZER_HH
