#include "RA_Leaderboard.h"
#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"

#include <time.h>

RA_LeaderboardManager g_LeaderboardManager;

namespace
{
	const char* FormatTypeToString[] =
	{
		"TIME",			//	TimeFrames
		"TIMESECS",		//	TimeSecs
		"MILLISECS",	//	TimeMillisecs
		"SCORE",		//	Score	/POINTS
		"VALUE",		//	Value
		"OTHER",		//	Other
	};
	static_assert( SIZEOF_ARRAY( FormatTypeToString ) == RA_Leaderboard::Format__MAX, "These must match!" );
}

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
		if( m_nVarSize == ComparisonVariableSize::Nibble_Lower )
		{
			nRetVal = g_MemManager.ActiveBankRAMByteRead( m_nAddress ) & 0xf;
		}
		else if( m_nVarSize == ComparisonVariableSize::Nibble_Upper )
		{
			nRetVal = ( g_MemManager.ActiveBankRAMByteRead( m_nAddress ) >> 4 ) & 0xf;
		}
		else if( m_nVarSize == ComparisonVariableSize::EightBit )
		{
			nRetVal = g_MemManager.ActiveBankRAMByteRead( m_nAddress );
		}
		else if( m_nVarSize == ComparisonVariableSize::SixteenBit )
		{
			nRetVal = g_MemManager.ActiveBankRAMByteRead( m_nAddress );
			nRetVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nAddress + 1 ) << 8 );
		}
		else if( m_nVarSize == ComparisonVariableSize::ThirtyTwoBit )
		{
			nRetVal = g_MemManager.ActiveBankRAMByteRead( m_nAddress );
			nRetVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nAddress + 1 ) << 8 );
			nRetVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nAddress + 2 ) << 16 );
			nRetVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nAddress + 3 ) << 24 );
		}
		else if( ( m_nVarSize >= ComparisonVariableSize::Bit_0 ) && ( m_nVarSize <= ComparisonVariableSize::Bit_7 ) )
		{
			const unsigned int nBitmask = ( 1 << ( m_nVarSize - ComparisonVariableSize::Bit_0 ) );
			nRetVal = ( g_MemManager.ActiveBankRAMByteRead( m_nAddress ) & nBitmask ) != 0;
		}
		//nRetVal = g_MemManager.RAMByte( m_nAddress );

		if( m_bBCDParse )
		{
			//	Reparse this value as a binary coded decimal.
			nRetVal = ( ( ( nRetVal >> 4 ) & 0xf ) * 10 ) + ( nRetVal & 0xf );
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
	m_nAddress = varTemp.RawValue();	//	Fetch value ('address') as parsed. Note RawValue! Do not parse memory!
	m_nVarSize = varTemp.Size();

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
RA_Leaderboard::RA_Leaderboard( const unsigned nLeaderboardID ) :
	m_nID( nLeaderboardID ),
	m_bStarted( false ),
	m_format( Format_Value )
{
}

RA_Leaderboard::~RA_Leaderboard()
{
}

//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::LoadFromJSON( const Value& element )
{
	const LeaderboardID nID = element["ID"].GetUint();
	ASSERT( nID == m_nID );	//	Must match!

	const std::string sMem = element["Mem"].GetString();
	char buffer[4096];
	strcpy_s( buffer, 4096, sMem.c_str() );
	ParseLBData( buffer );

	m_sTitle = element["Title"].GetString();
	m_sDescription = element["Description"].GetString();

	const std::string sFmt = element["Format"].GetString();
	for( size_t i = 0; i < Format__MAX; ++i )
	{
		if( sFmt.compare( FormatTypeToString[i] ) == 0 )
		{
			m_format = (FormatType)i;
			break;
		}
	}
}

void RA_Leaderboard::ParseLBData( char* pChar )
{
	while( ( ( *pChar ) != '\n' ) && ( ( *pChar ) != '\0' ) )
	{
		if( ( pChar[ 0 ] == ':' ) && ( pChar[ 1 ] == ':' ) )	//	New Phrase (double colon)
			pChar += 2;

		if( std::string( "STA:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;

			//	Parse Start condition
			do 
			{
				while( ( *pChar ) == ' ' || ( *pChar ) == '_' || ( *pChar ) == '|' )
					pChar++; // Skip any chars up til this point :S
				
				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_startCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( std::string( "CAN:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Parse Cancel condition
			do 
			{
				while( ( *pChar ) == ' ' || ( *pChar ) == '_' || ( *pChar ) == '|' )
					pChar++; // Skip any chars up til this point :S
				
				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_cancelCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( std::string( "SUB:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Parse Submit condition
			do 
			{
				while( ( *pChar ) == ' ' || ( *pChar ) == '_' || ( *pChar ) == '|' )
					pChar++; // Skip any chars up til this point :S

				Condition nNewCond;
				nNewCond.ParseFromString( pChar );
				m_submitCond.Add( nNewCond );
			}
			while( *pChar == '_' );
		}
		else if( std::string( "VAL:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Parse Value condition
			//	Value is a collection of memory addresses and modifiers.
			do 
			{
				while( ( *pChar ) == ' ' || ( *pChar ) == '_' || ( *pChar ) == '|' )
					pChar++; // Skip any chars up til this point :S

				MemValue newMemVal;
				pChar = newMemVal.ParseFromString( pChar );
				m_value.AddNewValue( newMemVal );
			}
			while( *pChar == '_' );
		}
		else if( std::string( "PRO:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Progress: normally same as value:
			//	Progress is a collection of memory addresses and modifiers.
			do 
			{
				while( ( *pChar ) == ' ' || ( *pChar ) == '_' || ( *pChar ) == '|' )
					pChar++; // Skip any chars up til this point :S

				MemValue newMemVal;
				pChar = newMemVal.ParseFromString( pChar );
				m_progress.AddNewValue( newMemVal );
			}
			while( *pChar == '_' );
		}
		else if( std::string( "FOR:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;

			//	Format
			if( strncmp( pChar, "TIMESECS", sizeof( "TIMESECS" ) - 1 ) == 0 )
			{
				m_format = Format_TimeSecs;
				pChar += sizeof( "TIMESECS" ) - 1;
			}
			else if( strncmp( pChar, "TIME", sizeof( "TIME" ) - 1 ) == 0 )
			{
				m_format = Format_TimeFrames;
				pChar += sizeof( "TIME" ) - 1;
			}
			else if( strncmp( pChar, "MILLISECS", sizeof( "MILLISECS" ) - 1 ) == 0 )
			{
				m_format = Format_TimeMillisecs;
				pChar += sizeof( "MILLISECS" ) - 1;
			}
			else if( strncmp( pChar, "POINTS", sizeof( "POINTS" ) - 1 ) == 0 )
			{
				m_format = Format_Score;
				pChar += sizeof( "POINTS" ) - 1;
			}
			else if( strncmp( pChar, "SCORE", sizeof( "SCORE" ) - 1 ) == 0 )	//dupe
			{
				m_format = Format_Score;
				pChar += sizeof( "SCORE" ) - 1;
			}
			else if( strncmp( pChar, "VALUE", sizeof( "VALUE" ) - 1 ) == 0 )
			{
				m_format = Format_Value;
				pChar += sizeof( "VALUE" ) - 1;
			}
			else
			{
				m_format = Format_Other;
				while( isalnum( *pChar ) )
					pChar++;
			}
		}
		else if( std::string( "TTL:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Title:
			char* pDest = &m_sTitle[ 0 ];
			while( !( pChar[ 0 ] == ':' && pChar[ 1 ] == ':' ) )
			{
				*pDest = *pChar;	//	char copy
				pDest++;
				pChar++;
			}
			*pDest = '\0';
		}
		else if( std::string( "DES:" ).compare( 0, 4, pChar, 0, 4 ) == 0 )
		{
			pChar += 4;
			//	Description:
			char* pDest = &m_sDescription[ 0 ];
			while( ( pChar[ 0 ] != '\n' ) && ( pChar[ 0 ] != '\0' ) )
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
			ASSERT( !"Badly formatted: this leaderboard makes no sense!" );
			break;
		}
	}
}

void RA_Leaderboard::ParseLine( char* sBuffer )
{
	char* pChar = &sBuffer[ 0 ];
	pChar++;											//	Skip over 'L' character
	ASSERT( m_nID == strtol( pChar, &pChar, 10 ) );		//	Skip over Leaderboard ID
	ParseLBData( pChar );
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
	BOOL bStartOK = m_startCond.Test( bUnused, bUnused, FALSE );
	BOOL bCancelOK = m_cancelCond.Test( bUnused, bUnused, TRUE );
	BOOL bSubmitOK = m_submitCond.Test( bUnused, bUnused, FALSE );

	if( !m_bStarted )
	{
		if( bStartOK )
		{
			m_bStarted = true;

			g_PopupWindows.AchievementPopups().AddMessage( 
				MessagePopup( "Challenge Available: " + m_sTitle,
							  m_sDescription,
							  PopupLeaderboardInfo,
							  nullptr ) );
			g_PopupWindows.LeaderboardPopups().Activate( m_nID );
		}
	}
	else
	{
 		if( bCancelOK )
		{
			m_bStarted = false;
			g_PopupWindows.LeaderboardPopups().Deactivate( m_nID );
			
			g_PopupWindows.AchievementPopups().AddMessage( 
				MessagePopup( "Leaderboard attempt cancelled!",
							  m_sTitle,
							  PopupLeaderboardCancel,
							  nullptr ) );
		}
		else if( bSubmitOK )
		{
			m_bStarted = false;
			g_PopupWindows.LeaderboardPopups().Deactivate( m_nID );

			if( g_bRAMTamperedWith )
			{
				g_PopupWindows.AchievementPopups().AddMessage( 
					MessagePopup( "Not posting to leaderboard: memory tamper detected!",
								  "Reset game to reenable posting.",
								  PopupInfo ) );
			}
			else
			{
				//	TBD: move to keys!
				char sValidationSig[ 50 ];
				sprintf_s( sValidationSig, 50, "%d%s%d", m_nID, RAUsers::LocalUser().Username().c_str(), m_nID );
				std::string sValidationMD5 = RAGenerateMD5( sValidationSig );

				PostArgs args;
				args['u'] = RAUsers::LocalUser().Username();
				args['t'] = RAUsers::LocalUser().Token();
				args['i'] = std::to_string( m_nID );
				args['v'] = sValidationMD5;
				args['s'] = std::to_string( static_cast<int>( m_value.GetValue() ) );	//	Concern about rounding?

				RAWeb::CreateThreadedHTTPRequest( RequestSubmitLeaderboardEntry, args );
			}
		}
	}
}

void RA_Leaderboard::SubmitRankInfo( unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved )
{
	LB_Entry newEntry;
	newEntry.m_nRank = nRank;
	newEntry.m_sUsername = sUsername;
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

			if( m_RankInfo.at( i ).m_nRank > m_RankInfo.at( j ).m_nRank )
			{
				LB_Entry temp = m_RankInfo[ i ];
				m_RankInfo[ i ] = m_RankInfo[ j ];
				m_RankInfo[ j ] = temp;
			}
		}
	}
}

//	static
std::string RA_Leaderboard::FormatScore( FormatType nType, int nScoreIn )
{
	//	NB. This is accessed by RA_Formattable... ought to be refactored really
	char buffer[ 256 ];
	switch( nType )
	{
		case Format_TimeFrames:
		{
			int nMins = nScoreIn / 3600;
			int nSecs = ( nScoreIn % 3600 ) / 60;
			int nMilli = static_cast<int>( ( ( nScoreIn % 3600 ) % 60 ) * ( 100.0 / 60.0 ) );	//	Convert from frames to 'millisecs'
			sprintf_s( buffer, 256, "%02d:%02d.%02d", nMins, nSecs, nMilli );
		}
		break;

		case Format_TimeSecs:
		{
			int nMins = nScoreIn / 60;
			int nSecs = nScoreIn % 60;
			sprintf_s( buffer, 256, "%02d:%02d", nMins, nSecs );
		}
		break;

		case Format_TimeMillisecs:
		{
			int nMins = nScoreIn / 6000;
			int nSecs = ( nScoreIn % 6000 ) / 100;
			int nMilli = static_cast<int>( nScoreIn % 100 );
			sprintf_s( buffer, 256, "%02d:%02d.%02d", nMins, nSecs, nMilli );
		}
		break;

		case Format_Score:
			sprintf_s( buffer, 256, "%06d Points", nScoreIn );
			break;

		case Format_Value:
			sprintf_s( buffer, 256, "%01d", nScoreIn );
			break;

		default:
			sprintf_s( buffer, 256, "%06d", nScoreIn );
			break;
	}
	return buffer;
}

std::string RA_Leaderboard::FormatScore( int nScoreIn ) const
{
	return FormatScore( m_format, nScoreIn );
}

RA_Leaderboard* RA_LeaderboardManager::FindLB( LeaderboardID nID )
{
	std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
	while( iter != m_Leaderboards.end() )
	{
		if( (*iter).ID() == nID )
			return &(*iter);

		iter++;
	}

	return nullptr;
}


//////////////////////////////////////////////////////////////////////////
//static
void RA_LeaderboardManager::OnSubmitEntry( const Document& doc )
{
	if( !doc.HasMember( "Response" ) )
	{
		ASSERT( !"Cannot process this LB Response!" );
		return;
	}

	const Value& Response = doc[ "Response" ];

	const Value& LBData = Response[ "LBData" ];

	const std::string& sFormat = LBData[ "Format" ].GetString();
	const LeaderboardID nLBID = static_cast<LeaderboardID>( LBData[ "LeaderboardID" ].GetUint() );
	const GameID nGameID = static_cast<GameID>( LBData[ "GameID" ].GetUint() );
	const std::string& sLBTitle = LBData[ "Title" ].GetString();
	const bool bLowerIsBetter = ( LBData[ "LowerIsBetter" ].GetUint() == 1 );

	RA_Leaderboard* pLB = g_LeaderboardManager.FindLB( nLBID );

	const int nSubmittedScore = Response[ "Score" ].GetInt();
	const int nBestScore = Response[ "BestScore" ].GetInt();
	const std::string& sScoreFormatted = Response[ "ScoreFormatted" ].GetString();

	pLB->ClearRankInfo();

	RA_LOG( "LB Data, Top Entries:\n" );
	const Value& TopEntries = Response[ "TopEntries" ];
	for( SizeType i = 0; i < TopEntries.Size(); ++i )
	{
		const Value& NextEntry = TopEntries[ i ];

		const unsigned int nRank = NextEntry[ "Rank" ].GetUint();
		const std::string& sUser = NextEntry[ "User" ].GetString();
		const int nUserScore = NextEntry[ "Score" ].GetInt();
		time_t nSubmitted = NextEntry[ "DateSubmitted" ].GetUint();

		RA_LOG( std::string( "(" + std::to_string( nRank ) + ") " + sUser + ": " + pLB->FormatScore( nUserScore ) ).c_str() );

		pLB->SubmitRankInfo( nRank, sUser, nUserScore, nSubmitted );
	}

	pLB->SortRankInfo();

	const Value& TopEntriesFriends = Response[ "TopEntriesFriends" ];
	const Value& RankData = Response[ "RankInfo" ];

	//	TBD!
	//char sTestData[ 4096 ];
	//sprintf_s( sTestData, 4096, "Leaderboard for %s (%s)\n\n", pLB->Title().c_str(), pLB->Description().c_str() );
	//for( size_t i = 0; i < pLB->GetRankInfoCount(); ++i )
	//{
	//	const LB_Entry& NextScore = pLB->GetRankInfo( i );

	//	std::string sScoreFormatted = pLB->FormatScore( NextScore.m_nScore );

	//	char bufferMessage[ 512 ];
	//	sprintf_s( bufferMessage, 512, "%02d: %s - %s\n", NextScore.m_nRank, NextScore.m_sUsername, sScoreFormatted.c_str() );
	//	strcat_s( sTestData, 4096, bufferMessage );
	//}

	g_PopupWindows.LeaderboardPopups().ShowScoreboard( pLB->ID() );
}

void RA_LeaderboardManager::AddLeaderboard( const RA_Leaderboard& lb )
{
	if( g_bLeaderboardsActive )	//	If not, simply ignore them.
		m_Leaderboards.push_back( lb );
}

void RA_LeaderboardManager::Test()
{
	if( g_bLeaderboardsActive )
	{
		std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
		while( iter != m_Leaderboards.end() )
		{
			( *iter ).Test();
			iter++;
		}
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
