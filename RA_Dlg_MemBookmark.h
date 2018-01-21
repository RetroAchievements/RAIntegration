#pragma once

#include <wtypes.h>
#include <vector>

#include "RA_Defs.h"

class MemBookmark
{
public:
	void SetDescription( const std::wstring& string )		{ m_sDescription = string; }
	void SetAddress( unsigned int nVal )					{ m_nAddress = nVal; }
	void SetType( unsigned int nVal )						{ m_nType = nVal; }
	void SetValue( unsigned int nVal )						{ m_sValue = nVal; }
	void SetPrevious( unsigned int nVal )					{ m_sPrevious = nVal; }
	void IncreaseCount()									{ m_nCount++; }
	void ResetCount()										{ m_nCount = 0; }

	void SetFrozen( bool b )								{ m_bFrozen = b; }
	void SetDecimal( bool b )								{ m_bDecimal = b; }

	inline const std::wstring& Description() const			{ return m_sDescription; }
	unsigned int Address() const							{ return m_nAddress; }
	unsigned int Type() const								{ return m_nType; }
	unsigned int Value() const								{ return m_sValue; }
	unsigned int Previous() const							{ return m_sPrevious; }
	unsigned int Count() const								{ return m_nCount; }

	bool Frozen() const										{ return m_bFrozen; }
	bool Decimal() const									{ return m_bDecimal; }

private:
	std::wstring m_sDescription;
	unsigned int m_nAddress;
	unsigned int m_nType;
	unsigned int m_sValue;
	unsigned int m_sPrevious;
	unsigned int m_nCount = 0;
	bool m_bFrozen = FALSE;
	bool m_bDecimal = FALSE;
};

class Dlg_MemBookmark
{
public:
	//Dlg_MemBookmark();
	//~Dlg_MemBookmark();

	static INT_PTR CALLBACK s_MemBookmarkDialogProc( HWND, UINT, WPARAM, LPARAM );
	INT_PTR MemBookmarkDialogProc( HWND, UINT, WPARAM, LPARAM );

	void InstallHWND( HWND hWnd )						{ m_hMemBookmarkDialog = hWnd; }
	HWND GetHWND() const								{ return m_hMemBookmarkDialog; }

	std::vector<MemBookmark*> Bookmarks()				{ return m_vBookmarks; }
	void UpdateBookmarks( bool bForceWrite );
	void AddBookmark( MemBookmark* newBookmark )		{ m_vBookmarks.push_back(newBookmark); }

	void OnLoad_NewRom();

private:
	int m_nNumOccupiedRows;

	void PopulateList();
	void SetupColumns( HWND hList );
	BOOL EditLabel ( int nItem, int nSubItem );
	void AddAddress();
	void ClearAllBookmarks();
	void WriteFrozenValue( const MemBookmark& Bookmark );
	unsigned int GetMemory( unsigned int nAddr, int type );

	void ExportJSON();
	void ImportFromFile( std::string filename );
	std::string ImportDialog();

	void AddBookmarkMap( MemBookmark* bookmark )
	{
		if (m_BookmarkMap.find( bookmark->Address() ) == m_BookmarkMap.end() )
			m_BookmarkMap.insert( std::map<ByteAddress, std::vector<const MemBookmark*>>::value_type( bookmark->Address(), std::vector<const MemBookmark*>() ) );

		std::vector<const MemBookmark*> *v = &m_BookmarkMap[ bookmark->Address() ];
		v->push_back( bookmark );
	}

public:
	const MemBookmark* FindBookmark( const ByteAddress& nAddr ) const
	{
		std::map<ByteAddress, std::vector<const MemBookmark*>>::const_iterator iter = m_BookmarkMap.find( nAddr );
		return( iter != m_BookmarkMap.end() && iter->second.size() > 0 ) ? iter->second.back() : nullptr;
	}

private:
	HWND m_hMemBookmarkDialog;
	std::map<ByteAddress, std::vector<const MemBookmark*>> m_BookmarkMap;
	std::vector<MemBookmark*> m_vBookmarks;
};

extern Dlg_MemBookmark g_MemBookmarkDialog;