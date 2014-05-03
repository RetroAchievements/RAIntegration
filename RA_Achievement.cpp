#include "RA_Achievement.h"

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <io.h>

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

AchievementSet* CoreAchievements = NULL;
AchievementSet* UnofficialAchievements = NULL;
AchievementSet* LocalAchievements = NULL;

AchievementType g_nActiveAchievementSet = AT_CORE;
AchievementSet* g_pActiveAchievements = CoreAchievements;

namespace
{
	const char* LockedBadge = "00000";
	const char* LockedBadgeFile = "00000.png";
}

void SetAchievementCollection( AchievementType Type )
{
	g_nActiveAchievementSet = Type;
	switch( Type )
	{
	case AT_CORE:
		g_pActiveAchievements = CoreAchievements;
		break;
	case AT_UNOFFICIAL:
		g_pActiveAchievements = UnofficialAchievements;
		break;
	case AT_USER:
		g_pActiveAchievements = LocalAchievements;
		break;
	default:
		assert(0);
		break;
	}

	//	Update TONS of dialogs :S
	//g_AchievementsDialog.OnLoad_NewRom();
	//AchievementEditorDialog.OnLoad_NewRom();
}

//////////////////////////////////////////////////////////////////////////

Achievement::Achievement()
{
	Clear();
	m_vConditions.push_back( ConditionSet() );
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

	size_t nCondGroupID = 0;
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

		for( i = 0; i < NewConditionGroup.size(); ++i )
			AddCondition( nCondGroupID, NewConditionGroup[i] );

		nCondGroupID++;
	}
	while( *pBuffer == 'S' );	//	Repeat for all subconditions if they exist

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
	SetUpvotes( nUpvotes );
	SetDownvotes( nDownvotes );
	SetBadgeImage( pBadgeFilename );
	
	return pBuffer;
}

void Achievement::SetID( unsigned int nID )
{ 
	m_nAchievementID = nID;
	SetDirtyFlag( Dirty_ID );
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

	m_sTitle[0] = '\0';
	m_sDescription[0] = '\0';
	m_sAuthor[0] = '\0';
	strcpy_s( m_sBadgeImageFilename, 16, LockedBadge );

	m_nPointValue = 0;
	m_bActive = FALSE;
	m_bModified = FALSE;
	ClearDirtyFlag();
	ClearBadgeImage();

	m_nInternalArrayOffset = 0;
	m_nNumDynamicVars = 0;
	m_bProgressEnabled = FALSE;
	m_sProgress[0] = '\0';
	m_sProgressMax[0] = '\0';
	m_sProgressFmt[0] = '\0';
	m_fProgressLastShown = 0.0f;

	m_nTimestampCreated = 0;
	m_nTimestampModified = 0;
	m_nUpvotes = 0;
	m_nDownvotes = 0;
}


void Achievement::AddConditionGroup()
{
	m_vConditions.push_back( ConditionSet() );
}

void Achievement::RemoveConditionGroup()
{
	m_vConditions.pop_back();
}

void Achievement::SetActive( BOOL bActive )
{
	if( m_bActive != bActive )
	{
		m_bActive = bActive;
		SetDirtyFlag( Dirty__All );
	}
}

void Achievement::SetAuthor( const char* sAuthor )
{
	strcpy_s( m_sAuthor, 255, sAuthor );
	//SetDirtyFlag();
}

void Achievement::SetTitle( const char* sTitle )
{
	strcpy_s( m_sTitle, 255, sTitle );
	//SetDirtyFlag();
}

void Achievement::SetDescription( const char* sDescription )
{
	strcpy_s( m_sDescription, 255, sDescription );
	//SetDirtyFlag();
}

void Achievement::SetPoints( unsigned int nPoints )
{
	m_nPointValue = nPoints;
}

void Achievement::SetProgressIndicator( const char* sProgress )
{
	strcpy_s( m_sProgress, 256, sProgress );
}

void Achievement::SetProgressIndicatorMax( const char* sProgressMax )
{
	strcpy_s( m_sProgressMax, 256, sProgressMax );
}

void Achievement::SetProgressIndicatorFormat( const char* sProgressFmt )
{
	strcpy_s( m_sProgressFmt, 50, sProgressFmt );
}

void Achievement::SetUpvotes( unsigned short nVal )
{
	m_nUpvotes = nVal;
	SetDirtyFlag( Dirty_Votes );

// 	if( bUpdateDlg )
// 	{
// 		//	Reverse find where I am in the list:
// 		int nOffset = 0;
// 		for( ; nOffset < pActiveAchievements->Count(); ++nOffset )
// 		{
// 			if( this == &pActiveAchievements->GetRef( nOffset ) )
// 				break;
// 		}
// 
// 		assert( nOffset < pActiveAchievements->Count() );
// 		if( nOffset < pActiveAchievements->Count() )
// 		{
// 			char buffer[256];
// 			sprintf_s( buffer, 256, "%d/%d", m_nUpvotes, m_nDownvotes );
// 
// 			g_AchievementsDialog.OnEditData( nOffset, Dlg_Achievements::Votes, buffer );
// 		}
// 	}
}

void Achievement::SetDownvotes( unsigned short nVal )
{
	m_nDownvotes = nVal;
	SetDirtyFlag( Dirty_Votes );

// 	if( bUpdateDlg )
// 	{
// 		//	Reverse find where I am in the list:
// 		int nOffset = 0;
// 		for( ; nOffset < pActiveAchievements->Count(); ++nOffset )
// 		{
// 			if( this == &pActiveAchievements->GetRef( nOffset ) )
// 				break;
// 		}
// 
// 		assert( nOffset < pActiveAchievements->Count() );
// 		if( nOffset < pActiveAchievements->Count() )
// 		{
// 			char buffer[256];
// 			sprintf_s( buffer, 256, "%d/%d", m_nUpvotes, m_nDownvotes );
// 
// 			g_AchievementsDialog.OnEditData( nOffset, Dlg_Achievements::Votes, buffer );
// 		}
// 	}
}

void Achievement::SetCreatedDate( time_t nTimeCreated )
{
	m_nTimestampCreated = nTimeCreated;
}

void Achievement::SetModifiedDate( time_t nTimeModified )
{
	m_nTimestampModified = nTimeModified;
}

void Achievement::SetModified( BOOL bModified )
{
	if( m_bModified != bModified )
	{
		m_bModified = bModified;
		SetDirtyFlag( Dirty__All );	//	TBD? questionable...
	}
}

void Achievement::SetBadgeImage( const char* sFilename )
{
	char buffer[256];
	SetDirtyFlag( Dirty_Badge );
	ClearBadgeImage();

	strcpy_s( m_sBadgeImageFilename, 16, sFilename );

	sprintf_s( buffer, 256, RA_DIR_BADGE "%s.png", sFilename );
	
	//	Blocking :S
	m_hBadgeImage = LoadLocalPNG( buffer, 64, 64 );

	sprintf_s( buffer, 256, RA_DIR_BADGE "%s_lock.png", sFilename );
	//	Blocking :S
	m_hBadgeImageLocked = LoadLocalPNG( buffer, 64, 64 );
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

int Achievement::CreateMemString( char* pStrOut, const int nNumChars )
{
	unsigned int i = 0;
	char sNextCondition[256];
	char* sDelta = "";
	char* sSize = "";

	pStrOut[0] = '\0';

	for( size_t nGrp = 0; nGrp < NumConditionGroups(); ++nGrp )
	{
		if( m_vConditions[nGrp].Count() == 0 )	//Ignore empty groups when saving
			continue;
		
		if( nGrp > 0 )	//	Subcondition start found
			strcat_s( pStrOut, nNumChars, "S" );

		for( i = 0; i < m_vConditions[nGrp].Count(); ++i )
		{
			Condition* pNextCond = &m_vConditions[nGrp].GetAt(i);
			memset( sNextCondition, 0, 256 );

			//	Source:
			sDelta = (pNextCond->CompSource().m_nVarType==CMPTYPE_DELTAMEM) ? "d" : "";
			sSize = "";

			if( pNextCond->IsResetCondition() )
				strcat_s( sNextCondition, 256, "R:" );
			else if( pNextCond->IsPauseCondition() )
				strcat_s( sNextCondition, 256, "P:" );

			if( pNextCond->CompSource().m_nVarType != CMPTYPE_VALUE )
			{
				if( pNextCond->CompSource().m_nVarSize == CMP_SZ_4BIT_LOWER )		//	L=lower
					sSize = "L";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_4BIT_UPPER )	//	U=upper
					sSize = "U";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_8BIT )		//	H=half
					sSize = "H";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_0 )		//	MNOPQRST=bits
					sSize = "M";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_1 )
					sSize = "N";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_2 )
					sSize = "O";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_3 )
					sSize = "P";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_4 )
					sSize = "Q";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_5 )
					sSize = "R";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_6 )
					sSize = "S";
				else if( pNextCond->CompSource().m_nVarSize == CMP_SZ_1BIT_7 )
					sSize = "T";
				else //if( pNextCond->CompSource().m_nVarSize == CMP_SZ_16BIT )
					sSize = "";

				if( g_MemManager.RAMTotalSize() > 65536 )
					sprintf_s( sNextCondition, 256, "%s%s0x%s%06x", sNextCondition, sDelta, sSize, pNextCond->CompSource().m_nVal );
				else
					sprintf_s( sNextCondition, 256, "%s%s0x%s%04x", sNextCondition, sDelta, sSize, pNextCond->CompSource().m_nVal );
			}
			else
			{
				//	Value: use direct!
				sprintf_s( sNextCondition, 256, "%s%d", sNextCondition, pNextCond->CompSource().m_nVal );
			}
			
			//	Comparison type:
			char* sCmpType = "=";
			switch( pNextCond->ComparisonType() )
			{
			case CMP_EQ:
				sCmpType = "="; break;
			case CMP_GT:
				sCmpType = ">"; break;
			case CMP_GTE:
				sCmpType = ">="; break;
			case CMP_LT:
				sCmpType = "<"; break;
			case CMP_LTE:
				sCmpType = "<="; break;
			case CMP_NEQ:
				sCmpType = "!="; break;
			default:
				assert(0); break;
			}
			strcat_s( sNextCondition, 256, sCmpType );

			//	Target:
			sDelta = (pNextCond->CompTarget().m_nVarType==CMPTYPE_DELTAMEM) ? "d" : "";
			sSize = "";

			if( pNextCond->CompTarget().m_nVarType != CMPTYPE_VALUE )
			{
				if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_4BIT_LOWER )		//	L=lower
					sSize = "L";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_4BIT_UPPER )	//	U=upper
					sSize = "U";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_8BIT )		//	H=half		
					sSize = "H";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_0 )		//	MNOPQRST=bits
					sSize = "M";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_1 )
					sSize = "N";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_2 )
					sSize = "O";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_3 )
					sSize = "P";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_4 )
					sSize = "Q";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_5 )
					sSize = "R";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_6 )
					sSize = "S";
				else if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_1BIT_7 )
					sSize = "T";
				else //if( pNextCond->CompTarget().m_nVarSize == CMP_SZ_16BIT )
					sSize = "";

				if( g_MemManager.RAMTotalSize() > 65536 )
					sprintf_s( sNextCondition, 256, "%s%s0x%s%06x", sNextCondition, sDelta, sSize, pNextCond->CompTarget().m_nVal );
				else
					sprintf_s( sNextCondition, 256, "%s%s0x%s%04x", sNextCondition, sDelta, sSize, pNextCond->CompTarget().m_nVal );
			}
			else
			{
				//	Value: use direct!
				sprintf_s( sNextCondition, 256, "%s%d", sNextCondition, pNextCond->CompTarget().m_nVal );
			}

			//	Hit count:
			
			if( pNextCond->RequiredHits() > 0 )
				sprintf_s( sNextCondition, 256, "%s.%d.", sNextCondition, pNextCond->RequiredHits() );
			
			//	Are we on the last condition? THIS IS IMPORTANT: check the parsing code!
			if( (i+1) < m_vConditions[nGrp].Count() )
				strcat_s( sNextCondition, 256, "_" );

			//	Copy in the next condition:
			strcat_s( pStrOut, nNumChars, sNextCondition );
		}
	}

	return strlen( pStrOut );
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
	SetUpvotes( rRHS.m_nUpvotes );
	SetDownvotes( rRHS.m_nDownvotes );
	SetBadgeImage( rRHS.m_sBadgeImageFilename );
	
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
//	assert( m_nNumDynamicVars < 5 );
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
//				assert(!"Unrecognised operator in format expression!");
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
//					assert(!"Bolloxed");
//					fNextVal = 0.0f;
//				}
//			}
//			else
//			{
//				//	Report error?
//				assert(!"Bolloxed2");
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
//			assert(!"Unrecognised operator?!");
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
//		assert(!"Bugger... can't parse this 'progress' thing. Too many iterations!");
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
//			assert(!"FUCK. Format string is fucked");
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

////////////////////////////////////////////////////////////////////////// 
//	static
void AchievementSet::GetAchievementsFilename( unsigned int nAchievementSet, unsigned int nGameID, char* pCharOut, int nNumChars )
{
	switch( nAchievementSet )
	{
	case AT_CORE:
		sprintf_s( pCharOut, nNumChars, "%s%d.txt", RA_DIR_DATA, nGameID );
		break;
	case AT_UNOFFICIAL:
		sprintf_s( pCharOut, nNumChars, "%s%d-Unofficial.txt", RA_DIR_DATA, nGameID );
		break;
	case AT_USER:
		sprintf_s( pCharOut, nNumChars, "%s%d-User.txt", RA_DIR_DATA, nGameID );
		break;
	default:
		assert(0);
		break;
	}
}

//	static
BOOL AchievementSet::DeletePatchFile( unsigned int nAchievementSet, unsigned int nGameID )
{
	if( nGameID != 0 )
	{
		char buffer[2048];
		GetAchievementsFilename( nAchievementSet, nGameID, buffer, 2048 );
							
		//	Remove the text file
		SetCurrentDirectory( g_sHomeDir );
		if( _access( buffer, 06 ) != -1 )	//	06= Read/write permission
		{
			if( remove( buffer ) == -1 )
			{
				//	Remove issues?
				assert(FALSE);
				return FALSE;
			}
		}

		return TRUE;
	}
	else
	{
		//	m_nGameID == 0, therefore there will be no patch file.
		return TRUE;
	}
}

Achievement& AchievementSet::AddAchievement()
{
	Achievement& retVal = m_Achievements[m_nNumAchievements];
	m_nNumAchievements++; 

	//	Clean before returning it
	retVal.Clear();
	return retVal;
}

BOOL AchievementSet::RemoveAchievement( unsigned int nIter )
{
	unsigned int i = 0;
	//	Note: need to remember this will have drastic effects on all that
	//	 rely on the linear order of this object!
	//MessageBox( NULL, "NOTIMPL", "ERROR", MB_OK );
	//	Reverse shuffle, like condition removal

	if( nIter >= m_nNumAchievements )
	{
		assert( 0 );
		return FALSE;
	}

	m_Achievements[nIter].ClearBadgeImage();

	for( i = nIter; i < m_nNumAchievements-1; ++i )
		m_Achievements[ i ] = m_Achievements[ i+1 ];

	m_nNumAchievements--;

	return TRUE;
}

Achievement* AchievementSet::Find( unsigned int nID )
{
	unsigned int i = 0;

	for( i = 0; i < m_nNumAchievements; ++i )
	{
		if( m_Achievements[i].ID() == nID )
			return &m_Achievements[i];
	}

	return NULL;
}

unsigned int AchievementSet::NumActive()
{
	unsigned int nNumActive = 0;
	unsigned int i = 0;
	for( i = 0; i < m_nNumAchievements; ++i )
	{
		if( m_Achievements[i].Active() )
			nNumActive++;
	}
	return nNumActive;
}

void AchievementSet::Clear()
{
	const unsigned int nNumAchievements = ( sizeof(m_Achievements) / sizeof(m_Achievements[0]) );
	unsigned int i = 0;
	for( i = 0; i < nNumAchievements; ++i )
		m_Achievements[i].Clear();

	m_nNumAchievements = 0;
	m_nGameID = 0;
	m_bProcessingActive = TRUE;
	m_sPreferredGameTitle[0] = '\0';
}

void AchievementSet::Test()
{
	unsigned int i = 0;
	unsigned int nOffset = 0;
	unsigned long nCharsRead = 0;
	char sTitle[1024];
	char sSubtitle[1024];
	char sValidation[50];
	char sUserDetails[512];

	if( !m_bProcessingActive )
		return;

	for( i = 0; i < m_nNumAchievements; ++i )
	{
		Achievement* pAch = &m_Achievements[i];
		if( !pAch->Active() )
			continue;

		//Cheevo.UpdateProgress();

		if( pAch->Test() == TRUE )
		{
			//	Award. If can award or have already awarded, set inactive:
			pAch->SetActive( FALSE );

			//	Reverse find where I am in the list:
 			for( nOffset = 0; nOffset < g_pActiveAchievements->Count(); ++nOffset )
 			{
 				if( pAch == &g_pActiveAchievements->GetAchievement( nOffset ) )
 					break;
 			}
 			assert( nOffset < Count() );
 			if( nOffset < Count() )
				g_AchievementsDialog.ReloadLBXData( nOffset );

			if( g_LocalUser.m_bIsLoggedIn )
			{
				if( pAch->ID() == 0 )
				{
					//g_AchievementsDialog.OnGet_Achievement( i );

					sprintf_s( sTitle, 1024, " Test: Achievement Unlocked " );
					sprintf_s( sSubtitle, 1024, " %s (%d) (Unofficial) ", pAch->Title(), pAch->Points() );
					g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_UNLOCKED, pAch->BadgeImage() );
				}
				else if( pAch->Modified() )
				{
					//g_AchievementsDialog.OnGet_Achievement( i );
					
					sprintf_s( sTitle, 1024, " Modified: Achievement Unlocked " );
					sprintf_s( sSubtitle, 1024, " %s (%d) (Unofficial) ", pAch->Title(), pAch->Points() );
					g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_UNLOCKED, pAch->BadgeImage() );
				}
				else if( g_fnDoValidation == NULL )
				{
					//g_AchievementsDialog.OnGet_Achievement( i );
					
					sprintf_s( sTitle, 1024, " (Missing RA_Keys.DLL): Achievement Unlocked " );
					sprintf_s( sSubtitle, 1024, " %s (%d) (Unofficial) ", pAch->Title(), pAch->Points() );
					g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_ERROR, pAch->BadgeImage() );
				}
				else if( g_bRAMTamperedWith )
				{
					sprintf_s( sTitle, 1024, " (RAM tampered with!): Achievement Unlocked " );
					sprintf_s( sSubtitle, 1024, " %s (%d) (Unofficial) ", pAch->Title(), pAch->Points() );
					g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_ACHIEVEMENT_ERROR, pAch->BadgeImage() );
				}
				else
				{
					g_fnDoValidation( sValidation, g_LocalUser.m_sUsername, g_LocalUser.m_sToken, pAch->ID() );

					sprintf_s( sUserDetails, 512, "u=%s&t=%s&a=%d&v=%s&h=%d",
						g_LocalUser.m_sUsername, 
						g_LocalUser.m_sToken, 
						pAch->ID(),
						sValidation,
						g_hardcoreModeActive );
					
					CreateHTTPRequestThread( "requestachievement.php", sUserDetails, HTTPRequest_Post, i, NULL );
				}
			}
		}
	}
}

BOOL AchievementSet::Save()
{
	//	Takes all achievements in this group and dumps them in the filename provided.
	FILE* pFile = NULL;
	char sNextLine[2048];
	char sMem[2048];

	char sFilename[1024];
	unsigned int i = 0;

	GetAchievementsFilename( m_nType, m_nGameID, sFilename, 1024 );

	fopen_s( &pFile, sFilename, "w" );
	if( pFile != NULL )
	{
		sprintf_s( sNextLine, 2048, "0.022\n" );						//	Min ver
		fwrite( sNextLine, sizeof(char), strlen(sNextLine), pFile );

		sprintf_s( sNextLine, 2048, "%s\n", m_sPreferredGameTitle );
		fwrite( sNextLine, sizeof(char), strlen(sNextLine), pFile );

		for( i = 0; i < m_nNumAchievements; ++i )
		{
			Achievement* pAch = &m_Achievements[i];

			ZeroMemory( sMem, 2048 );
			pAch->CreateMemString( sMem, 2048 );

			ZeroMemory( sNextLine, 2048 );
			sprintf_s( sNextLine, 2048, "%d:%s:%s:%s:%s:%s:%s:%s:%d:%lu:%lu:%d:%d:%s\n",
				pAch->ID(),
				sMem,
				pAch->Title(),
				pAch->Description(),
				" ", //Ach.ProgressIndicator()=="" ? " " : Ach.ProgressIndicator(),
				" ", //Ach.ProgressIndicatorMax()=="" ? " " : Ach.ProgressIndicatorMax(),
				" ", //Ach.ProgressIndicatorFormat()=="" ? " " : Ach.ProgressIndicatorFormat(),
				pAch->Author(),
				(unsigned short)pAch->Points(),
				(unsigned long)pAch->CreatedDate(),
				(unsigned long)pAch->ModifiedDate(),
				(unsigned short)pAch->Upvotes(),
				(unsigned short)pAch->Downvotes(),
				pAch->BadgeImageFilename() );

			fwrite( sNextLine, sizeof(char), strlen(sNextLine), pFile );
		}

		fclose( pFile );
		return TRUE;
	}

	return FALSE;
}

BOOL AchievementSet::Load( const unsigned int nGameID )
{
	char sFilename[1024];
	FILE* pFile = NULL;
	const char EndLine = '\n';
	char buffer[4096];
	unsigned int nNumAchievementsLoaded = 0;
	unsigned int nTotalPoints = 0;
	unsigned int i = 0;
	char sPostData[512];
	char bufferOut[32768];
	char* pBufferOut = NULL;
	DWORD nCharsRead = 0;
	int nNumUnlocked = 0;
	unsigned int nID = 0;
	char sMessage[512];
	unsigned int nFlags = 0;
	long nCurrentVer = strtol( g_sClientVersion+2, NULL, 10 );
	long nMinVer = 0;

	//	Clear everything. If we have no gameID, just return. If we have one, set it, then attempt to load.
	Clear();

	if( nGameID == 0 )
		return TRUE;

	GetAchievementsFilename( m_nType, nGameID, sFilename, 1024 );

	SetCurrentDirectory( g_sHomeDir );
	fopen_s( &pFile, sFilename, "r" );
	if( pFile != NULL )
	{
		//	Store this: we are now assuming this is the correct checksum if we have a file for it
		m_nGameID = nGameID;

		//	Get min ver:
		_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
		//	UNUSED at this point? TBD

		//	Get game title:
		_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
		if( nCharsRead > 0 )
		{
			buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string
			strcpy_s( m_sPreferredGameTitle, 64, buffer );
		}

		while( !feof(pFile) )
		{
			ZeroMemory( buffer, 4096 );
			_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
			if( nCharsRead > 0 )
			{
				if( buffer[0] == 'L' )
				{
					RA_Leaderboard lb;
					lb.ParseLine( buffer );

					g_LeaderboardManager.AddLeaderboard( lb );
				}
				else if( isdigit( buffer[0] ) )
				{
					Achievement& newAch = AddAchievement();
					buffer[nCharsRead-1] = '\0';	//	Turn that endline into an end-string

					newAch.ParseLine( buffer );

					if( m_nType == AT_CORE )
					{
						newAch.SetActive( TRUE );
					}
					else
					{
						//	Set all unofficial achievements deactivated by default
						newAch.SetActive( FALSE );
					}

					nNumAchievementsLoaded++;
				}
			}
		}

		fclose( pFile );
		
		
		if( m_nType == AT_CORE )
		{
			//	Fire off a request to get the latest rich presence info
			char bufferPost[256];
			sprintf_s( bufferPost, 256, "g=%d", m_nGameID );
			CreateHTTPRequestThread( "requestrichpresence.php", bufferPost, HTTPRequest_Post, m_nGameID, NULL );
		}

		if( nNumAchievementsLoaded > 0 )
		{
			if( m_nType == AT_CORE )
			{
				for( i = 0; i < m_nNumAchievements; ++i )
					nTotalPoints += m_Achievements[i].Points();

				//g_PopupWindows.AchievementPopups().AddMessage( buffer, NULL, MSG_INFO );	//TBD
			}
		}

		//	Loaded OK: now query against player:

		if( m_nType == AT_CORE )
		{
			if( g_LocalUser.m_bIsLoggedIn )
			{
				sprintf_s( sPostData, 512, "u=%s&t=%s&g=%d&h=%d", g_LocalUser.m_sUsername, g_LocalUser.m_sToken, nGameID, g_hardcoreModeActive );

				ZeroMemory( bufferOut, 32768 );
				if( DoBlockingHttpPost( "requestunlocks.php", sPostData, bufferOut, 4096, &nCharsRead ) )
				{
					pBufferOut = &bufferOut[0];
					if( strncmp( pBufferOut, "OK:", 3 ) == 0 )
					{
						pBufferOut += 3;
						while( *pBufferOut != '\0' )
						{
							nID = strtol( pBufferOut, &pBufferOut, 10 );
							if( nID == 0 )
								break;

							//Unlock achievement nID
							if( Unlock( nID ) )
								nNumUnlocked++;

							if( (*pBufferOut) == ':' )
								pBufferOut++;
						}
					}
					else
					{
						assert(!"Could not parse request unlocks return value?");
						MessageBox( NULL, "Could not log you in. Please login again.", "Error logging in!", MB_OK );
						g_LocalUser.Clear();
						//Build_Main_Menu();
					}
				}

				sprintf_s( buffer, 4096, " Loaded %d achievements, total score: %d ", nNumAchievementsLoaded, nTotalPoints );
				sprintf_s( sMessage, 512, " You have %d of %d achievements unlocked. ", nNumUnlocked, m_nNumAchievements );
				g_PopupWindows.AchievementPopups().AddMessage( buffer, sMessage, MSG_INFO );	//	TBD
			}
		}

		return TRUE;
	}
	else
	{
		//	Can't open file: attempt to find it on SQL server!

		ZeroMemory( bufferOut, 32768 );
		nCharsRead = 0;

		nFlags = 1<<0;						//	Active achievements! : 1
		if( m_nType == AT_CORE )
			nFlags |= 1<<1;					//	Core: 3
		else if( m_nType == AT_UNOFFICIAL )
			nFlags |= 1<<2;					//	Unofficial: 5
		else
			nFlags |= 1<<3;					//	Local and custom... (9)

		sprintf_s( sPostData, 256, "u=%s&g=%d&f=%d&l=%d", g_LocalUser.m_sUsername, nGameID, nFlags, (nFlags==3)?1:0 );	//	NB only load LB on Core Set

		if( DoBlockingHttpPost( "requestpatch.php", sPostData, bufferOut, 32768, &nCharsRead ) )
		{
			if( strncmp( bufferOut, "0.", 2 ) == 0 )
			{				
				nMinVer = strtol( bufferOut+2, NULL, 10 );
				if( nCurrentVer < nMinVer )
				{
					//	Client version too old!

					sprintf_s( buffer, 4096, "Client version of 0.%03d is too old for the latest patch format. "
						"Version 0.%03d or greater required.\nVisit " RA_HOST " for a more recent version? ",
							nCurrentVer, nMinVer );

					if( MessageBox( NULL, buffer, "Client out of date!", MB_YESNO ) == IDYES )
					{
						sprintf_s( buffer, 4096, "http://" RA_HOST "/download.php" );

						ShellExecute( NULL,
							"open",
							buffer,
							NULL,
							NULL,
							SW_SHOWNORMAL );
					}
					else
					{
						//MessageBox( NULL, "Cannot load achievements for this game.", "Error", MB_OK );
					}

					return FALSE;
				}
			}

			//	Reopen said file as writable:
			fopen_s( &pFile, sFilename, "w" );
			if( pFile != NULL )
			{
				if( m_nType == AT_CORE )
				{
					//	Only display for core achievements (otherwise receive this message three times)
					//g_PopupWindows.AchievementPopups().AddMessage( " Fetched fresh achievements file from server... ", "", MSG_INFO );	//	TBD
				}

				//	Write the lot out!
				fwrite( bufferOut, sizeof(char), nCharsRead, pFile );
				fclose( pFile );
			}
		}
		else
		{
			//	Could not connect...
			sprintf_s( buffer, 4096, " Could not connect to " RA_HOST "... " );
			g_PopupWindows.AchievementPopups().AddMessage( buffer, " Working offline... ", MSG_INFO ); //?

			return FALSE;
		}

		//	One more attempt at loading from our new file
		return Load( nGameID );
	}

	return FALSE;
}

void AchievementSet::SaveProgress( const char* sSaveStateFilename )
{
	if( !g_LocalUser.m_bIsLoggedIn )
		return;

	if( sSaveStateFilename == NULL )
		return;

	char buffer[4096];
	sprintf_s( buffer, 4096, "%s%s.rap", RA_DIR_DATA, sSaveStateFilename );

	FILE* pf = NULL;
	fopen_s( &pf, buffer, "w" );
	if( pf == NULL )
	{
		assert(0);
		return;
	}

	for( unsigned int i = 0; i < m_nNumAchievements; ++i )
	{
		Achievement* pAch = &m_Achievements[i];
		if( !pAch->Active() )
			continue;

		//	Write ID of achievement and num conditions:
		char cheevoProgressString[4096];
		memset( cheevoProgressString, '\0', 4096 );

		for( unsigned int nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp )
		{
			sprintf_s( buffer, "%d:%d:", pAch->ID(), pAch->NumConditions( nGrp ) );
			strcat_s( cheevoProgressString, 4096, buffer );

			for( unsigned int j = 0; j < pAch->NumConditions( nGrp ); ++j )
			{
				Condition& cond = pAch->GetCondition( nGrp, j );
				sprintf_s( buffer, 4096, "%d:%d:%d:%d:%d:", 
					cond.CurrentHits(),
					cond.CompSource().m_nVal,
					cond.CompSource().m_nLastVal,
					cond.CompTarget().m_nVal,
					cond.CompTarget().m_nLastVal );
				strcat_s( cheevoProgressString, 4096, buffer );
			}
		}
		
		//	Generate a slightly different key to md5ificateorise:
		char cheevoProgressMangled[4096];
		sprintf_s( cheevoProgressMangled, 4096, "%s%s%s%d", 
			g_LocalUser.m_sUsername, cheevoProgressString, g_LocalUser.m_sUsername, pAch->ID() );

		char md5Progress[33];
		md5_GenerateMD5( cheevoProgressMangled, strlen( cheevoProgressMangled ), md5Progress );

		char achMemString[4096];
		int nMemStringLen = pAch->CreateMemString( achMemString, 4096 );
		char md5Achievement[33];
		md5_GenerateMD5( achMemString, nMemStringLen, md5Achievement );
		
		fwrite( cheevoProgressString, sizeof(char), strlen(cheevoProgressString), pf );
		fwrite( md5Progress, sizeof(char), 32, pf );
		fwrite( ":", sizeof(char), 1, pf );
		fwrite( md5Achievement, sizeof(char), 32, pf );
		fwrite( ":", sizeof(char), 1, pf );	//	Check!
	}

	fclose( pf );
}

void AchievementSet::LoadProgress( const char* sLoadStateFilename )
{
	FILE* pf = NULL;
	char buffer[4096];
	long nFileSize = 0;
	unsigned int CondNumHits[50];	//	50 conditions per achievement
	unsigned int CondSourceVal[50];
	unsigned int CondSourceLastVal[50];
	unsigned int CondTargetVal[50];
	unsigned int CondTargetLastVal[50];
	unsigned int nID = 0;
	unsigned int nNumCond = 0;
	char cheevoProgressString[4096];
	unsigned int i = 0;
	unsigned int j = 0;
	char* pGivenProgressMD5 = NULL;
	char* pGivenCheevoMD5 = NULL;
	char pRecalculatedProgressMD5[33];
	char cheevoMD5TestMangled[4096];
	char bufferMemString[4096];
	int nMemStringLen = 0;
	char pRecalculatedAchievementMD5[33];

	if( !g_LocalUser.m_bIsLoggedIn )
		return;

	if( sLoadStateFilename == NULL )
		return;

	sprintf_s( buffer, 4096, "%s%s.rap", RA_DIR_DATA, sLoadStateFilename );

	char* pRawFile = _MallocAndBulkReadFileToBuffer( buffer, nFileSize );

	if( pRawFile != NULL )
	{
		unsigned int nOffs = 0;
		while( nOffs < (unsigned int)(nFileSize-2) && pRawFile[nOffs] != '\0' )
		{
			char* pIter = &pRawFile[nOffs];

			//	Parse achievement id and num conditions
			nID		 = strtol( pIter, &pIter, 10 ); pIter++;
			nNumCond = strtol( pIter, &pIter, 10 );	pIter++;

			//	Concurrently build the md5 checkstring
			sprintf_s( cheevoProgressString, 4096, "%d:%d:", nID, nNumCond );

			ZeroMemory( CondNumHits, 50*sizeof(unsigned int) );
			ZeroMemory( CondSourceVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondSourceLastVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondTargetVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondTargetLastVal, 50*sizeof(unsigned int) );

			for( i = 0; i < nNumCond && i < 50; ++i )
			{
				//	Parse next condition state
				CondNumHits[i]		 = strtol( pIter, &pIter, 10 ); pIter++;
				CondSourceVal[i]	 = strtol( pIter, &pIter, 10 ); pIter++;
				CondSourceLastVal[i] = strtol( pIter, &pIter, 10 ); pIter++;
				CondTargetVal[i]	 = strtol( pIter, &pIter, 10 ); pIter++;
				CondTargetLastVal[i] = strtol( pIter, &pIter, 10 ); pIter++;
			
				//	Concurrently build the md5 checkstring
				sprintf_s( buffer, 4096, "%d:%d:%d:%d:%d:", 
					CondNumHits[i], 
					CondSourceVal[i],
					CondSourceLastVal[i],
					CondTargetVal[i],
					CondTargetLastVal[i] );

				strcat_s( cheevoProgressString, 4096, buffer );
			}

			//	Read the given md5:
			pGivenProgressMD5 = strtok_s( pIter, ":", &pIter );
			pGivenCheevoMD5 = strtok_s( pIter, ":", &pIter );
		
			//	Regenerate the md5 and see if it sticks:
			sprintf_s( cheevoMD5TestMangled, 4096, "%s%s%s%d", 
				g_LocalUser.m_sUsername, cheevoProgressString, g_LocalUser.m_sUsername, nID );
			md5_GenerateMD5( cheevoMD5TestMangled, strlen(cheevoMD5TestMangled), pRecalculatedProgressMD5 );

			if( strncmp( pGivenProgressMD5, pRecalculatedProgressMD5, 32 ) == 0 )
			{
				//	Embed in achievement:
				Achievement* pAch = Find( nID );
				if( pAch != NULL )
				{
					nMemStringLen = pAch->CreateMemString( bufferMemString, 4096 );

					//	Recalculate the current achievement to see if it's compatible:
					md5_GenerateMD5( bufferMemString, nMemStringLen, pRecalculatedAchievementMD5 );

					if( strncmp( pGivenCheevoMD5, pRecalculatedAchievementMD5, 32 ) == 0 )
					{
						for( size_t nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp )
						{
							for( j = 0; j < pAch->NumConditions( nGrp ); ++j )
							{
								Condition& cond = pAch->GetCondition( nGrp, j );

								cond.OverrideCurrentHits( CondNumHits[j] );
								cond.CompSource().m_nVal		= CondSourceVal[j];
								cond.CompSource().m_nLastVal	= CondSourceLastVal[j];
								cond.CompTarget().m_nVal		= CondTargetVal[j];
								cond.CompTarget().m_nLastVal	= CondTargetLastVal[j];

								pAch->SetDirtyFlag( Dirty_Conditions );
							}
						}
					}
					else
					{
						assert(!"Achievement progress savestate incompatible (achievement has changed?)");
					}
				}
				else
				{
					assert(!"Achievement doesn't exist!");
				}
			}
			else
			{
				//assert(!"MD5 invalid... what to do... maybe they're trying to hack achievements?");
			}
		
			nOffs = (pIter - pRawFile);
		}
	
		free( pRawFile );
		pRawFile = NULL;
	}
}

Achievement& AchievementSet::Clone( unsigned int nIter )
{
	Achievement& newAch = AddAchievement();		//	Create a brand new achievement
	newAch.Set( m_Achievements[nIter] );		//	Copy in all values from the clone src
	newAch.SetID( 0 );
	newAch.SetModified( TRUE );
	newAch.SetActive( FALSE );

	return newAch;
}

BOOL AchievementSet::Unlock( unsigned int nAchievementID )
{
	unsigned int i = 0;

	for( i = 0; i < m_nNumAchievements; ++i )
	{
		if( m_Achievements[i].ID() == nAchievementID )
		{
			m_Achievements[i].SetActive( FALSE );
			//	Update Dlg? //TBD
			return TRUE;
		}
	}

	RA_LOG( "Attempted to unlock achievement %d but failed!\n", nAchievementID );
	//assert( !"Cannot unlock this achievement..." );
	return FALSE;//??
}

void AchievementSet::SetPaused( BOOL bPauseState )
{
	m_bProcessingActive = !bPauseState;
}

BOOL AchievementSet::HasUnsavedChanges()
{
	unsigned int i = 0;
	for( i = 0; i < m_nNumAchievements; ++i )
	{
		if( m_Achievements[i].Modified() )
			return TRUE;
	}

	return FALSE;
}
BOOL AchievementSet::IsCurrentAchievementSetSelected()
{
	return( this == g_pActiveAchievements );
}

const char* AchievementSet::GameTitle()				
{ 
	return m_sPreferredGameTitle;
}

void AchievementSet::SetGameTitle( const char* pStrIn )
{ 
	strcpy_s( m_sPreferredGameTitle, 64, pStrIn );
}

