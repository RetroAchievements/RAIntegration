#include "IndirectNoteResolver.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {
namespace util {

static uint32_t ResolveOperandRecursive(const rc_operand_t* pOperand,
    std::vector<ra::data::util::IndirectNoteResolver::Node>& vParentChain,
    const ra::data::models::MemoryNotesModel& pMemoryNotes)
{
    rc_typed_value_t pValue;
    rc_evaluate_operand(&pValue, pOperand, nullptr);
    rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);

    // check for {recall}
    if (pOperand->type == RC_OPERAND_RECALL)
    {
        auto* pParentNote = !vParentChain.empty() ? vParentChain.back().pNote : nullptr;

        auto& pNode = vParentChain.emplace_back();
        pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::Recall;

        if (pParentNote)
            pNode.pNote = pParentNote->GetPointerNoteAtOffset(pValue.value.u32);
        else
            pNode.pNote = pMemoryNotes.FindMemoryNoteModel(pValue.value.u32, false);

        pNode.nValue = pValue.value.u32;
        return pValue.value.u32;
    }

    // check for constant offset
    if (!rc_operand_is_memref(pOperand))
    {
        auto* pParentNote = !vParentChain.empty() ? vParentChain.back().pNote : nullptr;

        auto& pNode = vParentChain.emplace_back();
        pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::ConstantOffset;

        if (pParentNote)
            pNode.pNote = pParentNote->GetPointerNoteAtOffset(pValue.value.u32);

        pNode.nValue = pValue.value.u32;
        return pValue.value.u32;
    }

    // check for root pointer
    if (pOperand->value.memref->value.memref_type != RC_MEMREF_TYPE_MODIFIED_MEMREF)
    {
        auto& pNode = vParentChain.emplace_back();
        pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::Address;
        pNode.nValue = pOperand->value.memref->address;

        // find the memory note associated to the root pointer
        pNode.pNote = pMemoryNotes.FindMemoryNoteModel(pNode.nValue, false);
        if (!pNode.pNote)
        {
            const auto nStartAddress = pMemoryNotes.FindNoteStart(pNode.nValue);
            if (nStartAddress != 0xFFFFFFFF && nStartAddress != pNode.nValue)
            {
                const auto nOffset = pNode.nValue - nStartAddress;
                pNode.nValue = nStartAddress;
                const auto* pNote = pMemoryNotes.FindMemoryNoteModel(nStartAddress);
                pNode.pNote = pNote;

                auto& pNode2 = vParentChain.emplace_back();
                pNode2.nType = ra::data::util::IndirectNoteResolver::NodeType::Constant;
                pNode2.nValue = nOffset;
                pNode2.nModifierType = RC_OPERATOR_ADD;
                pNode2.pNote = pNote;
            }
        }

        return pValue.value.u32;
    }

    // process offset and recurse
    GSL_SUPPRESS_TYPE1 const auto* pModifiedMemref =
        reinterpret_cast<const rc_modified_memref_t*>(pOperand->value.memref);
    ResolveOperandRecursive(&pModifiedMemref->parent, vParentChain, pMemoryNotes);

    if (vParentChain.back().nType == ra::data::util::IndirectNoteResolver::NodeType::Address)
        vParentChain.back().nType = ra::data::util::IndirectNoteResolver::NodeType::DereferencedAddress;

    // calculate current values of both operands
    rc_evaluate_operand(&pValue, &pModifiedMemref->parent, nullptr);
    rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);

    rc_typed_value_t pModifier{};
    rc_evaluate_operand(&pModifier, &pModifiedMemref->modifier, nullptr);
    rc_typed_value_convert(&pModifier, RC_VALUE_TYPE_UNSIGNED);

    // if it's an indirect read, determine if it's a pointer or index offset
    if (pModifiedMemref->modifier_type == RC_OPERATOR_INDIRECT_READ)
    {
        // value is the parent pointer, modifier is the offset. combine them to get the new address
        rc_typed_value_combine(&pValue, &pModifier, RC_OPERATOR_ADD);
        const auto nAddress = pModifier.value.u32;

        const auto* pParentNote = !vParentChain.empty() ? vParentChain.back().pNote : nullptr;
        if (pParentNote && !pParentNote->IsPointer())
        {
            // if the parent note is not a pointer, assume it's an index
            Expects(pModifiedMemref->modifier.type == RC_OPERAND_CONST);
            auto& pNode = vParentChain.emplace_back();
            pNode.nValue = nAddress;
            pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::ArrayOffset;

            // find the memory note associated to the start of the array
            pNode.pNote = pMemoryNotes.FindMemoryNoteModel(nAddress, false);

            // return the address offset into the array
            return pValue.value.u32;
        }

        // process the pointer
        ResolveOperandRecursive(&pModifiedMemref->modifier, vParentChain, pMemoryNotes);
        vParentChain.back().nModifierType = RC_OPERATOR_INDIRECT_READ;

        return pValue.value.u32;
    }

    // not an indirect read. must be a scalar.

    // don't report mask on every pointer evaluation
    if (pModifiedMemref->modifier_type != RC_OPERATOR_AND)
    {
        const auto* pParentNote = !vParentChain.empty() ? vParentChain.back().pNote : nullptr;

        auto& pNode = vParentChain.emplace_back();
        pNode.nModifierType = pModifiedMemref->modifier_type;
        pNode.pNote = pParentNote;

        if (pModifiedMemref->modifier.type == RC_OPERAND_RECALL)
        {
            pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::Recall;
            pNode.nValue = pModifier.value.u32;
        }
        else if (rc_operand_is_memref(&pModifiedMemref->modifier))
        {
            pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::Address;
            pNode.nValue = pModifiedMemref->modifier.value.memref->address;
        }
        else
        {
            pNode.nType = ra::data::util::IndirectNoteResolver::NodeType::Constant;
            pNode.nValue = pModifiedMemref->modifier.value.num;
        }
    }

    // return the result of combining the value and the modifier
    rc_typed_value_combine(&pValue, &pModifier, pModifiedMemref->modifier_type);
    return pValue.value.u32;
}

ra::data::ByteAddress IndirectNoteResolver::ResolveOperand(
    const struct rc_condition_t& pCondition, bool bLeafIsOperand1,
    std::vector<Node>& vParentChain) const
{
    Expects(m_pMemoryNotes != nullptr);

    if (bLeafIsOperand1)
    {
        const auto* pOperand1 = rc_condition_get_real_operand1(&pCondition);
        if (rc_operand_is_memref(pOperand1))
            return ResolveOperandRecursive(pOperand1, vParentChain, *m_pMemoryNotes);
    }
    else
    {
        if (rc_operand_is_memref(&pCondition.operand2))
            return ResolveOperandRecursive(&pCondition.operand2, vParentChain, *m_pMemoryNotes);
    }

    return 0;
}

std::wstring IndirectNoteResolver::BuildPath(const std::vector<Node>& vParentChain) const
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    std::wstring sPointerChain;

    for (const auto& pNode : vParentChain)
    {
        switch (pNode.nModifierType)
        {
            case RC_OPERATOR_INDIRECT_READ:
            case RC_OPERATOR_ADD:
                sPointerChain.push_back(L'+');
                break;
            case RC_OPERATOR_SUB:
                sPointerChain.push_back(L'-');
                break;
            case RC_OPERATOR_MULT:
                sPointerChain.push_back(L'*');
                break;
            case RC_OPERATOR_DIV:
                sPointerChain.push_back(L'/');
                break;
            case RC_OPERATOR_AND:
                sPointerChain.push_back(L'&');
                break;
            case RC_OPERATOR_XOR:
                sPointerChain.push_back(L'^');
                break;
            case RC_OPERATOR_MOD:
                sPointerChain.push_back(L'%');
                break;
            default:
                break;
        }

        switch (pNode.nType)
        {
            case ra::data::util::IndirectNoteResolver::NodeType::Recall:
                sPointerChain += ra::util::String::Printf(L"{recall:0x%02x}", pNode.nValue);
                break;

            case ra::data::util::IndirectNoteResolver::NodeType::ConstantOffset:
                sPointerChain += ra::util::String::Printf(L"0x%02x", pNode.nValue);
                break;

            case ra::data::util::IndirectNoteResolver::NodeType::Address:
                sPointerChain += pMemoryContext.FormatAddress(pNode.nValue);
                break;

            case ra::data::util::IndirectNoteResolver::NodeType::DereferencedAddress:
                sPointerChain.push_back('$');
                sPointerChain += pMemoryContext.FormatAddress(pNode.nValue);
                break;

            case ra::data::util::IndirectNoteResolver::NodeType::ArrayOffset:
                sPointerChain.insert(0, ra::util::String::Printf(L"%s[", pMemoryContext.FormatAddress(pNode.nValue)));
                sPointerChain.push_back(']');
                break;

            case ra::data::util::IndirectNoteResolver::NodeType::Constant:
                switch (pNode.nModifierType)
                {
                    case RC_OPERATOR_AND:
                    case RC_OPERATOR_XOR: // use hex for bitwise combines
                        sPointerChain += ra::util::String::Printf(L"0x%02x", pNode.nValue);
                        break;

                    default:
                        if (pNode.nValue >= 0x1000 && (pNode.nValue & 0xFF) == 0) // large multiple of 256, use hex
                            sPointerChain += ra::util::String::Printf(L"0x%04x", pNode.nValue);
                        else if (pNode.nValue >= 10)
                            sPointerChain += ra::util::String::Printf(L"0x%02x", pNode.nValue);
                        else // otherwise use decimal
                            sPointerChain += std::to_wstring(pNode.nValue);
                        break;
                }
                break;

        }
    }

    return sPointerChain;
}


} // namespace util
} // namespace data
} // namespace ra
