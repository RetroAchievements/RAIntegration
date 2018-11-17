#ifndef RA_CONDITION_H
#define RA_CONDITION_H
#pragma once

#include "ra_fwd.h"

enum class MemSize
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

inline constexpr std::array<const LPCTSTR, 13> MEMSIZE_STR
{
    _T("Bit0"),
    _T("Bit1"),
    _T("Bit2"),
    _T("Bit3"),
    _T("Bit4"),
    _T("Bit5"),
    _T("Bit6"),
    _T("Bit7"),
    _T("Lower4"),
    _T("Upper4"),
    _T("8-bit"),
    _T("16-bit"),
    _T("32-bit")
};


enum class ComparisonType : std::size_t
{
    Equals,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    NotEqualTo
};
inline constexpr std::array<LPCTSTR, 6> COMPARISONTYPE_STR
{
    _T("="),
    _T("<"),
    _T("<="),
    _T(">"),
    _T(">="),
    _T("!=")
};

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

    inline static constexpr std::array<LPCTSTR, 4> TYPE_STR
    {
        _T("Memory"),
        _T("Value"),
        _T("Delta"),
        _T("DynVar")
    };

public:
    _CONSTANT_FN Set(_In_ MemSize nSize,
                     _In_ CompVariable::Type nType,
                     _In_ unsigned int nInitialValue) noexcept
    {
        m_nVarSize = nSize;
        m_nVarType = nType;
        m_nVal     = nInitialValue;
    }

    void SerializeAppend(_Out_ std::string& buffer) const;

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

    inline static constexpr std::array<LPCTSTR, 6> TYPE_STR
    {
        _T(""),
        _T("Pause If"),
        _T("Reset If"),
        _T("Add Source"),
        _T("Sub Source"),
        _T("Add Hits")
    };

    void SerializeAppend(std::string& buffer) const;

    inline CompVariable& CompSource() { return m_nCompSource; }
    inline const CompVariable& CompSource() const { return m_nCompSource; }
    
    inline CompVariable& CompTarget() { return m_nCompTarget; }
    inline const CompVariable& CompTarget() const { return m_nCompTarget; }

    inline ComparisonType CompareType() const { return m_nCompareType; }
    inline void SetCompareType(ComparisonType nType) { m_nCompareType = nType; }

    inline unsigned int RequiredHits() const { return m_nRequiredHits; }
    void SetRequiredHits(unsigned int nHits) { m_nRequiredHits = nHits; }

    _NODISCARD _CONSTANT_FN IsResetCondition() const { return(m_nConditionType == Type::ResetIf); }
    _NODISCARD _CONSTANT_FN IsPauseCondition() const { return(m_nConditionType == Type::PauseIf); }
    _NODISCARD _CONSTANT_FN IsAddCondition() const { return(m_nConditionType == Type::AddSource); }
    _NODISCARD _CONSTANT_FN IsSubCondition() const { return(m_nConditionType == Type::SubSource); }
    _NODISCARD _CONSTANT_FN IsAddHitsCondition() const { return(m_nConditionType == Type::AddHits); }

    inline Type GetConditionType() const { return m_nConditionType; }
    void SetConditionType(Type nNewType) { m_nConditionType = nNewType; }

private:
    Type           m_nConditionType = Type::Standard;
    CompVariable    m_nCompSource;
    ComparisonType m_nCompareType   = ComparisonType::Equals;
    CompVariable    m_nCompTarget;

    unsigned int   m_nRequiredHits  = 0U;
};

class ConditionGroup
{
public:
    void SerializeAppend(std::string& buffer) const;

    size_t Count() const { return m_Conditions.size(); }

    void Add(const Condition& newCond) { m_Conditions.push_back(newCond); }
    void Insert(size_t i, const Condition& newCond) { m_Conditions.insert(m_Conditions.begin() + i, newCond); }
    Condition& GetAt(size_t i) { return m_Conditions[i]; }
    const Condition& GetAt(size_t i) const { return m_Conditions[i]; }
    void Clear() { m_Conditions.clear(); }
    void RemoveAt(size_t i);

protected:
    std::vector<Condition> m_Conditions;
};

class ConditionSet
{
public:
    void Serialize(std::string& buffer) const;

    void Clear() { m_vConditionGroups.clear(); }
    size_t GroupCount() const { return m_vConditionGroups.size(); }
    void AddGroup() { m_vConditionGroups.emplace_back(); }
    void RemoveLastGroup() { m_vConditionGroups.pop_back(); }
    ConditionGroup& GetGroup(size_t i) { return m_vConditionGroups[i]; }
    const ConditionGroup& GetGroup(size_t i) const { return m_vConditionGroups[i]; }

protected:
    std::vector<ConditionGroup> m_vConditionGroups;
};

#endif // !RA_CONDITION_H
