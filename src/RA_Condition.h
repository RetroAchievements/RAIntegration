#ifndef RA_CONDITION_H
#define RA_CONDITION_H
#pragma once

#include "ra_fwd.h"

enum class MemSize : std::size_t
{
    Bit_0,
    Bit_1,
    Bit_2,
    Bit_3,
    Bit_4,
    Bit_5,
    Bit_6,
    Bit_7,
    Nibble_Lower,
    Nibble_Upper,
    EightBit,
    SixteenBit,
    ThirtyTwoBit
};

inline constexpr std::array<const LPCTSTR, 13> MEMSIZE_STR{
    _T("Bit0"), _T("Bit1"),   _T("Bit2"),   _T("Bit3"),  _T("Bit4"),   _T("Bit5"),  _T("Bit6"),
    _T("Bit7"), _T("Lower4"), _T("Upper4"), _T("8-bit"), _T("16-bit"), _T("32-bit")};

enum class ComparisonType : std::size_t
{
    Equals,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    NotEqualTo
};
inline constexpr std::array<LPCTSTR, 6> COMPARISONTYPE_STR{_T("="), _T("<"), _T("<="), _T(">"), _T(">="), _T("!=")};

class CompVariable
{
public:
    enum class Type : std::size_t
    {
        Address,         // compare to the value of a live address in RAM
        ValueComparison, // a number. assume 32 bit
        DeltaMem,        // the value last known at this address.
        DynamicVariable  // a custom user-set variable
    };

    inline static constexpr std::array<LPCTSTR, 4> TYPE_STR{_T("Memory"), _T("Value"), _T("Delta"), _T("DynVar")};

public:
    _CONSTANT_FN Set(_In_ MemSize nSize, _In_ CompVariable::Type nType, _In_ unsigned int nInitialValue) noexcept
    {
        m_nVarSize = nSize;
        m_nVarType = nType;
        m_nVal = nInitialValue;
    }

    void SerializeAppend(_Inout_ std::string& buffer) const;

    _CONSTANT_FN SetSize(_In_ MemSize nSize) noexcept { m_nVarSize = nSize; }
    _NODISCARD _CONSTANT_FN GetSize() const noexcept { return m_nVarSize; }

    _CONSTANT_FN SetType(_In_ Type nType) noexcept { m_nVarType = nType; }
    _NODISCARD _CONSTANT_FN GetType() const noexcept { return m_nVarType; }

    _CONSTANT_FN SetValue(_In_ unsigned int nValue) noexcept { m_nVal = nValue; }
    _NODISCARD _CONSTANT_FN GetValue() const noexcept { return m_nVal; }

private:
    MemSize m_nVarSize = MemSize::EightBit;
    Type m_nVarType = Type::Address;
    unsigned int m_nVal = 0U;
};

class Condition
{
public:
    enum class Type : std::size_t
    {
        Standard,
        PauseIf,
        ResetIf,
        AddSource,
        SubSource,
        AddHits
    };

    inline static constexpr std::array<LPCTSTR, 6> TYPE_STR{_T(""),           _T("Pause If"),   _T("Reset If"),
                                                            _T("Add Source"), _T("Sub Source"), _T("Add Hits")};

    void SerializeAppend(std::string& buffer) const;

    _NODISCARD _CONSTANT_FN& CompSource() noexcept { return m_nCompSource; }
    _NODISCARD _CONSTANT_FN& CompSource() const noexcept { return m_nCompSource; }

    _NODISCARD _CONSTANT_FN& CompTarget() noexcept { return m_nCompTarget; }
    _NODISCARD _CONSTANT_FN& CompTarget() const noexcept { return m_nCompTarget; }

    _NODISCARD _CONSTANT_FN CompareType() const noexcept { return m_nCompareType; }
    _CONSTANT_FN SetCompareType(_In_ ComparisonType nType) noexcept { m_nCompareType = nType; }

    _NODISCARD _CONSTANT_FN RequiredHits() const noexcept { return m_nRequiredHits; }
    _CONSTANT_FN SetRequiredHits(unsigned int nHits) noexcept { m_nRequiredHits = nHits; }

    _NODISCARD _CONSTANT_FN IsResetCondition() const noexcept { return (m_nConditionType == Type::ResetIf); }
    _NODISCARD _CONSTANT_FN IsPauseCondition() const noexcept { return (m_nConditionType == Type::PauseIf); }
    _NODISCARD _CONSTANT_FN IsAddCondition() const noexcept { return (m_nConditionType == Type::AddSource); }
    _NODISCARD _CONSTANT_FN IsSubCondition() const noexcept { return (m_nConditionType == Type::SubSource); }
    _NODISCARD _CONSTANT_FN IsAddHitsCondition() const noexcept { return (m_nConditionType == Type::AddHits); }

    _NODISCARD _CONSTANT_FN GetConditionType() const noexcept { return m_nConditionType; }
    _CONSTANT_FN SetConditionType(Type nNewType) noexcept { m_nConditionType = nNewType; }

private:
    Type m_nConditionType = Type::Standard;
    CompVariable m_nCompSource;
    ComparisonType m_nCompareType = ComparisonType::Equals;
    CompVariable m_nCompTarget;
    unsigned int m_nRequiredHits = 0U;
};

class ConditionGroup
{
public:
    void SerializeAppend(std::string& buffer) const;

    _NODISCARD auto Count() const noexcept { return m_Conditions.size(); }
    _NODISCARD auto Empty() const noexcept { return m_Conditions.empty(); }
    auto Add(const Condition& newCond) { m_Conditions.push_back(newCond); }
    auto Add(Condition&& newCond) { m_Conditions.push_back(std::move(newCond)); }
    auto Insert(size_t offs, const Condition& newCond)
    {
        m_Conditions.insert(std::next(m_Conditions.begin(), offs), newCond);
    }
    _NODISCARD auto& GetAt(size_t nPos) { return m_Conditions.at(nPos); }
    _NODISCARD auto& GetAt(size_t nPos) const { return m_Conditions.at(nPos); }
    auto Clear() noexcept { m_Conditions.clear(); }
    void RemoveAt(size_t i);

protected:
    std::vector<Condition> m_Conditions;
};

class ConditionSet
{
public:
    void Serialize(std::string& buffer) const;

    auto Clear() noexcept { m_vConditionGroups.clear(); }
    _NODISCARD auto GroupCount() const noexcept { return m_vConditionGroups.size(); }
    _NODISCARD auto EmptyGroup() const noexcept { return m_vConditionGroups.empty(); }
    // TODO: Put in a type_trait for nothrow EmplaceConstructible to give a nothrow guarantee
    void AddGroup() noexcept { GSL_SUPPRESS_F6 m_vConditionGroups.emplace_back(); }
    auto RemoveLastGroup() { m_vConditionGroups.pop_back(); }
    _NODISCARD auto& GetGroup(size_t nPos) { return m_vConditionGroups.at(nPos); }
    _NODISCARD auto& GetGroup(size_t nPos) const { return m_vConditionGroups.at(nPos); }

    // range for
    _NODISCARD inline auto begin() noexcept { return m_vConditionGroups.begin(); }
    _NODISCARD inline auto begin() const noexcept { return m_vConditionGroups.begin(); }
    _NODISCARD inline auto end() noexcept { return m_vConditionGroups.end(); }
    _NODISCARD inline auto end() const noexcept { return m_vConditionGroups.end(); }

protected:
    std::vector<ConditionGroup> m_vConditionGroups;
};

#endif // !RA_CONDITION_H
