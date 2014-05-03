#pragma once
#include <wtypes.h>
#include <vector>



#ifdef __cplusplus
extern "C" {
#endif

namespace
{
	enum CompVariableSize
	{
		CMP_SZ_1BIT_0,
		CMP_SZ_1BIT_1,
		CMP_SZ_1BIT_2,
		CMP_SZ_1BIT_3,
		CMP_SZ_1BIT_4,
		CMP_SZ_1BIT_5,
		CMP_SZ_1BIT_6,
		CMP_SZ_1BIT_7,
		CMP_SZ_4BIT_LOWER,
		CMP_SZ_4BIT_UPPER,
		CMP_SZ_8BIT,  
		CMP_SZ_16BIT,
		//CMP_SZ_32BIT,
		CMP_SZ__MAX
	};
	extern const char* g_MemSizeStrings[];
	extern const int g_NumMemSizeStrings;

	enum CompVariableType
	{
		CMPTYPE_ADDRESS = 0,	//	compare to the value of a live address in RAM
		CMPTYPE_VALUE,			//	a number. assume 32 bit 
		CMPTYPE_DELTAMEM,		//	the value last known at this address.
		CMPTYPE_DYNVAR,			//	a custom user-set variable
	};
	extern const char* g_MemTypeStrings[];
	extern const int g_NumMemTypeStrings;

	enum CompType
	{
		CMP_EQ = 0,
		CMP_LT,
		CMP_LTE,
		CMP_GT,
		CMP_GTE,
		CMP_NEQ,
		CMP__MAX
	};
	extern const char* g_CmpStrings[];
	extern const int g_NumCompTypes;

}

#ifdef __cplusplus
}
#endif

class CompVariable
{
public:
	CompVariable()
	{
		Clear();
	}

public:
	void Clear()
	{
		m_nVal = 0;
		m_nLastVal = 0;
		m_nVarSize = CMP_SZ_8BIT;
		m_nVarType = CMPTYPE_ADDRESS;
	}

	void ParseVariable( char*& sInString );	//	Parse from string
	unsigned int GetValue();						//	Returns the live value

public:
	CompVariableSize m_nVarSize;
	CompVariableType m_nVarType;
	unsigned int m_nVal;
	unsigned int m_nLastVal;
};


class Condition
{
public:
	Condition()
	{
		Clear();
	}

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

	inline unsigned int RequiredHits() const	{ return m_nRequiredHits; }
	inline unsigned int CurrentHits() const		{ return m_nCurrentHits; }
	inline CompVariable& CompSource()			{ return m_nCompSource; }
	inline CompType& ComparisonType()			{ return m_nComparison; }
	inline CompVariable& CompTarget()			{ return m_nCompTarget; }
	inline BOOL IsResetCondition() const		{ return m_bIsResetCondition; }
	inline BOOL IsPauseCondition() const		{ return m_bIsPauseCondition; }

	inline const CompVariable& CompSource() const	{ return m_nCompSource; }
	inline const CompVariable& CompTarget() const	{ return m_nCompTarget; }
	inline const CompType& ComparisonType() const	{ return m_nComparison; }

	void SetRequiredHits( unsigned int nHits )	{ m_nRequiredHits = nHits; }
	void IncrHits()								{ m_nCurrentHits++; }
	BOOL IsComplete() const						{ return (m_nCurrentHits >= m_nRequiredHits); }

	void OverrideCurrentHits(unsigned int nHits){ m_nCurrentHits = nHits; }

	void SetIsBasicCondition()					{ m_bIsResetCondition = FALSE; m_bIsPauseCondition = FALSE; }
	void SetIsPauseCondition()					{ m_bIsResetCondition = FALSE; m_bIsPauseCondition = TRUE; }
	void SetIsResetCondition()					{ m_bIsResetCondition = TRUE; m_bIsPauseCondition = FALSE; }

	void Set( const Condition& rRHS )			{ (*this) = rRHS; }

private:
	unsigned int	m_nRequiredHits;
	unsigned int	m_nCurrentHits;

	CompVariable	m_nCompSource;
	CompType		m_nComparison;
	CompVariable	m_nCompTarget;

	BOOL 			m_bIsResetCondition;
	BOOL 			m_bIsPauseCondition;
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

//extern BOOL CompareConditionValues( unsigned int nLHS, const enum CompType nCmpType, unsigned int nRHS );
extern enum CompType ReadOperator( char*& pBufferInOut );
extern unsigned int ReadHits( char*& pBufferInOut );