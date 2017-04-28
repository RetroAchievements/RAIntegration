#include "RA_Dlg_GameLibrary.h"

#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <queue>
#include <deque>
#include <stack>
#include <thread>
#include <shlobj.h>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"

#define KEYDOWN(vkCode) ((GetAsyncKeyState(vkCode) & 0x8000) ? true : false)

namespace
{
	const char* COL_TITLE[] = { "ID", "Game Title", "Completion", "File Path" };
	const int COL_SIZE[] = { 30, 230, 110, 170 };
	static_assert( SIZEOF_ARRAY( COL_TITLE ) == SIZEOF_ARRAY( COL_SIZE ), "Must match!" );
	const bool bCancelScan = false;

	std::mutex mtx;
}

//static 
std::deque<std::string> Dlg_GameLibrary::FilesToScan;
std::map<std::string, std::string> Dlg_GameLibrary::Results;	//	filepath,md5
std::map<std::string, std::string> Dlg_GameLibrary::VisibleResults;	//	filepath,md5
size_t Dlg_GameLibrary::nNumParsed = 0;
bool Dlg_GameLibrary::ThreadProcessingAllowed = true;
bool Dlg_GameLibrary::ThreadProcessingActive = false;

Dlg_GameLibrary g_GameLibrary;

bool ListFiles( std::string path, std::string mask, std::deque<std::string>& rFileListOut )
{
	std::stack<std::string> directories;
	directories.push( path );

	while( !directories.empty() )
	{
		path = directories.top();
		std::string spec = path + "\\" + mask;
		directories.pop();

		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFile( Widen( spec ).c_str(), &ffd );
		if( hFind == INVALID_HANDLE_VALUE )
			return false;

		do
		{
			std::string sFilename = Narrow( ffd.cFileName );
			if( ( strcmp( sFilename.c_str(), "." ) == 0 ) ||
				( strcmp( sFilename.c_str(), ".." ) == 0 ) )
				continue;

			if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				directories.push( path + "\\" + sFilename );
			else
				rFileListOut.push_front( path + "\\" + sFilename );
		}
		while( FindNextFile( hFind, &ffd ) != 0 );

		if( GetLastError() != ERROR_NO_MORE_FILES )
		{
			FindClose( hFind );
			return false;
		}

		FindClose( hFind );
		hFind = INVALID_HANDLE_VALUE;
	}

	return true;
}

//HWND Dlg_GameLibrary::m_hDialogBox = NULL;
//std::vector<GameEntry> Dlg_GameLibrary::m_vGameEntries;
//std::map<std::string, unsigned int> Dlg_GameLibrary::m_GameHashLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_GameTitlesLibrary;
//std::map<unsigned int, std::string> Dlg_GameLibrary::m_ProgressLibrary;
Dlg_GameLibrary::Dlg_GameLibrary()
 :	m_hDialogBox( nullptr )
{
}

Dlg_GameLibrary::~Dlg_GameLibrary()
{
}

void ParseGameHashLibraryFromFile( std::map<std::string, GameID>& GameHashLibraryOut )
{
	SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
	FILE* pf = NULL;
	fopen_s( &pf, RA_GAME_HASH_FILENAME, "rb" );
	if( pf != NULL )
	{
		Document doc;
		doc.ParseStream( FileStream( pf ) );

		if( !doc.HasParseError() && doc.HasMember( "Success" ) && doc[ "Success" ].GetBool() && doc.HasMember( "MD5List" ) )
		{
			const Value& List = doc[ "MD5List" ];
			for( Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter )
			{
				if( iter->name.IsNull() || iter->value.IsNull() )
					continue;

				const std::string sMD5 = iter->name.GetString();
				//GameID nID = static_cast<GameID>( std::strtoul( iter->value.GetString(), NULL, 10 ) );	//	MUST BE STRING, then converted to uint. Keys are strings ONLY
				GameID nID = static_cast<GameID>( iter->value.GetUint() );
				GameHashLibraryOut[ sMD5 ] = nID;
			}
		}

		fclose( pf );
	}
}

void ParseGameTitlesFromFile( std::map<GameID, std::string>& GameTitlesListOut )
{
	SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
	FILE* pf = nullptr;
	fopen_s( &pf, RA_TITLES_FILENAME, "rb" );
	if( pf != nullptr )
	{
		Document doc;
		doc.ParseStream( FileStream( pf ) );

		if( !doc.HasParseError() && doc.HasMember( "Success" ) && doc[ "Success" ].GetBool() && doc.HasMember( "Response" ) )
		{
			const Value& List = doc[ "Response" ];
			for( Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter )
			{
				if( iter->name.IsNull() || iter->value.IsNull() )
					continue;

				GameID nID = static_cast<GameID>( std::strtoul( iter->name.GetString(), nullptr, 10 ) );	//	KEYS ARE STRINGS, must convert afterwards!
				const std::string sTitle = iter->value.GetString();
				GameTitlesListOut[ nID ] = sTitle;
			}
		}

		fclose( pf );
	}
}

void ParseMyProgressFromFile( std::map<GameID, std::string>& GameProgressOut )
{
	FILE* pf = nullptr;
	fopen_s( &pf, RA_MY_PROGRESS_FILENAME, "rb" );
	if( pf != nullptr )
	{
		Document doc;
		doc.ParseStream( FileStream( pf ) );

		if( !doc.HasParseError() && doc.HasMember( "Success" ) && doc[ "Success" ].GetBool() && doc.HasMember( "Response" ) )
		{
			//{"ID":"7","NumAch":"14","Earned":"10","HCEarned":"0"},

			const Value& List = doc[ "Response" ];
			for( Value::ConstMemberIterator iter = List.MemberBegin(); iter != List.MemberEnd(); ++iter )
			{
				GameID nID = static_cast<GameID>( std::strtoul( iter->name.GetString(), nullptr, 10 ) );	//	KEYS MUST BE STRINGS
				const unsigned int nNumAchievements = iter->value[ "NumAch" ].GetUint();
				const unsigned int nEarned = iter->value[ "Earned" ].GetUint();
				const unsigned int nEarnedHardcore = iter->value[ "HCEarned" ].GetUint();

				std::stringstream sstr;
				sstr << nEarned;
				if( nEarnedHardcore > 0 )
					sstr << " (" << std::to_string( nEarnedHardcore ) << ")";
				sstr << " / " << nNumAchievements;
				if( nNumAchievements > 0 )
				{
					const int nNumEarnedTotal = nEarned + nEarnedHardcore;
					char bufPct[ 256 ];
					sprintf_s( bufPct, 256, " (%1.1f%%)", ( nNumEarnedTotal / static_cast<float>( nNumAchievements ) ) * 100.0f );
					sstr << bufPct;
				}

				GameProgressOut[ nID ] = sstr.str();
			}
		}

		fclose( pf );
	}
}

void Dlg_GameLibrary::SetupColumns( HWND hList )
{
	//	Remove all columns,
	while( ListView_DeleteColumn( hList, 0 ) ) {}

	//	Remove all data.
	ListView_DeleteAllItems( hList );

	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	for( size_t i = 0; i < SIZEOF_ARRAY( COL_TITLE ); ++i )
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		col.cchTextMax = 255;
		std::wstring sCol = Widen( COL_TITLE[ i ] ).c_str();	//	scoped cache
		col.pszText = const_cast<LPWSTR>( sCol.c_str() );
		col.cx = COL_SIZE[ i ];
		col.iSubItem = i;

		col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
		if( i == SIZEOF_ARRAY( COL_TITLE ) - 1 )	//	Final column should fill
			col.fmt |= LVCFMT_FILL;

		ListView_InsertColumn( hList, i, reinterpret_cast<LPARAM>( &col ) );
	}
}

//static
void Dlg_GameLibrary::AddTitle( const std::string& sTitle, const std::string& sFilename, GameID nGameID )
{
	LV_ITEM item;
	ZeroMemory( &item, sizeof( item ) );
	item.mask = LVIF_TEXT;
	item.cchTextMax = 255;
	item.iItem = m_vGameEntries.size();

	HWND hList = GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST );

	//	id:
	item.iSubItem = 0;
	std::wstring sIDWide = std::to_wstring( nGameID );	//scoped cache!
	item.pszText = const_cast<LPWSTR>( sIDWide.c_str() );
	item.iItem = ListView_InsertItem( hList, &item );

	item.iSubItem = 1;
	ListView_SetItemText( hList, item.iItem, 1, const_cast<LPWSTR>( Widen( sTitle ).c_str() ) );

	item.iSubItem = 2;
	ListView_SetItemText( hList, item.iItem, 2, const_cast<LPWSTR>( Widen( m_ProgressLibrary[ nGameID ] ).c_str() ) );

	item.iSubItem = 3;
	ListView_SetItemText( hList, item.iItem, 3, const_cast<LPWSTR>( Widen( sFilename ).c_str() ) );

	m_vGameEntries.push_back( GameEntry( sTitle, sFilename, nGameID ) );
}

void Dlg_GameLibrary::ClearTitles()
{
	nNumParsed = 0;

	m_vGameEntries.clear();

	ListView_DeleteAllItems( GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST ) );
	VisibleResults.clear();
}

//static
void Dlg_GameLibrary::ThreadedScanProc()
{
	Dlg_GameLibrary::ThreadProcessingActive = true;

	while( FilesToScan.size() > 0 )
	{
		mtx.lock();
		if( !Dlg_GameLibrary::ThreadProcessingAllowed )
		{
			mtx.unlock();
			break;
		}
		mtx.unlock();

		FILE* pf = nullptr;
		if( fopen_s( &pf, FilesToScan.front().c_str(), "rb" ) == 0 )
		{
			// obtain file size:
			fseek( pf, 0, SEEK_END );
			DWORD nSize = ftell( pf );
			rewind( pf );

			static BYTE* pBuf = nullptr;	//	static (optimisation)
			if( pBuf == nullptr )
				pBuf = new BYTE[ 6 * 1024 * 1024 ];

			//BYTE* pBuf = new BYTE[ nSize ];	//	Do we really need to load this into memory?
			if( pBuf != nullptr )
			{
				fread( pBuf, sizeof( BYTE ), nSize, pf );	//Check
				Results[ FilesToScan.front() ] = RAGenerateMD5( pBuf, nSize );

				SendMessage( g_GameLibrary.GetHWND(), WM_TIMER, NULL, NULL );
			}

			fclose( pf );
		}
		
		mtx.lock();
		FilesToScan.pop_front();
		mtx.unlock();
	}
	
	Dlg_GameLibrary::ThreadProcessingActive = false;
	ExitThread( 0 );
}

void Dlg_GameLibrary::ScanAndAddRomsRecursive( const std::string& sBaseDir )
{
	char sSearchDir[ 2048 ];
	sprintf_s( sSearchDir, 2048, "%s\\*.*", sBaseDir.c_str() );

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile( Widen( sSearchDir ).c_str(), &ffd );
	if( hFind != INVALID_HANDLE_VALUE )
	{
		unsigned int ROM_MAX_SIZE = 6 * 1024 * 1024;
		unsigned char* sROMRawData = new unsigned char[ ROM_MAX_SIZE ];

		do
		{
			if( KEYDOWN( VK_ESCAPE ) )
				break;

			memset( sROMRawData, 0, ROM_MAX_SIZE );	//?!??

			const std::string sFilename = Narrow( ffd.cFileName );
			if( strcmp( sFilename.c_str(), "." ) == 0 || 
				strcmp( sFilename.c_str(), ".." ) == 0 )
			{
				//	Ignore 'this'
			}
			else if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				RA_LOG( "Directory found: %s\n", ffd.cFileName );
				std::string sRecurseDir = sBaseDir + "\\" + sFilename.c_str();
				ScanAndAddRomsRecursive( sRecurseDir );
			}
			else
			{
				LARGE_INTEGER filesize;
				filesize.LowPart = ffd.nFileSizeLow;
				filesize.HighPart = ffd.nFileSizeHigh;
				if( filesize.QuadPart < 2048 || filesize.QuadPart > ROM_MAX_SIZE )
				{
					//	Ignore: wrong size
					RA_LOG( "Ignoring %s, wrong size\n", sFilename.c_str() );
				}
				else
				{
					//	Parse as ROM!
					RA_LOG( "%s looks good: parsing!\n", sFilename.c_str() );

					char sAbsFileDir[ 2048 ];
					sprintf_s( sAbsFileDir, 2048, "%s\\%s", sBaseDir.c_str(), sFilename.c_str() );

					HANDLE hROMReader = CreateFile( Widen( sAbsFileDir ).c_str(), GENERIC_READ, FILE_SHARE_READ,
													NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

					if( hROMReader != INVALID_HANDLE_VALUE )
					{
						BY_HANDLE_FILE_INFORMATION File_Inf;
						int nSize = 0;
						if( GetFileInformationByHandle( hROMReader, &File_Inf ) )
							nSize = ( File_Inf.nFileSizeHigh << 16 ) + File_Inf.nFileSizeLow;

						DWORD nBytes = 0;
						BOOL bResult = ReadFile( hROMReader, sROMRawData, nSize, &nBytes, NULL );
						const std::string sHashOut = RAGenerateMD5( sROMRawData, nSize );

						if( m_GameHashLibrary.find( sHashOut ) != m_GameHashLibrary.end() )
						{
							const unsigned int nGameID = m_GameHashLibrary[ std::string( sHashOut ) ];
							RA_LOG( "Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[ nGameID ].c_str() );

							const std::string& sGameTitle = m_GameTitlesLibrary[ nGameID ];
							AddTitle( sGameTitle, sAbsFileDir, nGameID );
							SetDlgItemText( m_hDialogBox, IDC_RA_GLIB_NAME, Widen( sGameTitle ).c_str() );
							InvalidateRect( m_hDialogBox, nullptr, TRUE );
						}

						CloseHandle( hROMReader );
					}
				}
			}
		}
		while( FindNextFile( hFind, &ffd ) != 0 );

		delete[]( sROMRawData );
		sROMRawData = NULL;

		FindClose( hFind );
	}

	SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, L"Scanning complete" );
}

void Dlg_GameLibrary::ReloadGameListData()
{
	ClearTitles();

	wchar_t sROMDir[ 1024 ];
	GetDlgItemText( m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024 );

	mtx.lock();
	while( FilesToScan.size() > 0 )
		FilesToScan.pop_front();
	mtx.unlock();

	bool bOK = ListFiles( Narrow( sROMDir ), "*.bin", FilesToScan );
	bOK |= ListFiles( Narrow( sROMDir ), "*.gen", FilesToScan );

	if( bOK )
	{
		std::thread scanner( &Dlg_GameLibrary::ThreadedScanProc );
		scanner.detach();
	}
}

void Dlg_GameLibrary::RefreshList()
{
	std::map<std::string, std::string>::iterator iter = Results.begin();
	while( iter != Results.end() )
	{
		const std::string& filepath = iter->first;
		const std::string& md5 = iter->second;

		if( VisibleResults.find( filepath ) == VisibleResults.end() )
		{
			//	Not yet added,
			if( m_GameHashLibrary.find( md5 ) != m_GameHashLibrary.end() )
			{
				//	Found in our hash library!
				const GameID nGameID = m_GameHashLibrary[ md5 ];
				RA_LOG( "Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[ nGameID ].c_str() );

				const std::string& sGameTitle = m_GameTitlesLibrary[ nGameID ];
				AddTitle( sGameTitle, filepath, nGameID );

				SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, Widen( sGameTitle ).c_str() );
				VisibleResults[ filepath ] = md5;	//	Copy to VisibleResults
			}
		}
		iter++;
	}
}

BOOL Dlg_GameLibrary::LaunchSelected()
{
	HWND hList = GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST );
	const int nSel = ListView_GetSelectionMark( hList );
	if( nSel != -1 )
	{
		wchar_t buffer[ 1024 ];
		ListView_GetItemText( hList, nSel, 1, buffer, 1024 );
		SetWindowText( GetDlgItem( m_hDialogBox, IDC_RA_GLIB_NAME ), buffer );

		ListView_GetItemText( hList, nSel, 3, buffer, 1024 );
		_RA_LoadROM( Narrow( buffer ).c_str() );

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void Dlg_GameLibrary::LoadAll()
{
	mtx.lock();
	FILE* pLoadIn = NULL;
	fopen_s( &pLoadIn, RA_MY_GAME_LIBRARY_FILENAME, "rb" );
	if( pLoadIn != NULL )
	{
		DWORD nCharsRead1 = 0;
		DWORD nCharsRead2 = 0;	
		do
		{
			nCharsRead1 = 0;
			nCharsRead2 = 0;	
			char fileBuf[2048];
			char md5Buf[64];
			ZeroMemory(fileBuf, 2048);
			ZeroMemory(md5Buf, 64);
			_ReadTil( '\n', fileBuf, 2048, &nCharsRead1, pLoadIn );

			if( nCharsRead1 > 0 )
			{
				_ReadTil( '\n', md5Buf, 64, &nCharsRead2, pLoadIn );
			}

			if( fileBuf[0] != '\0' && md5Buf[0] != '\0' && nCharsRead1 > 0 && nCharsRead2 > 0 )
			{
				fileBuf[nCharsRead1-1] = '\0';
				md5Buf[nCharsRead2-1] = '\0';

				//	Add
				std::string file = fileBuf;
				std::string md5 = md5Buf;

				Results[file] = md5;
			}

		}
		while( nCharsRead1 > 0 && nCharsRead2 > 0 );
		fclose( pLoadIn );
	}
	mtx.unlock();
}

void Dlg_GameLibrary::SaveAll()
{
	mtx.lock();
	FILE* pf = NULL;
	fopen_s( &pf, RA_MY_GAME_LIBRARY_FILENAME, "wb" );
	if( pf != NULL )
	{
		std::map<std::string,std::string>::iterator iter = Results.begin();
		while( iter != Results.end() )
		{
			const std::string& sFilepath = iter->first;
			const std::string& sMD5 = iter->second;
			
			fwrite( sFilepath.c_str(), sizeof(char), strlen(sFilepath.c_str()), pf );
			fwrite( "\n", sizeof(char), 1, pf );
			fwrite( sMD5.c_str(), sizeof(char), strlen(sMD5.c_str()), pf );
			fwrite( "\n", sizeof(char), 1, pf );

			iter++;
		}

		fclose( pf );
	}
	mtx.unlock();
}

//static 
INT_PTR CALLBACK Dlg_GameLibrary::s_GameLibraryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_GameLibrary.GameLibraryProc( hDlg, uMsg, wParam, lParam );
}

INT_PTR CALLBACK Dlg_GameLibrary::GameLibraryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
 	switch( uMsg )
 	{
	case WM_INITDIALOG:
	{
		HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );
		SetupColumns( hList );

		ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );

		SetDlgItemText( hDlg, IDC_RA_ROMDIR, Widen( g_sROMDirLocation ).c_str() );
		SetDlgItemText( hDlg, IDC_RA_GLIB_NAME, L"" );

		m_GameHashLibrary.clear();
		m_GameTitlesLibrary.clear();
		m_ProgressLibrary.clear();
		ParseGameHashLibraryFromFile( m_GameHashLibrary );
		ParseGameTitlesFromFile( m_GameTitlesLibrary );
		ParseMyProgressFromFile( m_ProgressLibrary );

		//int msBetweenRefresh = 1000;	//	auto?
		//SetTimer( hDlg, 1, msBetweenRefresh, (TIMERPROC)g_GameLibrary.s_GameLibraryProc );
		RefreshList();

		return FALSE;
	}

	case WM_TIMER:
		if( ( g_GameLibrary.GetHWND() != NULL ) && ( IsWindowVisible( g_GameLibrary.GetHWND() ) ) )
			RefreshList();
		//ReloadGameListData();
		return FALSE;
 
	case WM_NOTIFY:
 		switch( LOWORD( wParam ) )
		{
		case IDC_RA_LBX_GAMELIST:
			{
				switch( ( (LPNMHDR)lParam )->code )
				{
				case LVN_ITEMCHANGED:
				{
					//RA_LOG( "Item Changed\n" );
					HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );
					const int nSel = ListView_GetSelectionMark( hList );
					if( nSel != -1 )
					{
						wchar_t buffer[ 1024 ];
						ListView_GetItemText( hList, nSel, 1, buffer, 1024 );
						SetWindowText( GetDlgItem( hDlg, IDC_RA_GLIB_NAME ), buffer );
					}
				}
					break;

				case NM_CLICK:
					//RA_LOG( "Click\n" );
					break;

				case NM_DBLCLK:
					if( LaunchSelected() )
					{
						EndDialog( hDlg, TRUE );
						return TRUE;
					}
					break;

				default:
					break;
				}
			}
			return FALSE;

		default:
			RA_LOG( "%08x, %08x\n", wParam, lParam );
			return FALSE;
		}

 	case WM_COMMAND:
 		switch( LOWORD( wParam ) )
 		{
 		case IDOK:
			if( LaunchSelected() )
			{
				EndDialog( hDlg, TRUE );
				return TRUE;
			}
			else
			{
				return FALSE;
			}

		case IDC_RA_RESCAN:
			ReloadGameListData();
				
			mtx.lock();	//?
			SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, L"Scanning..." );
			mtx.unlock();
			return FALSE;

		case IDC_RA_PICKROMDIR:
			g_sROMDirLocation = GetFolderFromDialog();
			RA_LOG( "Selected Folder: %s\n", g_sROMDirLocation.c_str() );
			SetDlgItemText( hDlg, IDC_RA_ROMDIR, Widen( g_sROMDirLocation ).c_str() );
			return FALSE;

		case IDC_RA_LBX_GAMELIST:
			{
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );
				const int nSel = ListView_GetSelectionMark( hList );
				if( nSel != -1 )
				{
					wchar_t sGameTitle[ 1024 ];
					ListView_GetItemText( hList, nSel, 1, sGameTitle, 1024 );
					SetWindowText( GetDlgItem( hDlg, IDC_RA_GLIB_NAME ), sGameTitle );
				}
			}
			return FALSE;

		case IDC_RA_REFRESH:
			RefreshList();
			return FALSE;

		default:
			return FALSE;
		}

	case WM_PAINT:
		if( nNumParsed != Results.size() )
			nNumParsed = Results.size();
		return FALSE;

 	case WM_CLOSE:
 		EndDialog( hDlg, FALSE );
 		return TRUE;

	case WM_USER:
		return FALSE;

	default:
		return FALSE;
 	}
}

void Dlg_GameLibrary::KillThread()
{
	Dlg_GameLibrary::ThreadProcessingAllowed = false;
	while( Dlg_GameLibrary::ThreadProcessingActive )
	{
		RA_LOG("Waiting for background scanner..." );
		Sleep(200);
	}
}

////static
//void Dlg_GameLibrary::DoModalDialog( HINSTANCE hInst, HWND hParent )
//{
//	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), hParent, Dlg_GameLibrary::s_GameLibraryProc );
//}
