#ifndef RA_CONDITION_H
#define RA_CONDITION_H
#pragma once

#include "RA_Defs.h"

namespace ra {

enum class ComparisonVariableSize
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
    EightBit,//=Byte,  
    SixteenBit,
    ThirtyTwoBit
};

enum class ComparisonVariableType
{
    Address,         // compare to the value of a live address in RAM
    ValueComparison, // a number. assume 32 bit 
    DeltaMem,        // the value last known at this address.
    DynamicVariable  // a custom user-set variable
};

enum class ComparisonType { Equals, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual, NotEqualTo };
enum class ConditionType { Standard, PauseIf, ResetIf, AddSource, SubSource, AddHits };

namespace enum_sizes {

_CONSTANT_VAR NUM_COMPARISON_VARIABLE_SIZETYPES{ 13U };
_CONSTANT_VAR NUM_COMPARISON_VARIABLE_TYPES{ 4U };
_CONSTANT_VAR NUM_COMPARISON_TYPES{ 6U };
_CONSTANT_VAR NUM_CONDITION_TYPES{ 6U };

} // namespace enum_sizes

// Let's skip using NativeStr if we can
inline constexpr std::array<LPCTSTR, enum_sizes::NUM_COMPARISON_VARIABLE_SIZETYPES> COMPARISONVARIABLESIZE_STR
{
    _T("Bit0"), _T("Bit1"), _T("Bit2"), _T("Bit3"), _T("Bit4"), _T("Bit5"), _T("Bit6"), _T("Bit7"), _T("Lower4"),
    _T("Upper4"), _T("8-bit"), _T("16-bit"), _T("32-bit")
};
inline constexpr std::array<LPCTSTR, enum_sizes::NUM_COMPARISON_VARIABLE_TYPES> COMPARISONVARIABLETYPE_STR
{
    _T("Memory"), _T("Value"), _T("Delta"), _T("DynVar")
};
inline constexpr std::array<LPCTSTR, enum_sizes::NUM_COMPARISON_TYPES> COMPARISONTYPE_STR
{
    _T("="), _T("<"), _T("<="), _T(">"), _T(">="), _T("!=")
};
inline constexpr std::array<LPCTSTR, enum_sizes::NUM_CONDITION_TYPES> CONDITIONTYPE_STR
{
    _T(""), _T("Pause If"), _T("Reset If"), _T("Add Source"), _T("Sub Source"), _T("Add Hits")
};

} // namespace ra

class CompVariable
{
public:
    void Set(ra::ComparisonVariableSize nSize, ra::ComparisonVariableType nType, unsigned int nInitialValue)
    {
        m_nVarSize = nSize;
        m_nVarType = nType;
        m_nVal = nInitialValue;
    }

    void SetValues(unsigned int nVal, unsigned int nPrevVal)
    {
        m_nVal = nVal;
        m_nPreviousVal = nPrevVal;
    }

    void ResetDelta()
    {
        m_nPreviousVal = m_nVal;
    }

    bool ParseVariable(const char*& sInString);	//	Parse from string
    void SerializeAppend(std::string& buffer) const;

    unsigned int GetValue();				//	Returns the live value

    _CONSTANT_FN SetSize(_In_ ra::ComparisonVariableSize nSize) noexcept { m_nVarSize = nSize; }
    _NODISCARD _CONSTANT_FN Size() const noexcept { return m_nVarSize; }

    _CONSTANT_FN SetType(_In_ ra::ComparisonVariableType nType) noexcept { m_nVarType = nType; }
    _NODISCARD _CONSTANT_FN Type() const noexcept { return m_nVarType; }

    inline unsigned int RawValue() const { return m_nVal; }
    inline unsigned int RawPreviousValue() const { return m_nPreviousVal; }

private:
    ra::ComparisonVariableSize m_nVarSize{ ra::ComparisonVariableSize::EightBit };
    ra::ComparisonVariableType m_nVarType{};
    unsigned int m_nVal{};
    unsigned int m_nPreviousVal{};
};

class Condition
{
public:
    //	Parse a Condition from a string of characters
    bool ParseFromString(const char*& sBuffer);
    void SerializeAppend(std::string& buffer) const;

    //	Returns a logical comparison between m_CompSource and m_CompTarget, depending on m_nComparison
    bool Compare(unsigned int nAddBuffer = 0U);

    //	Returns whether a change was made
    bool ResetHits();

    //	Resets 'last known' values
    void ResetDeltas();

    inline CompVariable& CompSource() { return m_nCompSource; }	//	NB both required!!
    inline const CompVariable& CompSource() const { return m_nCompSource; }
    inline CompVariable& CompTarget() { return m_nCompTarget; }
    inline const CompVariable& CompTarget() const { return m_nCompTarget; }
    _CONSTANT_FN SetCompareType(_In_ ra::ComparisonType nType) noexcept { m_nCompareType = nType; }
    _NODISCARD _CONSTANT_FN CompareType() const noexcept { return m_nCompareType; }

    _NODISCARD _CONSTANT_FN RequiredHits() const noexcept { return m_nRequiredHits; }
    _NODISCARD _CONSTANT_FN CurrentHits() const noexcept { return m_nCurrentHits; }

    _NODISCARD _CONSTANT_FN IsResetCondition() const noexcept { return(m_nConditionType == ra::ConditionType::ResetIf); }
    _NODISCARD _CONSTANT_FN IsPauseCondition() const noexcept { return(m_nConditionType == ra::ConditionType::PauseIf); }
    _NODISCARD _CONSTANT_FN IsAddCondition() const noexcept { return(m_nConditionType == ra::ConditionType::AddSource); }
    _NODISCARD _CONSTANT_FN IsSubCondition() const noexcept { return(m_nConditionType == ra::ConditionType::SubSource); }
    _NODISCARD _CONSTANT_FN IsAddHitsCondition() const noexcept { return(m_nConditionType == ra::ConditionType::AddHits); }

    _NODISCARD _CONSTANT_FN GetConditionType() const noexcept { return m_nConditionType; }
    _CONSTANT_FN SetConditionType(_In_ ra::ConditionType nNewType) noexcept { m_nConditionType = nNewType; }

    void SetRequiredHits(unsigned int nHits) { m_nRequiredHits = nHits; }
    void IncrHits() { m_nCurrentHits++; }
    _NODISCARD _CONSTANT_FN  IsComplete() const noexcept { return(m_nCurrentHits >= m_nRequiredHits); }

    void OverrideCurrentHits(unsigned int nHits) { m_nCurrentHits = nHits; }


private:
    ra::ConditionType m_nConditionType{};

    CompVariable       m_nCompSource;
    ra::ComparisonType m_nCompareType{};
    CompVariable       m_nCompTarget;

    unsigned int m_nRequiredHits{};
    unsigned int m_nCurrentHits{};
};

class ConditionGroup
{
public:
    void SerializeAppend(std::string& buffer) const;

    bool Test(bool& bDirtyConditions, bool& bResetRead);
    size_t Count() const { return m_Conditions.size(); }

    void Add(const Condition& newCond) { m_Conditions.push_back(newCond); }
    void Insert(size_t i, const Condition& newCond) { m_Conditions.insert(m_Conditions.begin() + i, newCond); }
    Condition& GetAt(size_t i) { return m_Conditions[i]; }
    const Condition& GetAt(size_t i) const { return m_Conditions[i]; }
    void Clear() { m_Conditions.clear(); }
    void RemoveAt(size_t i);
    bool Reset(bool bIncludingDeltas);	//	Returns dirty

protected:
    bool Test(bool& bDirtyConditions, bool& bResetRead, const std::vector<bool>& vPauseConditions, bool bProcessingPauseIfs);

    std::vector<Condition> m_Conditions;
};

class ConditionSet
{
public:
    bool ParseFromString(const char*& sSerialized);
    void Serialize(std::string& buffer) const;

    bool Test(bool& bDirtyConditions, bool& bWasReset);
    bool Reset();

    void SetAlwaysTrue();
    void SetAlwaysFalse();

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
