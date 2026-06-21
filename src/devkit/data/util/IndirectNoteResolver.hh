#ifndef RA_DATA_UTIL_INDIRECTNOTERESOLVER_H
#define RA_DATA_UTIL_INDIRECTNOTERESOLVER_H
#pragma once

#include "data/Memory.hh"
#include "data/models/MemoryNotesModel.hh"

struct rc_condition_t;

namespace ra {
namespace data {
namespace util {

class IndirectNoteResolver
{
public:
    IndirectNoteResolver(const ra::data::models::MemoryNotesModel& pMemoryNotes)
        : m_pMemoryNotes(&pMemoryNotes)
    {
    }

    enum NodeType : uint8_t
    {
        None = 0,
        Address,
        DereferencedAddress,
        Constant,
        ConstantOffset,
        ArrayOffset,
        Recall,
    };

    struct Node
    {
        uint32_t nValue = 0;
        NodeType nType = NodeType::None;
        uint8_t nModifierType = 0; /* RC_OPERATOR_ */
        const ra::data::models::MemoryNoteModel* pNote = nullptr;
    };

    /// <summary>
    /// Gets the value read from an operand and the path taken to get there.
    /// </summary>
    /// <param name="pCondition">The condition to process</param>
    /// <param name="bLeafIsOperand1"><c>true</c> to process pCondition.operand1, <c>false</c> to process pCondition.operand2</param>
    /// <param name="vParentChain">[out] The path taken to reach the final address.</param>
    /// <returns>The derived address being read for the conditional comparison.</returns>
    uint32_t ResolveOperand(const struct rc_condition_t& pCondition, bool bLeafIsOperand1,
        std::vector<Node>& vParentChain) const;

    std::wstring BuildPath(const std::vector<Node>& vParentChain) const;

protected:
    IndirectNoteResolver() {}

    const ra::data::models::MemoryNotesModel* m_pMemoryNotes = nullptr;
};

} // namespace util
} // namespace data
} // namespace ra

#endif RA_DATA_UTIL_INDIRECTNOTERESOLVER_H
