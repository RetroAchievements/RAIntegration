#include "RA_Achievement.h"

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <io.h>
#include <sstream>

#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_GameTitle.h"

#include "md5.h"
#include "RA_md5factory.h"

#include "RA_MemManager.h"
#include "RA_User.h"
#include "RA_PopupWindows.h"
#include "RA_httpthread.h"
#include "RA_Defs.h"
#include "RA_ImageFactory.h"
#include "RA_Core.h"
#include "RA_Leaderboard.h"
#include "RA_Condition.h"
#include "RA_RichPresence.h"

//	No game-specific code here please!

//////////////////////////////////////////////////////////////////////////

Achievement::Achievement( AchievementSetType nType ) :
	m_nSetType( nType )
{
	Clear();
	m_vConditions.push_back( ConditionSet() );
}

void Achievement::Parse( const Value& element )
{
	//{"ID":"36","MemAddr":"0xfe20>=50","Title":"Fifty Rings","Description":"Collect 50 rings","Points":"0","Author":"Scott","Modified":"1351868953","Created":"1351814592","BadgeName":"00083","Flags":"5"},
	m_nAchievementID = element["ID"].GetUint();
	m_sTitle = element["Title"].GetString();
	m_sDescription = element["Description"].GetString();
	m_nPointValue = element["Points"].GetUint();
	m_sAuthor = element["Author"].GetString();
	m_nTimestampModified = element["Modified"].GetUint();
	m_nTimestampCreated = element["Created"].GetUint();
	//m_sBadgeImageURI = element["BadgeName"].GetString();
	SetBadgeImage( element["BadgeName"].GetString() );
	//unsigned int nFlags = element["Flags"].GetUint();


	if( element["MemAddr"].IsString() )
	{
		std::string sMem = element["MemAddr"].GetString();
		char buffer[8192];
		sprintf_s( buffer, 8192, sMem.c_str() );
		ParseMemString( buffer );
	}

	SetActive( IsCoreAchievement() );	//	Activate core by default
}

char* Achievement::ParseMemString( char* sMem )
{
	size_t nCondGroupID = 0;
	char* pBuffer = sMem;

	do
	{
		std::vector<Condition> NewConditionGroup;

		do 
		{
			{
				while( (*pBuffer) == ' ' || (*pBuffer) == '_' || (*pBuffer) == '|' || (*pBuffer) == 'S' )
					pBuffer++; // Skip any chars up til the start of the achievement condition
			}
			
			Condition nNewCond;
			nNewCond.ParseFromString( pBuffer );
			NewConditionGroup.push_back( nNewCond );
		}
		while( *pBuffer == '_' || *pBuffer == 'R' || *pBuffer == 'P' ); //	AND, ResetIf, PauseIf

		for( size_t i = 0; i < NewConditionGroup.size(); ++i )
			AddCondition( nCondGroupID, NewConditionGroup[i] );

		nCondGroupID++;
	}
	while( *pBuffer == 'S' );	//	Repeat for all subconditions if they exist

	return pBuffer;
}

char* Achievement::ParseLine( char* pBuffer )
{
	char* pTitle = NULL;
	char* pDesc = NULL;
	char* pProgress = NULL;
	char* pProgressMax = NULL;
	char* pProgressFmt = NULL;
	char* pAuthor = NULL;
	char* pBadgeFilename = NULL;

	unsigned int nPoints = 0;
	time_t nDateCreatedSecs = 0;
	time_t nDateModifiedSecs = 0;
	unsigned short nUpvotes = 0;
	unsigned short nDownvotes = 0;
	
	//	Achievement:
	char* pNextChar = NULL;
	unsigned int nResetCondIter = 0;
	unsigned int nAchievementID = 0;
	unsigned int i = 0;

	if( pBuffer == NULL || pBuffer[0] == '\0' )
		return pBuffer;

	if( pBuffer[0] == '/' || pBuffer[0] == '\\' )
		return pBuffer;

	//	Read ID of achievement:

	nAchievementID = strtol( pBuffer, &pNextChar, 10 );
	pBuffer = pNextChar+1;

	//	
	pBuffer = ParseMemString( pBuffer );
	//	

	while( *pBuffer == ' ' || *pBuffer == ':' )
		pBuffer++; // Skip any whitespace/colons

	SetActive( FALSE );
	SetID( nAchievementID );

	//	buffer now contains TITLE : DESCRIPTION : $POINTS : $Author : $DateCreated : $DateModified : upvotes : downvotes : badgefilename
	
	pTitle				= _ReadStringTil( ':', pBuffer, TRUE );
	pDesc				= _ReadStringTil( ':', pBuffer, TRUE );
	pProgress			= _ReadStringTil( ':', pBuffer, TRUE );
	pProgressMax		= _ReadStringTil( ':', pBuffer, TRUE );
	pProgressFmt		= _ReadStringTil( ':', pBuffer, TRUE );
	pAuthor				= _ReadStringTil( ':', pBuffer, TRUE );
	
	nPoints =			(unsigned int)atol(   _ReadStringTil( ':', pBuffer, TRUE ) );
	nDateCreatedSecs =	(time_t)atol(		  _ReadStringTil( ':', pBuffer, TRUE ) );
	nDateModifiedSecs =	(time_t)atol(		  _ReadStringTil( ':', pBuffer, TRUE ) );
	nUpvotes =			(unsigned short)atol( _ReadStringTil( ':', pBuffer, TRUE ) );
	nDownvotes =		(unsigned short)atol( _ReadStringTil( ':', pBuffer, TRUE ) );

	pBadgeFilename		= _ReadStringTil( ':', pBuffer, TRUE );

	SetPoints( nPoints );
	SetCreatedDate( nDateCreatedSecs );
	SetModifiedDate( nDateModifiedSecs );
	SetTitle( pTitle );
	SetDescription( pDesc );
	SetAuthor( pAuthor );
	SetProgressIndicator( pProgress );
	SetProgressIndicatorMax( pProgressMax );
	SetProgressIndicatorFormat( pProgressFmt );
	//SetUpvotes( nUpvotes );
	//SetDownvotes( nDownvotes );
	SetBadgeImage( pBadgeFilename );
	
	return pBuffer;
}

BOOL Achievement::Test()
{
	BOOL bDirtyConditions = FALSE;
	BOOL bResetConditions = FALSE;

	BOOL bRetVal = FALSE;
	BOOL bRetValSubCond = NumConditionGroups() == 1 ? TRUE : FALSE;
	for( size_t i = 0; i < NumConditionGroups(); ++i )
	{
		if( i == 0 )
			bRetVal = m_vConditions[i].Test( bDirtyConditions, bResetConditions, FALSE );
		else	//	OR!
			bRetValSubCond |= m_vConditions[i].Test( bDirtyConditions, bResetConditions, FALSE );
	}

	if( bDirtyConditions )
	{
		SetDirtyFlag( Dirty_Conditions );
	}
	if( bResetConditions )
	{
		Reset();
	}

	return bRetVal && bRetValSubCond;
}

void Achievement::Clear()
{
	size_t i = 0;
	for( size_t i = 0; i < m_vConditions.size(); ++i )
		m_vConditions[i].Clear();

	//m_vConditions.clear();

	m_nAchievementID = 0;
	
	m_sTitle.clear();
	m_sDescription.clear();
	m_sAuthor.clear();
	m_sBadgeImageURI.clear();

	m_nPointValue = 0;
	m_bActive = FALSE;
	m_bModified = FALSE;
	ClearDirtyFlag();
	ClearBadgeImage();

	m_bProgressEnabled = FALSE;
	m_sProgress[0] = '\0';
	m_sProgressMax[0] = '\0';
	m_sProgressFmt[0] = '\0';
	m_fProgressLastShown = 0.0f;

	m_nTimestampCreated = 0;
	m_nTimestampModified = 0;
	//m_nUpvotes = 0;
	//m_nDownvotes = 0;
}

void Achievement::AddConditionGroup()
{
	m_vConditions.push_back( ConditionSet() );
}

void Achievement::RemoveConditionGroup()
{
	m_vConditions.pop_back();
}

void Achievement::SetID( unsigned int nID )
{ 
	m_nAchievementID = nID;
	SetDirtyFlag( Dirty_ID );
}

void Achievement::SetActive( BOOL bActive )
{
	if( m_bActive != bActive )
	{
		m_bActive = bActive;
		SetDirtyFlag( Dirty__All );
	}
}

//void Achievement::SetUpvotes( unsigned short nVal )
//{
//	m_nUpvotes = nVal;
//	SetDirtyFlag( Dirty_Votes );
//}
//
//void Achievement::SetDownvotes( unsigned short nVal )
//{
//	m_nDownvotes = nVal;
//	SetDirtyFlag( Dirty_Votes );
//}

void Achievement::SetModified( BOOL bModified )
{
	if( m_bModified != bModified )
	{
		m_bModified = bModified;
		SetDirtyFlag( Dirty__All );	//	TBD? questionable...
	}
}

void Achievement::SetBadgeImage( const std::string& sBadgeURI )
{
	SetDirtyFlag( Dirty_Badge );
	ClearBadgeImage();

	char chars[] = "_lock";

	std::string sNewBadgeURI = sBadgeURI;

	for (unsigned int i = 0; i < strlen(chars); ++i)
		sNewBadgeURI.erase(std::remove(sNewBadgeURI.begin(), sNewBadgeURI.end(), chars[i]), sNewBadgeURI.end());

	m_sBadgeImageURI = sNewBadgeURI;
	m_hBadgeImage = LoadOrFetchBadge(sNewBadgeURI, RA_BADGE_PX);

	if (sNewBadgeURI.find("_lock") == std::string::npos)	//	Ensure we're not going to look for _lock_lock
		m_hBadgeImageLocked = LoadOrFetchBadge(sNewBadgeURI + "_lock", RA_BADGE_PX);
}

void Achievement::Reset()
{
	//	Get all conditions, set hits found=0
	BOOL bDirty = FALSE;
	
	for( size_t i = 0; i < NumConditionGroups(); ++i )
		bDirty |= m_vConditions[i].Reset( false );

	if( bDirty )
		SetDirtyFlag( Dirty_Conditions );
}

size_t Achievement::AddCondition( size_t nConditionGroup, const Condition& rNewCond )
{ 
	while( NumConditionGroups() <= nConditionGroup )	//	ENSURE this is a legal op!
		m_vConditions.push_back( ConditionSet() );

	m_vConditions[nConditionGroup].Add( rNewCond );	//	NB. Copy by value	
	SetDirtyFlag( Dirty__All );

	return m_vConditions[nConditionGroup].Count();
}

BOOL Achievement::RemoveCondition( size_t nConditionGroup, unsigned int nID )
{
	m_vConditions[nConditionGroup].RemoveAt( nID );
	SetDirtyFlag( Dirty__All );	//	Not Conditions: 

	return TRUE;
}

void Achievement::RemoveAllConditions( size_t nConditionGroup )
{
	m_vConditions[nConditionGroup].Clear();

	SetDirtyFlag( Dirty__All );	//	All - not just conditions!
}

std::string Achievement::CreateMemString() const
{
	std::stringstream sstr;
	for( size_t nGrp = 0; nGrp < NumConditionGroups(); ++nGrp )
	{
		if( m_vConditions[ nGrp ].Count() == 0 )	//	Ignore empty groups when saving
			continue;
		
		if( nGrp > 0 )	//	Subcondition start found
			sstr << "S";

		for( size_t i = 0; i < m_vConditions[ nGrp ].Count(); ++i )
		{
			const Condition& NextCond = m_vConditions[ nGrp ].GetAt( i );
			const CompVariable& Src = NextCond.CompSource();
			const CompVariable& Target = NextCond.CompTarget();

			char sNextCondition[ 256 ];
			memset( sNextCondition, 0, 256 );
			
			if( NextCond.IsResetCondition() )
				strcat_s( sNextCondition, 256, "R:" );
			else if( NextCond.IsPauseCondition() )
				strcat_s( sNextCondition, 256, "P:" );
			
			//	Source:
			if( ( Src.Type() == Address ) || 
				( Src.Type() == DeltaMem ) )
			{
				char buffer1[ 64 ];
				sprintf_s( buffer1, 64, "%s0x%s%06x", ( Src.Type() == DeltaMem ) ? "d" : "", ComparisonSizeToPrefix( Src.Size() ), Src.RawValue() );
				strcat_s( sNextCondition, 256, buffer1 );
			}
			else if( Src.Type() == ValueComparison )
			{
				//	Value: use direct!
				char buffer2[ 64 ];
				sprintf_s( buffer2, 64, "%d", Src.RawValue() );
				strcat_s( sNextCondition, 256, buffer2 );
			}
			else
			{
				ASSERT( !"Unknown type? (DynMem)?" );
			}
			
			//	Comparison type:
			strcat_s( sNextCondition, 256, ComparisonTypeToStr( NextCond.CompareType() ) );

			//	Target:
			if( ( Target.Type() == Address ) || 
				( Target.Type() == DeltaMem ) )
			{
				char buffer3[ 64 ];
				sprintf_s( buffer3, 64,
						   "%s%s%06x",
						   ( Target.Type() == DeltaMem ) ? "d0x" : "0x",
						   ComparisonSizeToPrefix( Target.Size() ), 
						   Target.RawValue() );

				strcat_s( sNextCondition, 256, buffer3 );
			}
			else if( Target.Type() == ValueComparison )
			{
				//	Value: use direct!
				char buffer4[ 64 ];
				sprintf_s( buffer4, 64, "%d", Target.RawValue() );
				strcat_s( sNextCondition, 256, buffer4 );
			}
			else
			{
				ASSERT( !"Unknown type? (DynMem)?" );
			}
			
			//	Hit count:
			if( NextCond.RequiredHits() > 0 )
			{
				char buffer5[ 64 ];
				sprintf_s( buffer5, 64, ".%d.", NextCond.RequiredHits() );
				strcat_s( sNextCondition, 256, buffer5 );
			}
			
			//	Copy in the next condition:
			sstr << sNextCondition;

			//	Are we on the last condition? THIS IS IMPORTANT: check the parsing code!
			if( ( i + 1 ) < m_vConditions[nGrp].Count() )
				sstr << "_";
		}
	}

	return sstr.str();
}

void Achievement::ClearBadgeImage()
{
	if( m_hBadgeImage != NULL )
	{
		DeleteObject( m_hBadgeImage );
		m_hBadgeImage = NULL;
	}
	if( m_hBadgeImageLocked != NULL )
	{
		DeleteObject( m_hBadgeImageLocked );
		m_hBadgeImageLocked = NULL;
	}
}

void Achievement::Set( const Achievement& rRHS )
{
	ClearBadgeImage();
	SetID( rRHS.m_nAchievementID );
	SetActive( rRHS.m_bActive );
	SetAuthor( rRHS.m_sAuthor );
	SetDescription( rRHS.m_sDescription );
	SetPoints( rRHS.m_nPointValue );
	SetTitle( rRHS.m_sTitle );
	SetModified( rRHS.m_bModified );
	SetBadgeImage( rRHS.m_sBadgeImageURI );
	
	//	TBD: move to 'now'?
	SetModifiedDate( rRHS.m_nTimestampModified );
	SetCreatedDate( rRHS.m_nTimestampCreated );

	for( size_t i = 0; i < NumConditionGroups(); ++i )
		RemoveAllConditions( i );
	m_vConditions.clear();

	for( size_t nGrp = 0; nGrp < rRHS.NumConditionGroups(); ++nGrp )
	{
		m_vConditions.push_back( ConditionSet() );

		for( size_t i = 0; i < rRHS.m_vConditions[nGrp].Count(); ++i )
			AddCondition( nGrp, rRHS.m_vConditions[nGrp].GetAt(i) );
	}

	SetDirtyFlag( Dirty__All );
}

//int Achievement::StoreDynamicVar( char* pVarName, CompVariable nVar )
//{
//	ASSERT( m_nNumDynamicVars < 5 );
// 	
//	//m_nDynamicVars[m_nNumDynamicVars].m_nVar = nVar;
//	//strcpy_s( m_nDynamicVars[m_nNumDynamicVars].m_sName, 16, pVarName );
// 	
//	m_nNumDynamicVars++;
// 
//	return m_nNumDynamicVars;
//}
//
//
//float Achievement::ProgressGetNextStep( char* sFormat, float fLastKnownProgress )
//{
//	const float fStep = ( (float)strtol( sFormat, NULL, 10 ) / 100.0f );
//	int nIter = 1;	//	Progress of 0% doesn't require a popup...
//	float fStepAt = fStep * nIter;
//	while( (fLastKnownProgress >= fStepAt) && (fStepAt < 1.0f) && (nIter < 20) )
//	{
//		nIter++;
//		fStepAt = fStep * nIter;
//	}
// 
//	if( fStepAt >= 1.0f )
//		fStepAt = 1.0f;
// 
//	return fStepAt;
//}
//
//float Achievement::ParseProgressExpression( char* pExp )
//{
//	if( pExp == NULL )
//		return 0.0f;
// 
//	int nOperator = 0;	//	0=add, 1=mult, 2=sub
// 
//	while( *pExp == ' ' )
//		++pExp; //	Trim
// 
//	float fProgressValue = 0.0f;
//	char* pStrIter = &pExp[0];
// 
//	int nIterations = 0;
// 
//	while( *pStrIter != NULL && nIterations < 20 )
//	{
//		float fNextVal = 0.0f;
// 
//		//	Parse operator
//		if( pStrIter == pExp )
//		{
//			//	Start of string: assume operator of 'add'
//			nOperator = 0;
//		}
//		else
//		{
//			if( *pStrIter == '+' )
//				nOperator = 0;
//			else if( *pStrIter == '*' )
//				nOperator = 1;
//			else if( *pStrIter == '-' )
//				nOperator = 2;
//			else
//			{
//				char buffer[256];
//				sprintf_s( buffer, 256, "Unrecognised operator character at %d",
//					(&pStrIter) - (&pExp) );
// 
//				ASSERT(!"Unrecognised operator in format expression!");
//				return 0.0f;
//			}
//			
//			pStrIter++;
//		}
// 
//		//	Parse value:
//		if( strncmp( pStrIter, "Cond:", 5 ) == 0 )
//		{
//			//	Get the specified condition, and the value from it.
//			unsigned int nCondIter = strtol( pStrIter+5, NULL, 10 );
//			
//			if( nCondIter < NumConditions() )
//			{
//				Condition& Cond = GetCondition( nCondIter );
//				if( Cond.CompSource().m_nVarType != CMPTYPE_VALUE )
//				{
//					fNextVal = (float)Cond.CompSource().m_nVal;
//				}
//				else if( Cond.CompTarget().m_nVarType != CMPTYPE_VALUE )
//				{
//					//	Best guess?
//					fNextVal = (float)Cond.CompTarget().m_nVal;
//				}
//				else
//				{
//					//wtf? Both elements are 'value'?
//					ASSERT(!"Bolloxed");
//					fNextVal = 0.0f;
//				}
//			}
//			else
//			{
//				//	Report error?
//				ASSERT(!"Bolloxed2");
//				return 0.0f;
//			}
//		}
//		else if( pStrIter[0] == '0' && pStrIter[1] == 'x' )
//		{
//			CompVariable Var;							//	Memory location
//			Var.ParseVariable( pStrIter );				//	Automatically pushes pStrIter
//			fNextVal = (float)( Var.GetValue() );
//		}
//		else
//		{
//			//	Assume value, assume base 10
//			fNextVal = (float)( strtol( pStrIter, &pStrIter, 10 ) );
//		}
// 
//		switch( nOperator )
//		{
//		case 0:	//	Add
//			fProgressValue += fNextVal;
//			break;
//		case 1:	//	Mult
//			fProgressValue *= fNextVal;
//			break;
//		case 2:	//	Sub
//			fProgressValue -= fNextVal;
//			break;
//		default:
//			ASSERT(!"Unrecognised operator?!");
//			break;
//		}
// 
//		while( *pExp == ' ' )
//			++pExp; //	Trim any whitespace
// 
//		nIterations++;
//	}
// 
//	if( nIterations == 20 )
//	{
//		ASSERT(!"Bugger... can't parse this 'progress' thing. Too many iterations!");
//	}
// 
//	return fProgressValue;
//}

//void Achievement::UpdateProgress()
// {
//	//	Produce a float which represents the current progress.
//	//	Compare to m_sProgressFmt. If greater and 
//	//	Compare to m_fProgressLastShown
// 
//	//	TBD: Don't forget backslashes!
//	//sprintf_s( m_sProgress, 256,	"0xfe20", 10 );	//	every 10 percent
//	//sprintf_s( m_sProgressMax, 256, "200", 10 );	//	every 10 percent
//	//sprintf_s( m_sProgressFmt, 50,	"%d,%%01.f,%%01.f,,", 10 );	//	every 10 percent
//	
//	const float fProgressValue = ParseProgressExpression( m_sProgress );
//	const float fMaxValue = ParseProgressExpression( m_sProgressMax );
// 
//	if( fMaxValue == 0.0f )
//		return;
// 
//	const float fProgressPercent = ( fProgressValue / fMaxValue );
//	const float fNextStep = ProgressGetNextStep( m_sProgressFmt, fProgressPercent );
// 
//	if( fProgressPercent >= m_fProgressLastShown && m_fProgressLastShown < fNextStep )
//	{
//		if( m_fProgressLastShown == 0.0f )
//		{
//			m_fProgressLastShown = fNextStep;
//			return;	//	Don't show first indicator!
//		}
//		else if( fNextStep == 1.0f )
//		{
//			return;	//	Don't show final indicator!
//		}
// 
//		m_fProgressLastShown = fNextStep;
//		
//		char formatSpareBuffer[256];
//		strcpy_s( formatSpareBuffer, 256, m_sProgressFmt );
// 
//		//	DO NOT USE Sprintf!! this will interpret the format!
//		//	sprintf_s( formatSpareBuffer, 256, m_sProgressFmt ); 
// 
//		char* pUnused = &formatSpareBuffer[0];
//		char* pUnused2 = NULL;
//		char* pFirstFmt = NULL;
//		char* pSecondFmt = NULL;
//		strtok_s( pUnused, ",", &pFirstFmt );
//		strtok_s( pFirstFmt, ",", &pSecondFmt );
//		strtok_s( pSecondFmt, ",\0\n:", &pUnused2 );	//	Adds a very useful '\0' to pSecondFmt 
//		
//		if( pFirstFmt==NULL || pSecondFmt==NULL )
//		{
//			ASSERT(!"FUCK. Format string is fucked");
//			return;
//		}
//		
//		//	Display progress message:
//		
//		char bufferVal1[64];
//		sprintf_s( bufferVal1, 64, pFirstFmt, fProgressValue );
//		
//		char bufferVal2[64];
//		sprintf_s( bufferVal2, 64, pSecondFmt, fMaxValue );
// 
//		char progressTitle[256];
//		sprintf_s( progressTitle, 256, " Progress: %s ", Title() );
// 
//		char progressDesc[256];
//		sprintf_s( progressDesc, 256, " %s of %s ", bufferVal1, bufferVal2 );
//		g_PopupWindows.ProgressPopups().AddMessage( progressTitle, progressDesc );
//	}
// }
