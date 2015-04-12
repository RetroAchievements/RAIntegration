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
	CompVariable()
	 :	m_nVal( 0 ),
		m_nPreviousVal( 0 ),
		m_nVarSize( ComparisonVariableSize::EightBit ),
		m_nVarType( ComparisonVariableType::Address ),
		m_nBankID( 0 )
	{
	}

public:
	void Set( ComparisonVariableSize nSize, ComparisonVariableType nType, unsigned int nInitialValue, unsigned short nBankID )
	{
		m_nVarSize = nSize;
		m_nVarType = nType;
		m_nVal = nInitialValue;
		m_nBankID = nBankID;
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
	
	//void Clear()
	//{
	//	m_nVal = 0;
	//	m_nPreviousVal = 0;
	//	m_nVarSize = COMPVAR_8BIT;
	//	m_nVarType = COMPVARTYPE_ADDRESS;
	//}

	void ParseVariable( char*& sInString );	//	Parse from string
	unsigned int GetValue();				//	Returns the live value
	
	inline void SetSize( ComparisonVariableSize nSize )		{ m_nVarSize = nSize; }
	inline ComparisonVariableSize Size() const				{ return m_nVarSize; }

	inline void SetType( ComparisonVariableType nType )		{ m_nVarType = nType; }
	inline ComparisonVariableType Type() const				{ return m_nVarType; }
	
	inline unsigned int RawValue() const					{ return m_nVal; }
	inline unsigned int RawPreviousValue() const			{ return m_nPreviousVal; }
	inline unsigned short BankID() const					{ return m_nBankID; }

private:
	ComparisonVariableSize m_nVarSize;
	ComparisonVariableType m_nVarType;
	unsigned short m_nBankID;
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
	void ParseFromString( char*& sBuffer );

	//	Returns a logical comparison between m_CompSource and m_CompTarget, depending on m_nComparison
	BOOL Compare();

	//	Returns whether a change was made
	BOOL ResetHits();

	//	Resets 'last known' values
	void ResetDeltas();
	void Clear();
	
	inline CompVariable& CompSource()				{ return m_nCompSource; }	//	NB both required!!
	inline const CompVariable& CompSource() const	{ return m_nCompSource; }
	inline CompVariable& CompTarget()				{ return m_nCompTarget; }
	inline const CompVariable& CompTarget() const	{ return m_nCompTarget; }

	void SetCompareType( ComparisonType nType )		{ m_nCompareType = nType; }

	inline ComparisonType CompareType() const		{ return m_nCompareType; }

	inline unsigned int RequiredHits() const		{ return m_nRequiredHits; }
	inline unsigned int CurrentHits() const			{ return m_nCurrentHits; }

	inline BOOL IsResetCondition() const			{ return( m_nConditionType == ResetIf ); }
	inline BOOL IsPauseCondition() const			{ return( m_nConditionType == PauseIf ); }
	inline ConditionType GetConditionType() const	{ return m_nConditionType; }
	void SetConditionType( ConditionType nNewType )	{ m_nConditionType = nNewType; }
	
	void SetRequiredHits( unsigned int nHits )		{ m_nRequiredHits = nHits; }
	void IncrHits()									{ m_nCurrentHits++; }
	BOOL IsComplete() const							{ return( m_nCurrentHits >= m_nRequiredHits ); }

	void OverrideCurrentHits( unsigned int nHits )	{ m_nCurrentHits = nHits; }

	void SetIsBasicCondition()						{ m_nConditionType = Standard; }
	void SetIsPauseCondition()						{ m_nConditionType = PauseIf; }
	void SetIsResetCondition()						{ m_nConditionType = ResetIf; }

	void Set( const Condition& rRHS )				{ (*this) = rRHS; }

private:
	ConditionType	m_nConditionType;

	CompVariable	m_nCompSource;
	ComparisonType	m_nCompareType;
	CompVariable	m_nCompTarget;
	
	unsigned int	m_nRequiredHits;
	unsigned int	m_nCurrentHits;
};

class ConditionSet
{
public:
	//	Final param indicates 'or'
	BOOL Test( BOOL& bDirtyConditions, BOOL& bResetRead, BOOL bMatchAny );
	size_t Count() const		{ return m_Conditions.size(); }

	void Add( const Condition& newCond )		{ m_Conditions.push_back( newCond ); }
	Condition& GetAt( size_t i )				{ return m_Conditions[i]; }
	const Condition& GetAt( size_t i ) const	{ return m_Conditions[i]; }
	void Clear()								{ m_Conditions.clear(); }
	void RemoveAt( size_t i );
	BOOL Reset( BOOL bIncludingDeltas );	//	Returns dirty

protected:
	std::vector<Condition> m_Conditions;
};

extern ComparisonVariableSize PrefixToComparisonSize( char cPrefix );
extern const char* ComparisonSizeToPrefix( ComparisonVariableSize nSize );
extern const char* ComparisonTypeToStr( ComparisonType nType );

//extern BOOL CompareConditionValues( unsigned int nLHS, const enum CompType nCmpType, unsigned int nRHS );
extern ComparisonType ReadOperator( char*& pBufferInOut );
extern unsigned int ReadHits( char*& pBufferInOut );