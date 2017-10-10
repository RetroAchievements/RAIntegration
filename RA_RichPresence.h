#pragma once
#include <vector>
#include <map>
#include "RA_Leaderboard.h"
#include "RA_Defs.h"

typedef unsigned int DataPos;


class RA_Lookup
{
public:
	RA_Lookup( const std::string& sDesc );
	
public:
	void AddLookupData( DataPos nValue, const std::string& sLookupData )	{ m_lookupData[ nValue ] = sLookupData; }
	const std::string& Lookup( DataPos nValue ) const;

	const std::string& Description() const									{ return m_sLookupDescription; }

private:
	std::string m_sLookupDescription;
	std::map<DataPos, std::string> m_lookupData;
};

class RA_Formattable
{
public:
	RA_Formattable( const std::string& sDesc, RA_Leaderboard::FormatType nType );

	std::string Lookup( DataPos nValue ) const;

	const std::string& Description() const									{ return m_sLookupDescription; }

private:
	std::string m_sLookupDescription;
	RA_Leaderboard::FormatType m_nFormatType;
};

class RA_RichPresenceInterpretter
{
public:
	static void PersistAndParseScript( GameID nGameID, const std::string& sScript );

public:
	RA_RichPresenceInterpretter() {}

public:
	void ParseRichPresenceFile( const std::string& sFilename );

	const std::string& GetRichPresenceString();
	const std::string& Lookup( const std::string& sLookupName, const std::string& sMemString ) const;

	bool Enabled() const;

private:
	std::vector<RA_Lookup> m_lookups;
	std::vector<RA_Formattable> m_formats;

	std::string m_sDisplay;
};

extern RA_RichPresenceInterpretter g_RichPresenceInterpretter;