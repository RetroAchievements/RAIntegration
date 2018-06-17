#ifndef RA_CONDITION_H
#define RA_CONDITION_H
#pragma once

#include "RA_Defs.h"

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
#pragma warning(push)
    // unused inline functions
#pragma warning(disable : 4514) 
    void Set(ComparisonVariableSize nSize, ComparisonVariableType nType, unsigned int nInitialValue)
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

    inline void SetSize(ComparisonVariableSize nSize) { m_nVarSize = nSize; }
    inline ComparisonVariableSize Size() const { return m_nVarSize; }

    inline void SetType(ComparisonVariableType nType) { m_nVarType = nType; }
    inline ComparisonVariableType Type() const { return m_nVarType; }

    inline unsigned int RawValue() const { return m_nVal; }
    inline unsigned int RawPreviousValue() const { return m_nPreviousVal; }
#pragma warning(pop)
private:
    ComparisonVariableSize m_nVarSize{ ComparisonVariableSize::EightBit };
    ComparisonVariableType m_nVarType{ ComparisonVariableType::Address };
    unsigned int m_nVal{};
    unsigned int m_nPreviousVal = {};
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
    //	Parse a Condition from a string of characters
    bool ParseFromString(const char*& sBuffer);
    void SerializeAppend(std::string& buffer) const;

    //	Returns a logical comparison between m_CompSource and m_CompTarget, depending on m_nComparison
    bool Compare(unsigned int nAddBuffer = 0);

    //	Returns whether a change was made
    bool ResetHits();

    //	Resets 'last known' values
    void ResetDeltas();
#pragma warning(push)
    // unused inline functions
#pragma warning(disable : 4514) 
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
#pragma warning(pop)

private:
    ConditionType	m_nConditionType{ConditionType::Standard};

    CompVariable	m_nCompSource;
    ComparisonType	m_nCompareType{ComparisonType::Equals};
    CompVariable	m_nCompTarget;

    unsigned int	m_nRequiredHits ={};
    unsigned int	m_nCurrentHits ={};
};

class ConditionGroup
{
    using Conditions = std::vector<Condition>;
public:
    void SerializeAppend(std::string& buffer) const;

    bool Test(bool& bDirtyConditions, bool& bResetRead);
    

#pragma warning(push)
#pragma warning(disable : 4514) // unused inline functions
    size_t Count() const { return m_Conditions.size(); }
    auto Add(_In_ const Condition& newCond) { m_Conditions.push_back(newCond); }
    auto Insert(_In_ size_t i, _In_ const Condition& newCond)
    {
        m_Conditions.insert(std::next(m_Conditions.begin(), std::make_signed_t<Conditions::size_type>(i)),
            newCond);
    }
    auto& GetAt(_In_ size_t i) const { return m_Conditions.at(i); }
    auto& GetAt(_In_ size_t i) { return m_Conditions.at(i); }
    auto Clear() noexcept { m_Conditions.clear(); }
#pragma warning(pop)

    
    void RemoveAt(size_t i);
    bool Reset(bool bIncludingDeltas);	//	Returns dirty

protected:
    bool Test(bool& bDirtyConditions, bool& bResetRead, const std::vector<bool>& vPauseConditions, bool bProcessingPauseIfs);

    Conditions m_Conditions;
};

class ConditionSet
{
    using ConditionGroups = std::vector<ConditionGroup>;
public:
    bool ParseFromString(const char*& sSerialized);
    void Serialize(std::string& buffer) const;

    bool Test(bool& bDirtyConditions, bool& bWasReset);
    bool Reset();

#pragma warning(push)
#pragma warning(disable : 4514) // unused functions
    auto Clear() { m_vConditionGroups.clear(); }
    auto AddGroup() { m_vConditionGroups.emplace_back(); }
    auto RemoveLastGroup() { m_vConditionGroups.pop_back(); }
    _NODISCARD auto& GetGroup(_In_ size_t i) { return m_vConditionGroups.at(i); }
    _NODISCARD auto& GetGroup(_In_ size_t i) const { return m_vConditionGroups.at(i); }
    auto GroupCount() const { return m_vConditionGroups.size(); }
#pragma warning(pop)

protected:
    ConditionGroups m_vConditionGroups;
};


#endif // !RA_CONDITION_H
