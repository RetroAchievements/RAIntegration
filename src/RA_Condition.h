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
    //Byte,
    EightBit,//=Byte,  
    SixteenBit,
    ThirtyTwoBit
};

namespace enum_sizes {

_CONSTANT_VAR NUM_COMPARISON_VARIABLE_SIZETYPES{13U};

} // namespace enum_sizes

// Let's skip using NativeStr if we can
inline constexpr std::array<LPCTSTR, enum_sizes::NUM_COMPARISON_VARIABLE_SIZETYPES> COMPARISONVARIABLESIZE_STR
{
    _T("Bit0"), _T("Bit1"), _T("Bit2"), _T("Bit3"), _T("Bit4"), _T("Bit5"), _T("Bit6"), _T("Bit7"), _T("Lower4"),
    _T("Upper4"), _T("8-bit"), _T("16-bit"), _T("32-bit")
};

} // namespace ra

enum ComparisonVariableType
{
    Address,			//	compare to the value of a live address in RAM
    ValueComparison,	//	a number. assume 32 bit 
    DeltaMem,			//	the value last known at this address.
    DynamicVariable,	//	a custom user-set variable

    NumComparisonVariableTypes
};
extern const char* COMPARISONVARIABLETYPE_STR[];

enum ComparisonType
{
    Equals,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    NotEqualTo,

    NumComparisonTypes
};
extern const char* COMPARISONTYPE_STR[];

extern const char* CONDITIONTYPE_STR[];

class CompVariable
{
public:
    void Set(ra::ComparisonVariableSize nSize, ComparisonVariableType nType, unsigned int nInitialValue)
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

    inline void SetType(ComparisonVariableType nType) { m_nVarType = nType; }
    inline ComparisonVariableType Type() const { return m_nVarType; }

    inline unsigned int RawValue() const { return m_nVal; }
    inline unsigned int RawPreviousValue() const { return m_nPreviousVal; }

private:
    ra::ComparisonVariableSize m_nVarSize{ ra::ComparisonVariableSize::EightBit };
    ComparisonVariableType m_nVarType{};
    unsigned int m_nVal{};
    unsigned int m_nPreviousVal{};
};


class Condition
{
public:
    enum ConditionType
    {
        Standard,
        PauseIf,
        ResetIf,
        AddSource,
        SubSource,
        AddHits,

        NumConditionTypes
    };

public:
    Condition()
        : m_nConditionType(Standard),
        m_nCompareType(Equals),
        m_nRequiredHits(0),
        m_nCurrentHits(0)
    {
    }

public:
    //	Parse a Condition from a string of characters
    bool ParseFromString(const char*& sBuffer);
    void SerializeAppend(std::string& buffer) const;

    //	Returns a logical comparison between m_CompSource and m_CompTarget, depending on m_nComparison
    bool Compare(unsigned int nAddBuffer = 0);

    //	Returns whether a change was made
    bool ResetHits();

    //	Resets 'last known' values
    void ResetDeltas();

    inline CompVariable& CompSource() { return m_nCompSource; }	//	NB both required!!
    inline const CompVariable& CompSource() const { return m_nCompSource; }
    inline CompVariable& CompTarget() { return m_nCompTarget; }
    inline const CompVariable& CompTarget() const { return m_nCompTarget; }
    void SetCompareType(ComparisonType nType) { m_nCompareType = nType; }
    inline ComparisonType CompareType() const { return m_nCompareType; }

    inline unsigned int RequiredHits() const { return m_nRequiredHits; }
    inline unsigned int CurrentHits() const { return m_nCurrentHits; }

    inline bool IsResetCondition() const { return(m_nConditionType == ResetIf); }
    inline bool IsPauseCondition() const { return(m_nConditionType == PauseIf); }
    inline bool IsAddCondition() const { return(m_nConditionType == AddSource); }
    inline bool IsSubCondition() const { return(m_nConditionType == SubSource); }
    inline bool IsAddHitsCondition() const { return(m_nConditionType == AddHits); }

    inline ConditionType GetConditionType() const { return m_nConditionType; }
    void SetConditionType(ConditionType nNewType) { m_nConditionType = nNewType; }

    void SetRequiredHits(unsigned int nHits) { m_nRequiredHits = nHits; }
    void IncrHits() { m_nCurrentHits++; }
    bool IsComplete() const { return(m_nCurrentHits >= m_nRequiredHits); }

    void OverrideCurrentHits(unsigned int nHits) { m_nCurrentHits = nHits; }


private:
    ConditionType	m_nConditionType;

    CompVariable	m_nCompSource;
    ComparisonType	m_nCompareType;
    CompVariable	m_nCompTarget;

    unsigned int	m_nRequiredHits;
    unsigned int	m_nCurrentHits;
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
