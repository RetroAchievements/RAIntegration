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
inline constexpr std::array<const char*, NumComparisonVariableSizeTypes> COMPARISONVARIABLESIZE_STR{
	"Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7", "Lower4", "Upper4", "8-bit", "16-bit",
	"32-bit"
};

enum ComparisonVariableType
{
	Address,			//	compare to the value of a live address in RAM
	ValueComparison,	//	a number. assume 32 bit 
	DeltaMem,			//	the value last known at this address.
	DynamicVariable,	//	a custom user-set variable

	NumComparisonVariableTypes
};
inline constexpr std::array<const char*, NumComparisonVariableTypes> COMPARISONVARIABLETYPE_STR{
	"Memory", "Value", "Delta", "DynVar"
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
inline constexpr std::array<const char*, NumComparisonTypes> COMPARISONTYPE_STR{
	"=", "<", "<=", ">", ">=", "!="
};



class CompVariable
{
public:
	CompVariable()
	 :	m_nVal( 0 ),
		m_nPreviousVal( 0 ),
		m_nVarSize( ComparisonVariableSize::EightBit ),
		m_nVarType( ComparisonVariableType::Address )
	{
	}

public:
	void Set( ComparisonVariableSize nSize, ComparisonVariableType nType, unsigned int nInitialValue )
	{
		m_nVarSize = nSize;
		m_nVarType = nType;
		m_nVal = nInitialValue;
	}

	void SetValues( unsigned int nVal, unsigned int nPrevVal )
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
	
	inline void SetSize( ComparisonVariableSize nSize )		{ m_nVarSize = nSize; }
	inline ComparisonVariableSize Size() const				{ return m_nVarSize; }

	inline void SetType( ComparisonVariableType nType )		{ m_nVarType = nType; }
	inline ComparisonVariableType Type() const				{ return m_nVarType; }
	
	inline unsigned int RawValue() const					{ return m_nVal; }
	inline unsigned int RawPreviousValue() const			{ return m_nPreviousVal; }

private:
	ComparisonVariableSize m_nVarSize;
	ComparisonVariableType m_nVarType;
	unsigned int m_nVal;
	unsigned int m_nPreviousVal;
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
	 :	m_nConditionType( Standard ),
		m_nCompareType( Equals ),
		m_nRequiredHits( 0 ),
		m_nCurrentHits( 0 )
	{}

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
	
	inline CompVariable& CompSource()				{ return m_nCompSource; }	//	NB both required!!
	inline const CompVariable& CompSource() const	{ return m_nCompSource; }
	inline CompVariable& CompTarget()				{ return m_nCompTarget; }
	inline const CompVariable& CompTarget() const	{ return m_nCompTarget; }
	void SetCompareType( ComparisonType nType )		{ m_nCompareType = nType; }
	inline ComparisonType CompareType() const		{ return m_nCompareType; }

	inline unsigned int RequiredHits() const		{ return m_nRequiredHits; }
	inline unsigned int CurrentHits() const			{ return m_nCurrentHits; }

	inline bool IsResetCondition() const			{ return( m_nConditionType == ResetIf ); }
	inline bool IsPauseCondition() const			{ return( m_nConditionType == PauseIf ); }
	inline bool IsAddCondition() const				{ return( m_nConditionType == AddSource ); }
	inline bool IsSubCondition() const				{ return( m_nConditionType == SubSource ); }
	inline bool IsAddHitsCondition() const			{ return( m_nConditionType == AddHits ); }

	inline ConditionType GetConditionType() const	{ return m_nConditionType; }
	void SetConditionType( ConditionType nNewType )	{ m_nConditionType = nNewType; }
	
	void SetRequiredHits( unsigned int nHits )		{ m_nRequiredHits = nHits; }
	void IncrHits()									{ m_nCurrentHits++; }
	bool IsComplete() const							{ return( m_nCurrentHits >= m_nRequiredHits ); }

	void OverrideCurrentHits( unsigned int nHits )	{ m_nCurrentHits = nHits; }


private:
	ConditionType	m_nConditionType;

	CompVariable	m_nCompSource;
	ComparisonType	m_nCompareType;
	CompVariable	m_nCompTarget;

	unsigned int	m_nRequiredHits;
	unsigned int	m_nCurrentHits;
};

<<<<<<< HEAD
class ConditionGroup
=======
inline constexpr std::array<const char*, Condition::NumConditionTypes> CONDITIONTYPE_STR{
	"", "Pause If", "Reset If", "Add Source", "Sub Source", "Add Hits"
};


class ConditionSet
>>>>>>> global_carrays_to_stdarray
{
public:
	void SerializeAppend(std::string& buffer) const;

	//	Final param indicates 'or'
	bool Test( bool& bDirtyConditions, bool& bResetRead );
	size_t Count() const		{ return m_Conditions.size(); }

	void Add( const Condition& newCond )				{ m_Conditions.push_back( newCond ); }
	void Insert( size_t i, const Condition& newCond )	{ m_Conditions.insert( m_Conditions.begin() + i, newCond ); }
	Condition& GetAt( size_t i )						{ return m_Conditions[i]; }
	const Condition& GetAt( size_t i ) const			{ return m_Conditions[i]; }
	void Clear()										{ m_Conditions.clear(); }
	void RemoveAt( size_t i );
	bool Reset( bool bIncludingDeltas );	//	Returns dirty

protected:
	std::vector<Condition> m_Conditions;
};

class ConditionSet
{
public:
	bool ParseFromString(const char*& sSerialized);
	void Serialize(std::string& buffer) const;

	bool Test(bool& bDirtyConditions, bool& bWasReset);
	bool Reset();

	void Clear()                                        { m_vConditionGroups.clear(); }
	size_t GroupCount() const                           { return m_vConditionGroups.size(); }
	void AddGroup()                                     { m_vConditionGroups.emplace_back(); }
	void RemoveLastGroup()                              { m_vConditionGroups.pop_back(); }
	ConditionGroup& GetGroup(size_t i)                  { return m_vConditionGroups[i]; }
	const ConditionGroup& GetGroup(size_t i) const      { return m_vConditionGroups[i]; }

protected:
	std::vector<ConditionGroup> m_vConditionGroups;
};
//extern BOOL CompareConditionValues( unsigned int nLHS, const enum CompType nCmpType, unsigned int nRHS );
extern ComparisonType ReadOperator( char*& pBufferInOut );
extern unsigned int ReadHits( char*& pBufferInOut );
