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
		CodeNoteObj( std::string sAuthor, std::string sAddr, std::string sNote ) : 
		  m_sAuthor(sAuthor), m_sAddress(sAddr), m_sNote(sNote) {}

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
	static void s_OnUpdateCB( void* pObj );

	void Sort();
	BOOL Exists( const char* sAddress, char* sAuthorOut, char* sDescriptionOut, const size_t nMaxLen=512 );
	BOOL ExistsRef( const char* sAddress, std::string*& psDescOut );
	void Add( const char* sAuthor, const char* sAddress, const char* sDescription );
	BOOL Remove( const char* sAddress );

	const char* GetAddress( size_t nIndex )	{ return m_sCodeNotes[nIndex].m_sAddress.c_str(); }
	const char* GetDescription( size_t nIndex )	{ return m_sCodeNotes[nIndex].m_sNote.c_str(); }

private:
	std::vector< CodeNoteObj > m_sCodeNotes;
};