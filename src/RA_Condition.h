#ifndef RA_CONDITION_H
#define RA_CONDITION_H
#pragma once

#include <string>
#include <vector>

enum ComparisonVariableSize
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
    ThirtyTwoBit,

    NumComparisonVariableSizeTypes
};
extern const char* COMPARISONVARIABLESIZE_STR[];

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
    CompVariable() {}

    void Set(ComparisonVariableSize nSize, ComparisonVariableType nType, unsigned int nValue)
    {
        m_nVarSize = nSize;
        m_nVarType = nType;
        m_nVal = nValue;
    }

    void SerializeAppend(std::string& buffer) const;

    inline void SetSize(ComparisonVariableSize nSize) { m_nVarSize = nSize; }
    inline ComparisonVariableSize Size() const { return m_nVarSize; }

    inline void SetType(ComparisonVariableType nType) { m_nVarType = nType; }
    inline ComparisonVariableType Type() const { return m_nVarType; }

    inline void SetValue(unsigned int nValue) { m_nVal = nValue; }
    inline unsigned int Value() const { return m_nVal; }

private:
    ComparisonVariableSize m_nVarSize = ComparisonVariableSize::ThirtyTwoBit;
    ComparisonVariableType m_nVarType = ComparisonVariableType::Address;
    unsigned int m_nVal = 0;
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
    Condition() {}

    void SerializeAppend(std::string& buffer) const;

    inline CompVariable& CompSource() { return m_nCompSource; }	//	NB both required!!
    inline const CompVariable& CompSource() const { return m_nCompSource; }
    
    inline CompVariable& CompTarget() { return m_nCompTarget; }
    inline const CompVariable& CompTarget() const { return m_nCompTarget; }

    inline ComparisonType CompareType() const { return m_nCompareType; }
    inline void SetCompareType(ComparisonType nType) { m_nCompareType = nType; }

    inline unsigned int RequiredHits() const { return m_nRequiredHits; }
    void SetRequiredHits(unsigned int nHits) { m_nRequiredHits = nHits; }

    inline bool IsResetCondition() const { return(m_nConditionType == ResetIf); }
    inline bool IsPauseCondition() const { return(m_nConditionType == PauseIf); }
    inline bool IsAddCondition() const { return(m_nConditionType == AddSource); }
    inline bool IsSubCondition() const { return(m_nConditionType == SubSource); }
    inline bool IsAddHitsCondition() const { return(m_nConditionType == AddHits); }

    inline ConditionType GetConditionType() const { return m_nConditionType; }
    void SetConditionType(ConditionType nNewType) { m_nConditionType = nNewType; }

private:
    CompVariable	m_nCompSource;
    CompVariable	m_nCompTarget;

    ConditionType	m_nConditionType = ConditionType::Standard;
    ComparisonType	m_nCompareType = ComparisonType::Equals;
    unsigned int	m_nRequiredHits = 0U;
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
