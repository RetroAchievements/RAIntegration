#include "RA_Leaderboard.h"
#include <vector>
#include <assert.h>
#include <time.h>
#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"

RA_LeaderboardManager g_LeaderboardManager;

//////////////////////////////////////////////////////////////////////////
double MemValue::GetValue() const
{
	int nRetVal = 0;
	if( m_bParseVal )
	{
		nRetVal = m_nAddress;	//	insert address as value.
	}
	else
	{
		if( m_nVarSize == CMP_SZ_4BIT_LOWER )
		{
			nRetVal = g_MemManager.RAMByte( m_nAddress )&0xf;
		}
		else if( m_nVarSize == CMP_SZ_4BIT_UPPER )
		{
			nRetVal = (g_MemManager.RAMByte( m_nAddress )>>4)&0xf;
		}
		else if( m_nVarSize == CMP_SZ_8BIT )
		{
			nRetVal = g_MemManager.RAMByte( m_nAddress );
		}
		else if( m_nVarSize == CMP_SZ_16BIT )
		{
			nRetVal = g_MemManager.RAMByte( m_nAddress );
			nRetVal |= (g_MemManager.RAMByte( m_nAddress+1 ) << 8);
		}
		else if( m_nVarSize >= CMP_SZ_1BIT_0 && m_nVarSize <= CMP_SZ_1BIT_7 )
		{
			const unsigned int nBitmask = ( 1 << (m_nVarSize-CMP_SZ_1BIT_0) );
			nRetVal = ( g_MemManager.RAMByte( m_nAddress ) & nBitmask ) != 0;
		}
		//nRetVal = g_MemManager.RAMByte( m_nAddress );

		if( m_bBCDParse )
		{
			//	Reparse this value as a binary coded decimal.
			nRetVal = ( ( (nRetVal>>4)&0xf) * 10) + (nRetVal&0xf);
		}
	}

	return nRetVal * m_fModifier;
}

char* MemValue::ParseFromString( char* pBuffer )
{
	char* pIter = &pBuffer[0];

	//	Borrow parsing from CompVariable

	m_bBCDParse = false;
	if( toupper( *pIter ) == 'B' )
	{
		m_bBCDParse = true;
		pIter++;
	}
	else if( toupper( *pIter ) == 'V' )
	{
		m_bParseVal = true;
		pIter++;
	}

	CompVariable varTemp;
	varTemp.ParseVariable( pIter );
	m_nAddress = varTemp.m_nVal;	//	Fetch value ('address') as parsed
	m_nVarSize = varTemp.m_nVarSize;

	m_fModifier = 1.0;
	if( *pIter == '*' )
	{
		pIter++;						//	Skip over modifier type.. assume mult( '*' );
		m_fModifier = strtod( pIter, &pIter );
	}

	return pIter;
}

//////////////////////////////////////////////////////////////////////////
double ValueSet::GetValue() const
{
	double fVal = 0.0;
	std::vector<MemValue>::const_iterator iter = m_Values.begin();
	while( iter != m_Values.end() )
	{
		fVal += (*iter).GetValue();
		iter++;
	}

	return fVal;
}

void ValueSet::AddNewValue( MemValue nMemVal )
{
	m_Values.push_back( nMemVal );
}

void ValueSet::ParseMemString( char* pChar )
{
	do 
	{
		{
			while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
				pChar++; // Skip any chars up til this point :S
		}

		MemValue newMemVal;
		pChar = newMemVal.ParseFromString( pChar );
		AddNewValue( newMemVal );
	}
	while( *pChar == '_' );
}

void ValueSet::Clear()
{
	m_Values.clear();
}

//////////////////////////////////////////////////////////////////////////
RA_Leaderboard::RA_Leaderboard()
{
	m_sTitle[0] = '\0';
	m_sDescription[0] = '\0';
	m_bStarted = false;
}

RA_Leaderboard::~RA_Leaderboard()
{
}

void RA_Leaderboard::ParseLine( char* sBuffer )
{
	char* pChar = &sBuffer[0];

	pChar++;								//	Skip over 'L' character
	m_nID = strtol( pChar, &pChar, 10 );	//	Get ID

	while( *pChar != '\n' && *pChar != '\0' )
	{
		if( pChar[0] == ':' && pChar[1] == ':' )	//	New Phrase (double colon)
			pChar += 2;

		if( strncmp( pChar, "STA:", sizeof("STA:")-1 ) == 0 )
		{
			pChar += 4;

			//	Parse Start condition
			do 
			{
				{
					while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
						pChar++; // Skip any chars up til this point :S
				}
				
				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_startCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( strncmp( pChar, "CAN:", sizeof("CAN:")-1 ) == 0 )
		{
			pChar += 4;
			//	Parse Cancel condition
			do 
			{
				{
					while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
						pChar++; // Skip any chars up til this point :S
				}
				
				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_cancelCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( strncmp( pChar, "SUB:", sizeof("SUB:")-1 ) == 0 )
		{
			pChar += 4;
			//	Parse Submit condition
			do 
			{
				{
					while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
						pChar++; // Skip any chars up til this point :S
				}

				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_submitCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( strncmp( pChar, "VAL:", sizeof("VAL:")-1 ) == 0 )
		{
			pChar += 4;
			//	Parse Value condition
			//	Value is a collection of memory addresses and modifiers.
			do 
			{
				{
					while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
						pChar++; // Skip any chars up til this point :S
				}

				MemValue newMemVal;
				pChar = newMemVal.ParseFromString( pChar );
				m_value.AddNewValue( newMemVal );
			}
			while( *pChar == '_' );
		}
		else if( strncmp( pChar, "PRO:", sizeof("PRO:")-1 ) == 0 )
		{
			pChar += 4;
			//	Progress: normally same as value:
			//	Progress is a collection of memory addresses and modifiers.
			do 
			{
				{
					while( (*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' )
						pChar++; // Skip any chars up til this point :S
				}

				MemValue newMemVal;
				pChar = newMemVal.ParseFromString( pChar );
				m_progress.AddNewValue( newMemVal );
			}
			while( *pChar == '_' );
		}
		else if( strncmp( pChar, "FOR:", sizeof("FOR:")-1 ) == 0 )
		{
			pChar += 4;
			//	Format

			if( strncmp( pChar, "TIMESECS", sizeof("TIMESECS")-1 ) == 0 )
			{
				m_format = Format_TimeSecs;
				pChar += sizeof("TIMESECS")-1;
			}
			else if( strncmp( pChar, "TIME", sizeof("TIME")-1 ) == 0 ) 
			{
				m_format = Format_TimeFrames;
				pChar += sizeof("TIME")-1;
			}
			else if( strncmp( pChar, "MILLISECS", sizeof("MILLISECS")-1 ) == 0 ) 
			{
				m_format = Format_TimeMillisecs;
				pChar += sizeof("MILLISECS")-1;
			}
			else if( strncmp( pChar, "POINTS", sizeof("POINTS")-1 ) == 0 )
			{
				m_format = Format_Score;
				pChar += sizeof("POINTS")-1;
			}
			else if( strncmp( pChar, "SCORE", sizeof("SCORE")-1 ) == 0 )	//dupe
			{
				m_format = Format_Score;
				pChar += sizeof("SCORE")-1;
			}
			else if( strncmp( pChar, "VALUE", sizeof("VALUE")-1 ) == 0 )
			{
				m_format = Format_Value;
				pChar += sizeof("VALUE")-1;
			}
			else
			{
				m_format = Format_Other;
				while( isalnum( *pChar ) )
					pChar++;
			}
		}
		else if( strncmp( pChar, "TTL:", sizeof("TTL:")-1 ) == 0 )
		{
			pChar += 4;
			//	Title:
			char* pDest = &m_sTitle[0];
			while( !( pChar[0] == ':' && pChar[1] == ':' ) )
			{
				*pDest = *pChar;	//	char copy
				pDest++;
				pChar++;
			}
			*pDest = '\0';
		}
		else if( strncmp( pChar, "DES:", sizeof("DES:")-1 ) == 0 )
		{
			pChar += 4;
			//	Description:
			char* pDest = &m_sDescription[0];
			while( pChar[0] != '\n' && pChar[0] != '\0' )
			{
				*pDest = *pChar;	//	char copy
				pDest++;
				pChar++;
			}
			*pDest = '\0';
		}
		else
		{
			//	badly formatted... cannot progress!
			assert(!"Badly formatted: this leaderboard makes no sense!");
			break;
		}
	}
}

double RA_Leaderboard::GetCurrentValue()
{
	return m_value.GetValue();
}

double RA_Leaderboard::GetCurrentValueProgress() const
{
	if( m_progress.NumValues() > 0 )
		return m_progress.GetValue();
	else
		return m_value.GetValue();
}

void RA_Leaderboard::Clear()
{
	m_value.Clear();
	m_progress.Clear();
	//	Clear all locally stored lb info too tbd
}

void RA_Leaderboard::Reset()
{
	m_bStarted = false;

	m_startCond.Reset( true );
	m_cancelCond.Reset( true );
	m_submitCond.Reset( true );
}

void RA_Leaderboard::Test()
{
	BOOL bUnused;

	//	Ensure these are always tested once every frame, to ensure delta
	//	 variables work properly :)
	BOOL bStartOK  = m_startCond.Test(bUnused, bUnused, FALSE);
	BOOL bCancelOK = m_cancelCond.Test(bUnused, bUnused, TRUE);
	BOOL bSubmitOK = m_submitCond.Test(bUnused, bUnused, FALSE);

	if( !m_bStarted )
	{
		if( bStartOK )
		{
			m_bStarted = TRUE;

			char sTitle[1024];
			char sSubtitle[1024];
			sprintf_s( sTitle, 1024, " Challenge Available: %s ", m_sTitle );
			sprintf_s( sSubtitle, 1024, " %s ", m_sDescription );
			g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_LEADERBOARD_INFO, NULL );
			g_PopupWindows.LeaderboardPopups().Activate( m_nID );
		}
	}
	else
	{
 		if( bCancelOK )
		{
			m_bStarted = FALSE;

			g_PopupWindows.LeaderboardPopups().Deactivate( m_nID );

			char sTitle[1024];
			char sSubtitle[1024];
			sprintf_s( sTitle, 1024, " Leaderboard attempt cancelled! " );
			sprintf_s( sSubtitle, 1024, " %s ", m_sTitle );
			g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_LEADERBOARD_CANCEL, NULL );
		}
		else if( bSubmitOK )
		{
			g_PopupWindows.LeaderboardPopups().Deactivate( m_nID );

			m_bStarted = FALSE;
			int nVal = (int)m_value.GetValue();

			if( g_bRAMTamperedWith )
			{
				g_PopupWindows.AchievementPopups().AddMessage( 
					"Not posting to leaderboard: memory tamper detected!",
					"Reset game to reenable posting.",
					MSG_INFO );
			}
			else if( g_hRAKeysDLL != NULL && g_fnDoValidation != NULL )
			{
				//	TBD: move to keys!s
				char sValidationSig[50];
				sprintf_s( sValidationSig, 50, "%d%s%d", m_nID, g_LocalUser.m_sUsername, m_nID );

				char sValidation[50];
				md5_GenerateMD5( sValidationSig, strlen(sValidationSig), sValidation );

				char sPost[1024];
				sprintf_s( sPost, 1024, "u=%s&t=%s&i=%d&v=%s&s=%d",
					g_LocalUser.m_sUsername, 
					g_LocalUser.m_sToken, 
					m_nID,
					sValidation,
					nVal );
				
				CreateHTTPRequestThread( "requestsubmitlbentry.php", sPost, HTTPRequest_Post, 0, &RA_LeaderboardManager::s_OnSubmitEntry );
			}
		}
	}
}

void RA_Leaderboard::SubmitRankInfo( unsigned int nRank, char* sUsername, unsigned int nScore, time_t nAchieved )
{
	LB_Entry newEntry;
	newEntry.m_nRank = nRank;
	strcpy_s( newEntry.m_sUsername, 50, sUsername );
	newEntry.m_nScore = nScore;
	newEntry.m_TimeAchieved = nAchieved;

	std::vector<LB_Entry>::iterator iter = m_RankInfo.begin();
	while( iter != m_RankInfo.end() )
	{
		if( (*iter).m_nRank == nRank )
		{
			(*iter) = newEntry;
			return;
		}
		iter++;
	}
	
	//	If not found, add new entry.
	m_RankInfo.push_back( newEntry );
}

void RA_Leaderboard::SortRankInfo()
{
	for( size_t i = 0; i < m_RankInfo.size(); ++i )
	{
		for( size_t j = i; j < m_RankInfo.size(); ++j )
		{
			if( i == j )
				continue;

			if( m_RankInfo.at(i).m_nRank > m_RankInfo.at(j).m_nRank )
			{
				LB_Entry temp = m_RankInfo[i];
				m_RankInfo[i]= m_RankInfo[j];
				m_RankInfo[j] = temp;
			}
		}
	}
}

//static 
void RA_Leaderboard::FormatScore( FormatType nType, unsigned int nScoreIn, char* pBuffer, unsigned int nLen )
{
	if( nType == Format_TimeFrames )
	{
		int nMins = nScoreIn/3600;
		int nSecs = (nScoreIn%3600)/60;
		int nMilli = (int)( ( (nScoreIn%3600)%60 ) * (100.0/60.0) );	//	Convert from frames to 'millisecs'
		sprintf_s( pBuffer, nLen, "%02d:%02d.%02d", nMins, nSecs, nMilli );
	}
	else if( nType == Format_TimeSecs )
	{
		int nMins = nScoreIn/60;
		int nSecs = nScoreIn%60;
		sprintf_s( pBuffer, nLen, "%02d:%02d", nMins, nSecs );
	}
	else if( nType == Format_TimeMillisecs )
	{
		int nMins = nScoreIn/6000;
		int nSecs = (nScoreIn%6000)/100;
		int nMilli = (int)( nScoreIn%100 );
		sprintf_s( pBuffer, nLen, "%02d:%02d.%02d", nMins, nSecs, nMilli );
	}
	else if( nType == Format_Score )
	{
		sprintf_s( pBuffer, nLen, "%06d Points", nScoreIn );
	}
	else if( nType == Format_Value )
	{
		sprintf_s( pBuffer, nLen, "%01d", nScoreIn );
	}
	else
	{
		sprintf_s( pBuffer, nLen, "%06d", nScoreIn );
	}
}

RA_Leaderboard* RA_LeaderboardManager::FindLB( unsigned int nID )
{
	std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
	while( iter != m_Leaderboards.end() )
	{
		if( (*iter).ID() == nID )
			return &(*iter);

		iter++;
	}

	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//static
void RA_LeaderboardManager::s_OnSubmitEntry( void* pPostData )
{
	RequestObject* pDataSent = static_cast<RequestObject*>( pPostData );
	if( pDataSent && pDataSent->m_bResponse )
	{
		if( strncmp( pDataSent->m_sResponse, "OK:", sizeof("OK:")-1 ) == 0 )
		{
			char* pChar = pDataSent->m_sResponse+3;

			unsigned int nLBID	= strtol( pChar, &pChar, 10 ); pChar++;
			char* pUsername		= _ReadStringTil( ':', pChar, TRUE );
			unsigned int nScore = strtol( pChar, &pChar, 10 ); pChar++;		//	user's best score, even if DB is different
			unsigned int nRank	= strtol( pChar, &pChar, 10 ); pChar++;		//	Rank for this DB
			
			RA_Leaderboard* pThisLB = g_LeaderboardManager.FindLB( nLBID );
			if( pThisLB != NULL )
			{
				pThisLB->ClearRankInfo();
				time_t nNow = time(NULL);
				pThisLB->SubmitRankInfo( nRank, pUsername, nScore, nNow );

				while( *pChar != '\0' )
				{
					unsigned int nNextRank	= strtol( pChar, &pChar, 10 ); pChar++;
					char* sNextUser			= _ReadStringTil( ':', pChar, TRUE );
					unsigned int nNextScore	= strtol( pChar, &pChar, 10 ); pChar++;
					time_t nAchievedAt		= (time_t)atol( _ReadStringTil( '\n', pChar, TRUE ) );
					
					pThisLB->SubmitRankInfo( nNextRank, sNextUser, nNextScore, nAchievedAt );
				}
				
				pThisLB->SortRankInfo();

				char sTestData[4096];
				sprintf_s( sTestData, 4096, "Leaderboard for %s (%s)\n\n", pThisLB->Title(), pThisLB->Description() );
				for( size_t i = 0; i < pThisLB->GetRankInfoCount(); ++i )
				{
					const LB_Entry& NextScore = pThisLB->GetRankInfo( i );
					char bufferScore[256];
					
					RA_Leaderboard::FormatScore( pThisLB->GetFormatType(), NextScore.m_nScore, bufferScore, 256 );

					char bufferMessage[512];
					sprintf_s( bufferMessage, 512, "%02d: %s - %s\n", NextScore.m_nRank, NextScore.m_sUsername, bufferScore );
					strcat_s( sTestData, 4096, bufferMessage );
				}

				g_PopupWindows.LeaderboardPopups().ShowScoreboard( pThisLB->ID() );
				//g_PopupWindows.AchievementPopups().AddMessage( "Successfully posted new score!", "Desc", MSG_LEADERBOARD_INFO, NULL );
			}
		}
		else
		{
			//	Failed
		}
	}
}

void RA_LeaderboardManager::AddLeaderboard( const RA_Leaderboard& lb )
{
	if( g_bLeaderboardsActive )	//	If not, simply ignore them.
		m_Leaderboards.push_back( lb );
}

void RA_LeaderboardManager::Test()
{
	std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
	while( iter != m_Leaderboards.end() )
	{
		(*iter).Test();
		iter++;
	}
}

void RA_LeaderboardManager::Reset()
{
	std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
	while( iter != m_Leaderboards.end() )
	{
		(*iter).Reset();
		iter++;
	}
}
