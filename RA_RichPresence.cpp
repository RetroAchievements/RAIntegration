#include "RA_RichPresence.h"
#include "RA_Core.h"

RA_RichPresenceInterpretter g_RichPresenceInterpretter;

RA_Lookup::RA_Lookup( const std::string& sDesc )
 :	m_sLookupDescription( sDesc )
{
}

const std::string& RA_Lookup::Lookup( DataPos nValue ) const
{
	if( m_lookupData.find( nValue ) != m_lookupData.end() )
		return m_lookupData.find( nValue )->second;

	static const std::string sUnknown = "";
	return sUnknown;
}

RA_Formattable::RA_Formattable( const std::string& sDesc, RA_Leaderboard::FormatType nType )
 :	m_sLookupDescription( sDesc ),
	m_nFormatType( nType )
{
}

std::string RA_Formattable::Lookup( DataPos nValue ) const
{
	return RA_Leaderboard::FormatScore( m_nFormatType, nValue );
}

void RA_RichPresenceInterpretter::ParseRichPresenceFile( const std::string& sFilename )
{
	m_formats.clear();
	m_lookups.clear();
	m_sDisplay.clear();

	const char EndLine = '\n';

	const char* LookupStr = "Lookup:";
	const char* FormatStr = "Format:";
	const char* FormatTypeStr = "FormatType=";
	const char* DisplayableStr = "Display:";

	FILE* pFile = NULL;
	fopen_s( &pFile, sFilename.c_str(), "r" );
	if( pFile != NULL )
	{
		DWORD nCharsRead = 0;
		char buffer[4096];

		_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
		while( nCharsRead != 0 )
		{
			if( strncmp( LookupStr, buffer, strlen( LookupStr ) ) == 0 )
			{
				//	Lookup type
				char* pBuf = buffer + ( strlen( LookupStr ) );
				RA_Lookup newLookup( _ReadStringTil( EndLine, pBuf, TRUE ) );
				while( nCharsRead != 0 && ( buffer[0] != EndLine ) )
				{
					_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
					if( nCharsRead > 2 )
					{
						char* pBuf = &buffer[0];
						const char* pValue = _ReadStringTil( '=', pBuf, TRUE );
						const char* pName = _ReadStringTil( EndLine, pBuf, TRUE );

						int nBase = 10;
						if( pValue[0] == '0' && pValue[1] == 'x' )
							nBase = 16;

						DataPos nVal = static_cast<DataPos>( strtol( pValue, NULL, nBase ) );

						newLookup.AddLookupData( nVal, pName );
					}
				}

				RA_LOG( "RP: Adding Lookup %s\n", newLookup.Description().c_str() );
				m_lookups.push_back( newLookup );

			}
			else if( strncmp( FormatStr, buffer, strlen( FormatStr ) ) == 0 )
			{
				//	
				char* pBuf = &buffer[0];
				const char* pUnused = _ReadStringTil( ':', pBuf, TRUE );
				std::string sDesc = _ReadStringTil( EndLine, pBuf, TRUE );

				_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
				if( nCharsRead > 0 && strncmp( FormatTypeStr, buffer, strlen( FormatTypeStr ) ) == 0 )
				{
					char* pBuf = &buffer[0];
					const char* pUnused = _ReadStringTil( '=', pBuf, TRUE );
					const char* pType = _ReadStringTil( EndLine, pBuf, TRUE );
					
					RA_Leaderboard::FormatType nType;

					if( strcmp( pType, "SCORE" ) == 0 || strcmp( pType, "POINTS" ) == 0 )
					{
						nType = RA_Leaderboard::Format_Score;
					}
					else if( strcmp( pType, "TIME" ) == 0 || strcmp( pType, "FRAMES" ) == 0 )
					{
						nType = RA_Leaderboard::Format_TimeFrames;
					}
					else if( strcmp( pType, "SECS" ) == 0 )
					{
						nType = RA_Leaderboard::Format_TimeSecs;
					}
					else if( strcmp( pType, "MILLISECS" ) == 0 )
					{
						nType = RA_Leaderboard::Format_TimeMillisecs;
					}
					else if( strcmp( pType, "VALUE" ) == 0 )
					{
						nType = RA_Leaderboard::Format_Value;
					}
					else
					{
						nType = RA_Leaderboard::Format_Other;
					}

					RA_LOG( "RP: Adding Formatter %s (%s)\n", sDesc.c_str(), pType );
					m_formats.push_back( RA_Formattable( sDesc, nType ) );
				}
			}
			else if( strncmp( DisplayableStr, buffer, strlen( DisplayableStr ) ) == 0 )
			{
				_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
				
				char* pBuf =  &buffer[0];
				m_sDisplay = _ReadStringTil( '\n', pBuf, TRUE );	//	Terminates \n instead
			}

			_ReadTil( EndLine, buffer, 4096, &nCharsRead, pFile );
		}

		fclose( pFile );
	}
}

const std::string& RA_RichPresenceInterpretter::Lookup( const std::string& sName, const std::string& sMemString ) const
{
	static std::string sReturnVal;
	sReturnVal.clear();

	//	check lookups
	for( size_t i = 0; i < m_lookups.size(); ++i )
	{
		if( m_lookups.at( i ).Description().compare( sName ) == 0 )
		{
			//	This lookup! (ugh must be non-const)
			char buffer[1024];
			sprintf_s( buffer, 1024, (char*)sMemString.c_str() );

			ValueSet nValue;
			nValue.ParseMemString( buffer );
			sReturnVal = m_lookups.at( i ).Lookup( static_cast<DataPos>( nValue.GetValue() ) );

			return sReturnVal;
		}
	}

	//	check formatters
	for( size_t i = 0; i < m_formats.size(); ++i )
	{
		if( m_formats.at( i ).Description().compare( sName ) == 0 )
		{
			//	This lookup! (ugh must be non-const)
			char buffer[1024];
			sprintf_s( buffer, 1024, (char*)sMemString.c_str() );

			ValueSet nValue;
			nValue.ParseMemString( buffer );
			sReturnVal = m_formats.at( i ).Lookup( static_cast<DataPos>( nValue.GetValue() ) );

			return sReturnVal;
		}
	}

	return sReturnVal;
}

bool RA_RichPresenceInterpretter::Enabled() const
{
	return !m_sDisplay.empty();
}

const std::string& RA_RichPresenceInterpretter::GetRichPresenceString()
{
	static std::string sReturnVal;
	sReturnVal.clear();

	bool bParsingLookupName = false;
	bool bParsingLookupContent = false;

	std::string sLookupName;
	std::string sLookupValue;

	for( size_t i = 0; i < m_sDisplay.size(); ++i )
	{
		char c = m_sDisplay.at( i );

		if( bParsingLookupContent )
		{
			if( c == ')' )
			{
				//	End of content
				bParsingLookupContent = false;	

				//	Lookup!

				sReturnVal.append( Lookup( sLookupName, sLookupValue ) );

				sLookupName.clear();
				sLookupValue.clear();
			}
			else
			{
				sLookupValue.push_back( c );
			}
		}
		else if( bParsingLookupName )
		{
			if( c == '(' )
			{
				//	Now parsing lookup content
				bParsingLookupContent = true;
				bParsingLookupName = false;
			}
			else
			{
				//	Continue to parse lookup name
				sLookupName.push_back( c );
			}
		}
		else
		{
			//	Not parsing a lookup at all.
			if( c == '@' )
			{
				bParsingLookupName = true;
			}
			else
			{
				//	Standard text.
				sReturnVal.push_back( c );
			}
		}
	}

	return sReturnVal;
}

//	static
void RA_RichPresenceInterpretter::PersistAndParseScript( GameID nGameID, const std::string& str )
{
	//	Read to file:
	SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
	_WriteBufferToFile( RA_DIR_DATA + std::to_string( nGameID ) + "-Rich.txt", str );
						
	//	Then install it
	g_RichPresenceInterpretter.ParseRichPresenceFile( RA_DIR_DATA + std::to_string( nGameID ) + "-Rich.txt" );
}