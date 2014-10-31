#pragma once

#include <string>
#include <utility>
#include <vector>

#include "RA_Defs.h"

class CodeNotes
{
public:
	class CodeNoteObj
	{
	public:
		CodeNoteObj( const std::string& sAuthor, const std::string& sAddr, std::string sNote ) : 
		  m_sAuthor( sAuthor ), m_sAddress( sAddr ), m_sNote( sNote ) {}

	public:
		std::string m_sAuthor;
		std::string m_sAddress;
		std::string m_sNote;
	};
	
public:
	CodeNotes();
	~CodeNotes();

public:
	void Clear();

	BOOL Save( const char* sFile );
	size_t Load( const char* sFile );

	BOOL Update( unsigned int nID );
	static void OnCodeNotesResponse( Document& doc );

	void Sort();
	BOOL Exists( const std::string& sAddress, char* sAuthorOut, char* sDescriptionOut, const size_t nMaxLen=512 );

	BOOL Find( const std::string& sAddress, CodeNoteObj* pNoteOut );

	void Add( const std::string& sAuthor, const std::string& sAddress, const std::string& sDescription );
	BOOL Remove( const std::string& sAddress );

	const char* GetAddress( size_t nIndex )	{ return m_sCodeNotes[nIndex].m_sAddress.c_str(); }
	const char* GetDescription( size_t nIndex )	{ return m_sCodeNotes[nIndex].m_sNote.c_str(); }

private:
	std::vector< CodeNoteObj > m_sCodeNotes;
};