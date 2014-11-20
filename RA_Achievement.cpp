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

AchievementSet* CoreAchievements = NULL;
AchievementSet* UnofficialAchievements = NULL;
AchievementSet* LocalAchievements = NULL;

AchievementSetType g_nActiveAchievementSet = AchievementSetCore;
AchievementSet* g_pActiveAchievements = CoreAchievements;

namespace
{
	const char* LockedBadgeFile = "00000.png";

	unsigned int GetFlagsFromType( AchievementSetType nType )
	{
		unsigned int nFlags = 0;
		nFlags = 1<<0;						//	Active achievements! : 1
		if( nType == AchievementSetCore )
			nFlags |= 1<<1;					//	Core: 3
		else if( nType == AchievementSetUnofficial )
			nFlags |= 1<<2;					//	Unofficial: 5
		else
			nFlags |= 1<<3;					//	Local and custom... (9)

		return nFlags;
	}

}

void SetAchievementCollection( AchievementSetType Type )
{
	g_nActiveAchievementSet = Type;
	switch( Type )
	{
	case AchievementSetCore:
		g_pActiveAchievements = CoreAchievements;
		break;
	case AchievementSetUnofficial:
		g_pActiveAchievements = UnofficialAchievements;
		break;
	case AchievementSetLocal:
		g_pActiveAchievements = LocalAchievements;
		break;
	}
}

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

	m_sBadgeImageURI = sBadgeURI;

	m_hBadgeImage = LoadOrFetchBadge( sBadgeURI, RA_BADGE_PX );
	if( sBadgeURI.find( "_lock" ) == std::string::npos )	//	Ensure we're not going to look for _lock_lock
		m_hBadgeImageLocked = LoadOrFetchBadge( sBadgeURI + "_lock", RA_BADGE_PX );
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
	
	unsigned int i = 0;
	char* sDelta = "";
	char* sSize = "";

	for( size_t nGrp = 0; nGrp < NumConditionGroups(); ++nGrp )
	{
		if( m_vConditions[nGrp].Count() == 0 )	//Ignore empty groups when saving
			continue;
		
		if( nGrp > 0 )	//	Subcondition start found
			sstr << "S";

		for( i = 0; i < m_vConditions[nGrp].Count(); ++i )
		{
			const Condition* pNextCond = &m_vConditions[nGrp].GetAt(i);

			char sNextCondition[256];
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
			sstr << sNextCondition;
		}
	}

	return sstr.str();
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
std::string AchievementSet::GetAchievementSetFilename( GameID nGameID )
{
	return RA_DIR_DATA + std::to_string( nGameID ) + ".txt";

	//switch( nSet )
	//{
	//case AchievementSetCore:
	//	return RA_DIR_DATA + std::to_string( nGameID ) + ".txt";
	//case AchievementSetUnofficial:
	//	return RA_DIR_DATA + std::to_string( nGameID ) + "-Unofficial.txt";
	//default:
	//case AchievementSetLocal:
	//	return RA_DIR_DATA + std::to_string( nGameID ) + "-User.txt";
	//}
}

//	static
BOOL AchievementSet::DeletePatchFile( AchievementSetType nSet, GameID nGameID )
{
	if( nGameID != 0 )
	{
		std::string sFilename = AchievementSet::GetAchievementSetFilename( nGameID );
							
		//	Remove the text file
		SetCurrentDirectory( g_sHomeDir.c_str() );
		if( _access( sFilename.c_str(), 06 ) != -1 )	//	06= Read/write permission
		{
			if( remove( sFilename.c_str() ) == -1 )
			{
				//	Remove issues?
				ASSERT( !"Could not remove patch file!?" );
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
	m_Achievements.push_back( Achievement( m_nSetType ) );
	return m_Achievements.back();
}

BOOL AchievementSet::RemoveAchievement( size_t nIter )
{
	if( nIter < m_Achievements.size() )
	{
		m_Achievements.erase( m_Achievements.begin()+nIter );
		return TRUE;
	}
	else
	{
		assert( 0 );
		return FALSE;
	}
}

Achievement* AchievementSet::Find( AchievementID nAchievementID )
{
	std::vector<Achievement>::iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		if( (*iter).ID() == nAchievementID )
			return &(*iter);
		iter++;
	}

	return NULL;
}

size_t AchievementSet::GetAchievementIndex( const Achievement& Ach )
{
	for( size_t nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset )
	{
		if( &Ach == &GetAchievement( nOffset ) )
			return nOffset;
	}

	//	Not found
	return -1;
}

unsigned int AchievementSet::NumActive() const
{
	unsigned int nNumActive = 0;
	std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		if( (*iter).Active() )
			nNumActive++;
		iter++;
	}
	return nNumActive;
}

void AchievementSet::Clear()
{
	std::vector<Achievement>::iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		iter->Clear();
		iter++;
	}

	m_Achievements.clear();
	m_nGameID = 0;
	m_bProcessingActive = TRUE;
	m_sPreferredGameTitle[0] = '\0';
}

void AchievementSet::Test()
{
	if( !m_bProcessingActive )
		return;
	
	for( std::vector<Achievement>::iterator iter = m_Achievements.begin(); iter != m_Achievements.end(); ++iter )
	{
		Achievement& ach = (*iter);
		if( !ach.Active() )
			continue;

		if( ach.Test() == TRUE )
		{
			//	Award. If can award or have already awarded, set inactive:
			ach.SetActive( FALSE );

			//	Reverse find where I am in the list:
			unsigned int nOffset = 0;
			for( nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset )
 			{
 				if( &ach == &g_pActiveAchievements->GetAchievement( nOffset ) )
 					break;
 			}

 			ASSERT( nOffset < NumAchievements() );
 			if( nOffset < NumAchievements() )
				g_AchievementsDialog.ReloadLBXData( nOffset );

			if( RAUsers::LocalUser.IsLoggedIn() )
			{
				const std::string sPoints = std::to_string( ach.Points() );

				if( ach.ID() == 0 )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( " Test: Achievement Unlocked ",
									  " " + ach.Title() + " (" + sPoints + ") (Unofficial) ",
									  PopupAchievementUnlocked,
									  ach.BadgeImage() ) );
				}
				else if( ach.Modified() )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( " Modified: Achievement Unlocked ",
									  " " + ach.Title() + " (" + sPoints + ") (Unofficial) ",
									  PopupAchievementUnlocked,
									  ach.BadgeImage() ) );
				}
				else if( g_fnDoValidation == NULL )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( " (Missing RA_Keys.DLL): Achievement Unlocked ",
									  " " + ach.Title() + " (" + sPoints + ") (Unofficial) ",
									  PopupAchievementError,
									  ach.BadgeImage() ) );
				}
				else if( g_bRAMTamperedWith )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( " (RAM tampered with!): Achievement Unlocked ",
									  " " + ach.Title() + " (" + sPoints + ") (Unofficial) ",
									  PopupAchievementError,
									  ach.BadgeImage() ) );
				}
				else
				{
					char sValidation[50];
					g_fnDoValidation( sValidation, RAUsers::LocalUser.Username().c_str(), RAUsers::LocalUser.Token().c_str(), ach.ID() );

					PostArgs args;
					args['u'] = RAUsers::LocalUser.Username();
					args['t'] = RAUsers::LocalUser.Token();
					args['a'] = std::to_string( ach.ID() );
					args['v'] = sValidation;
					args['h'] = std::to_string( static_cast<int>( g_hardcoreModeActive ) );
					
					RAWeb::CreateThreadedHTTPRequest( RequestSubmitAwardAchievement, args );
				}
			}
		}
	}
}

BOOL AchievementSet::SaveToFile()
{
	//	Why not submit each ach straight to cloud?
	return FALSE;

	//FILE* pf = NULL;
	//const std::string sFilename = AchievementSet::GetAchievementSetFilename( m_nGameID );
	//if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
	//{
	//	FileStream fs( pf );
	//	
	//	CoreAchievements->Serialize( fs );
	//	UnofficialAchievements->Serialize( fs );
	//	LocalAchievements->Serialize( fs );

	//	fclose( pf );
	//}
}

BOOL AchievementSet::Serialize( FileStream& Stream )
{
	//	Why not submit each ach straight to cloud?
	return FALSE;

	//FILE* pf = NULL;
	//const std::string sFilename = AchievementSet::GetAchievementSetFilename( m_nGameID );
	//if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
	//{
	//	FileStream fs( pf );
	//	Writer<FileStream> writer( fs );
	//	
	//	Document doc;
	//	doc.AddMember( "MinVer", "0.050", doc.GetAllocator() );
	//	doc.AddMember( "GameTitle", m_sPreferredGameTitle, doc.GetAllocator() );
	//	
	//	Value achElements;
	//	achElements.SetArray();

	//	std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
	//	while( iter != m_Achievements.end() )
	//	{
	//		Value nextElement;

	//		const Achievement& ach = (*iter);
	//		nextElement.AddMember( "ID", ach.ID(), doc.GetAllocator() );
	//		nextElement.AddMember( "Mem", ach.CreateMemString(), doc.GetAllocator() );
	//		nextElement.AddMember( "Title", ach.Title(), doc.GetAllocator() );
	//		nextElement.AddMember( "Description", ach.Description(), doc.GetAllocator() );
	//		nextElement.AddMember( "Author", ach.Author(), doc.GetAllocator() );
	//		nextElement.AddMember( "Points", ach.Points(), doc.GetAllocator() );
	//		nextElement.AddMember( "Created", ach.CreatedDate(), doc.GetAllocator() );
	//		nextElement.AddMember( "Modified", ach.ModifiedDate(), doc.GetAllocator() );
	//		nextElement.AddMember( "Badge", ach.BadgeImageFilename(), doc.GetAllocator() );

	//		achElements.PushBack( nextElement, doc.GetAllocator() );
	//		iter++;
	//	}

	//	doc.AddMember( "Achievements", achElements, doc.GetAllocator() );

	//	//	Build a document to persist, then pass to doc.Accept();
	//	doc.Accept( writer );

	//	fclose( pf );
	//	return TRUE;
	//}
	//else
	//{
	//	//	Could not write to file?
	//	return FALSE;
	//}
}

BOOL AchievementSet::FetchFromWebBlocking( GameID nGameID )
{
	//	Can't open file: attempt to find it on SQL server!
	PostArgs args;
	args['u'] = RAUsers::LocalUser.Username();
	args['t'] = RAUsers::LocalUser.Token();
	args['g'] = std::to_string( nGameID );
	args['h'] = g_hardcoreModeActive ? "1" : "0";
	//args['f'] = std::to_string( GetFlagsFromType( m_nSetType ) );

	const long CURRENT_VER = strtol( std::string( g_sClientVersion ).substr( 2 ).c_str(), NULL, 10 );

	Document doc;
	if( RAWeb::DoBlockingRequest( RequestPatch, args, doc ) && doc.HasMember("Success") && doc["Success"].GetBool() && doc.HasMember("PatchData") )
	{
		const Value& PatchData = doc["PatchData"];

		//const std::string& sMinVer = PatchData["MinVer"].GetString();
		//const long nMinVerRequired = strtol( sMinVer.substr( 2 ).c_str(), NULL, 10 );

		//if( CURRENT_VER < nMinVerRequired )
		//{
		//	//	Client version too old!

		//	char buffer[4096];
		//	sprintf_s( buffer, 4096, 
		//		"Client version of 0.%03d is too old for the latest patch format.\r\n"
		//		"Version 0.%03d or greater required.\r\n"
		//		"Visit " RA_HOST " for a more recent version? ",
		//		CURRENT_VER,
		//		nMinVerRequired );

		//	if( MessageBox( NULL, buffer, "Client out of date!", MB_YESNO ) == IDYES )
		//	{
		//		sprintf_s( buffer, 4096, "http://" RA_HOST "/download.php" );

		//		ShellExecute( NULL,
		//			"open",
		//			buffer,
		//			NULL,
		//			NULL,
		//			SW_SHOWNORMAL );
		//	}
		//	else
		//	{
		//		//MessageBox( NULL, "Cannot load achievements for this game.", "Error", MB_OK );
		//	}

		//	return FALSE;
		//}
		//else
		{
			SetCurrentDirectory( g_sHomeDir.c_str() );
			FILE* pf = NULL;
			fopen_s( &pf, std::string( RA_DIR_DATA + std::to_string( nGameID ) + ".txt" ).c_str(), "wb" );
			if( pf != NULL )
			{
				FileStream fs( pf );
				Writer<FileStream> writer( fs );
				PatchData.Accept( writer );
				fclose( pf );
				return TRUE;
			}
			else
			{
				ASSERT( !"Could not open patch file for writing?" );
				RA_LOG( "Could not open patch file for writing?" );
				return FALSE;
			}
		}
	}
	else
	{
		//	Could not connect...
		PopupWindows::AchievementPopups().AddMessage( 
			MessagePopup( " Could not connect to " RA_HOST "... ", " Working offline... ", PopupInfo ) ); //?

		return FALSE;
	}
}

//	static
BOOL AchievementSet::LoadFromFile( GameID nGameID )
{
	//	Is this safe?
	CoreAchievements->Clear();
	UnofficialAchievements->Clear();
	LocalAchievements->Clear();			//?!?!?

	if( nGameID == 0 )
		return TRUE;

	const std::string sFilename = GetAchievementSetFilename( nGameID );

	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pFile = NULL;
	fopen_s( &pFile, sFilename.c_str(), "rb" );
	if( pFile != NULL )
	{
		//	Store this: we are now assuming this is the correct checksum if we have a file for it
		CoreAchievements->SetGameID( nGameID );
		UnofficialAchievements->SetGameID( nGameID );
		LocalAchievements->SetGameID( nGameID );

		Document doc;
		doc.ParseStream( FileStream( pFile ) );
		if( !doc.HasParseError() )
		{
			
			//ASSERT( doc["Success"].GetBool() );
			//const Value& PatchData = doc["PatchData"];
			const GameID nGameIDFetched = doc["ID"].GetUint();
			ASSERT( nGameIDFetched == nGameID );
			const std::string& sGameTitle = doc["Title"].GetString();
			const unsigned int nConsoleID = doc["ConsoleID"].GetUint();
			const std::string& sConsoleName = doc["ConsoleName"].GetString();
			const unsigned int nForumTopicID = doc["ForumTopicID"].GetUint();
			const unsigned int nGameFlags = doc["Flags"].GetUint();
			const std::string& sImageIcon = doc["ImageIcon"].GetString();
			const std::string& sImageTitle = doc["ImageTitle"].GetString();
			const std::string& sImageIngame = doc["ImageIngame"].GetString();
			const std::string& sImageBoxArt = doc["ImageBoxArt"].GetString();
			const std::string& sPublisher = doc["Publisher"].IsNull() ? "Unknown" : doc["Publisher"].GetString();
			const std::string& sDeveloper = doc["Developer"].IsNull() ? "Unknown" : doc["Developer"].GetString();
			const std::string& sGenre = doc["Genre"].IsNull() ? "Unknown" : doc["Genre"].GetString();
			const std::string& sReleased = doc["Released"].IsNull() ? "Unknown" : doc["Released"].GetString();
			const bool bIsFinal = doc["IsFinal"].GetBool();
			const std::string& sRichPresencePatch = doc["RichPresencePatch"].IsNull() ? "" : doc["RichPresencePatch"].GetString();
			
			RA_RichPresenceInterpretter::PersistAndParseScript( nGameIDFetched, sRichPresencePatch );

			const Value& AchievementsData = doc["Achievements"];

			for( SizeType i = 0; i < AchievementsData.Size(); ++i )
			{
				//	Parse into correct boxes
				unsigned int nFlags = AchievementsData[i]["Flags"].GetUint();
				if( nFlags == 3 )
				{
					Achievement& newAch = CoreAchievements->AddAchievement();
					newAch.Parse( AchievementsData[i] );
				}
				else if( nFlags == 5 )
				{
					Achievement& newAch = UnofficialAchievements->AddAchievement();
					newAch.Parse( AchievementsData[i] );
				}
				else
				{
					RA_LOG( "Cannot deal with achievement with flags: %d\n", nFlags );
				}
			}
			
			const Value& LeaderboardsData = doc["Leaderboards"];
			
			for( SizeType i = 0; i < LeaderboardsData.Size(); ++i )
			{
				//"Leaderboards":[{"ID":"2","Mem":"STA:0xfe10=h0000_0xhf601=h0c_d0xhf601!=h0c_0xfff0=0_0xfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 1","Description":"Complete this act in the fastest time!"},
				
				RA_Leaderboard lb( LeaderboardsData[i]["ID"].GetUint() );
				lb.LoadFromJSON( LeaderboardsData[i] );

				g_LeaderboardManager.AddLeaderboard( lb );
			}
		}
		else
		{
			fclose( pFile );
			ASSERT( !"Could not parse file?!" );
			return FALSE;
		}

		fclose( pFile );
		
		unsigned int nTotalPoints = 0;
		for( size_t i = 0; i < CoreAchievements->NumAchievements(); ++i )
			nTotalPoints += CoreAchievements->GetAchievement( i ).Points();

		if( RAUsers::LocalUser.IsLoggedIn() )
		{	
			//	Loaded OK: post a request for unlocks
			PostArgs args;
			args['u'] = RAUsers::LocalUser.Username();
			args['t'] = RAUsers::LocalUser.Token();
			args['g'] = std::to_string( nGameID );
			args['h'] = g_hardcoreModeActive ? "1" : "0";

			RAWeb::CreateThreadedHTTPRequest( RequestUnlocks, args );
			
			std::string sNumCoreAch = std::to_string( CoreAchievements->NumAchievements() );

			//sprintf_s( sMessage, 512, " You have %d of %d achievements unlocked. ", nNumUnlocked, m_nNumAchievements );
			g_PopupWindows.AchievementPopups().AddMessage( 
				MessagePopup( "Loaded " + sNumCoreAch + " achievements, Total Score " + std::to_string( nTotalPoints ), "", PopupInfo ) );	//	TBD
		}

		return TRUE;
	}
	else
	{
		//	Cannot open file
		RA_LOG( "Cannot open file %s\n", sFilename.c_str() );
		return FALSE;
	}
}

void AchievementSet::SaveProgress( const char* sSaveStateFilename )
{
	if( !RAUsers::LocalUser.IsLoggedIn() )
		return;

	if( sSaveStateFilename == NULL )
		return;

	char buffer[4096];
	sprintf_s( buffer, 4096, "%s%s.rap", RA_DIR_DATA, sSaveStateFilename );
	
	SetCurrentDirectory( g_sHomeDir.c_str() );
	FILE* pf = NULL;
	fopen_s( &pf, buffer, "w" );
	if( pf == NULL )
	{
		assert(0);
		return;
	}

	for( size_t i = 0; i < NumAchievements(); ++i )
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
		
		//	Generate a slightly different key to md5ify:
		char sCheevoProgressMangled[4096];
		sprintf_s( sCheevoProgressMangled, 4096, "%s%s%s%d", 
			RAUsers::LocalUser.Username().c_str(), cheevoProgressString, RAUsers::LocalUser.Username().c_str(), pAch->ID() );
		
		std::string sMD5Progress = RA::GenerateMD5( std::string( sCheevoProgressMangled ) );
		std::string sMD5Achievement = RA::GenerateMD5( pAch->CreateMemString() );
		
		fwrite( cheevoProgressString, sizeof(char), strlen(cheevoProgressString), pf );
		fwrite( sMD5Progress.c_str(), sizeof(char), sMD5Progress.length(), pf );
		fwrite( ":", sizeof(char), 1, pf );
		fwrite( sMD5Achievement.c_str(), sizeof(char), sMD5Achievement.length(), pf );
		fwrite( ":", sizeof(char), 1, pf );	//	Check!
	}

	fclose( pf );
}

void AchievementSet::LoadProgress( const char* sLoadStateFilename )
{
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
	char cheevoMD5TestMangled[4096];
	int nMemStringLen = 0;

	if( !RAUsers::LocalUser.IsLoggedIn() )
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
				RAUsers::LocalUser.Username().c_str(), cheevoProgressString, RAUsers::LocalUser.Username().c_str(), nID );

			std::string sRecalculatedProgressMD5 = RA::GenerateMD5( cheevoMD5TestMangled );

			if( sRecalculatedProgressMD5.compare( pGivenProgressMD5 ) == 0 )
			{
				//	Embed in achievement:
				Achievement* pAch = Find( nID );
				if( pAch != NULL )
				{
					std::string sMemStr = pAch->CreateMemString();

					//	Recalculate the current achievement to see if it's compatible:
					std::string sMemMD5 = RA::GenerateMD5( sMemStr );
					if( sMemMD5.compare( 0, 32, pGivenCheevoMD5 ) == 0 )
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
						ASSERT( !"Achievement progress savestate incompatible (achievement has changed?)" );
						RA_LOG( "Achievement progress savestate incompatible (achievement has changed?)" );
					}
				}
				else
				{
					ASSERT( !"Achievement doesn't exist!" );
					RA_LOG( "Achievement doesn't exist!" );
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

BOOL AchievementSet::Unlock( AchievementID nAchID )
{
	for( size_t i = 0; i < NumAchievements(); ++i )
	{
		if( m_Achievements[ i ].ID() == nAchID )
		{
			m_Achievements[ i ].SetActive( FALSE );
			return TRUE;	//	Update Dlg? //TBD
		}
	}

	RA_LOG( "Attempted to unlock achievement %d but failed!\n", nAchID );
	return FALSE;//??
}

BOOL AchievementSet::IsCurrentAchievementSetSelected() const
{
	return( this == g_pActiveAchievements );
}

BOOL AchievementSet::HasUnsavedChanges()
{
	for( size_t i = 0; i < NumAchievements(); ++i )
	{
		if( m_Achievements[i].Modified() )
			return TRUE;
	}

	return FALSE;
}

