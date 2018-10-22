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

class CompVariable
{
public:
    enum class Type
    {
        Address,         // compare to the value of a live address in RAM
        ValueComparison, // a number. assume 32 bit 
        DeltaMem,        // the value last known at this address.
        DynamicVariable  // a custom user-set variable
    };

    inline static constexpr std::array<const LPCTSTR, 4> TYPE_STR
    {
        _T("Memory"),
        _T("Value"),
        _T("Delta"),
        _T("DynVar")
    };

public:
    inline constexpr CompVariable() noexcept = default;
    inline constexpr explicit CompVariable(_In_ MemSize nSize,
                                           _In_ CompVariable::Type nType,
                                           _In_ unsigned int nInitialValue) noexcept :
        m_nVarSize{ nSize },
        m_nVarType{ nType },
        m_nVal{ nInitialValue }
    {
    }

    inline constexpr explicit CompVariable(_In_ unsigned int nInitialValue,
                                           _In_ unsigned int nPrevVal) noexcept :
        m_nVal{ nInitialValue },
        m_nPreviousVal{ nPrevVal }
    {
    }

    _CONSTANT_FN Set(_In_ MemSize nSize,
                     _In_ CompVariable::Type nType,
                     _In_ unsigned int nInitialValue) noexcept
    {
        m_nVarSize = nSize;
        m_nVarType = nType;
        m_nVal     = nInitialValue;
    }

    _CONSTANT_FN SetValues(_In_ unsigned int nVal, _In_ unsigned int nPrevVal) noexcept
    {
        m_nVal         = nVal;
        m_nPreviousVal = nPrevVal;
    }

    _CONSTANT_FN ResetDelta() noexcept { m_nPreviousVal = m_nVal; }

    _NODISCARD bool ParseVariable(_Inout_ const char*& sInString); // Parse from string
    void SerializeAppend(_Out_ std::string& buffer) const;

    unsigned int GetValue(); // Returns the live value

    _CONSTANT_FN SetSize(_In_ MemSize nSize) noexcept { m_nVarSize = nSize; }
    _NODISCARD _CONSTANT_FN Size() const noexcept { return m_nVarSize; }

    _CONSTANT_FN SetType(_In_ Type nType) noexcept { m_nVarType = nType; }
    _NODISCARD _CONSTANT_FN GetType() const noexcept { return m_nVarType; }

    _NODISCARD _CONSTANT_FN RawValue() const noexcept { return m_nVal; }
    _NODISCARD _CONSTANT_FN RawPreviousValue() const noexcept { return m_nPreviousVal; }

private:
    MemSize m_nVarSize{ MemSize::EightBit };
    Type m_nVarType{};
    unsigned int m_nVal{};
    unsigned int m_nPreviousVal{};
};

class Condition
{
public:
    enum class Type
    {
        Standard,
        PauseIf,
        ResetIf,
        AddSource,
        SubSource,
        AddHits
    };

    inline static constexpr std::array<const LPCTSTR, 6> TYPE_STR
    {
        _T(""),
        _T("Pause If"),
        _T("Reset If"),
        _T("Add Source"),
        _T("Sub Source"),
        _T("Add Hits")
    };

public:
    //  Parse a Condition from a string of characters
    bool ParseFromString(const char*& sBuffer);
    void SerializeAppend(std::string& buffer) const;

    //  Returns a logical comparison between m_CompSource and m_CompTarget, depending on m_nComparison
    bool Compare(unsigned int nAddBuffer = 0);

    //  Returns whether a change was made
    bool ResetHits();

    //  Resets 'last known' values
    void ResetDeltas();

    inline CompVariable& CompSource() { return m_nCompSource; } //  NB both required!!
    inline const CompVariable& CompSource() const { return m_nCompSource; }
    inline CompVariable& CompTarget() { return m_nCompTarget; }
    inline const CompVariable& CompTarget() const { return m_nCompTarget; }
    void SetCompareType(ComparisonType nType) { m_nCompareType = nType; }
    inline ComparisonType CompareType() const { return m_nCompareType; }

    inline unsigned int RequiredHits() const { return m_nRequiredHits; }
    inline unsigned int CurrentHits() const { return m_nCurrentHits; }

    _NODISCARD _CONSTANT_FN IsResetCondition() const { return(m_nConditionType == Type::ResetIf); }
    _NODISCARD _CONSTANT_FN IsPauseCondition() const { return(m_nConditionType == Type::PauseIf); }
    _NODISCARD _CONSTANT_FN IsAddCondition() const { return(m_nConditionType == Type::AddSource); }
    _NODISCARD _CONSTANT_FN IsSubCondition() const { return(m_nConditionType == Type::SubSource); }
    _NODISCARD _CONSTANT_FN IsAddHitsCondition() const { return(m_nConditionType == Type::AddHits); }

    inline Type GetConditionType() const { return m_nConditionType; }
    void SetConditionType(Type nNewType) { m_nConditionType = nNewType; }

    void SetRequiredHits(unsigned int nHits) { m_nRequiredHits = nHits; }
    void IncrHits() { m_nCurrentHits++; }
    bool IsComplete() const { return(m_nCurrentHits >= m_nRequiredHits); }

    void OverrideCurrentHits(unsigned int nHits) { m_nCurrentHits = nHits; }


private:
    Type           m_nConditionType = Type::Standard;

    CompVariable    m_nCompSource;
    ComparisonType m_nCompareType   = ComparisonType::Equals;
    CompVariable    m_nCompTarget;

    unsigned int   m_nRequiredHits  = 0U;
    unsigned int   m_nCurrentHits   = 0U;
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
    bool Reset(bool bIncludingDeltas);  //  Returns dirty

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
