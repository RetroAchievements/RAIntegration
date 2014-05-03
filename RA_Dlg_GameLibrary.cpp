#include "RA_Dlg_GameLibrary.h"

#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include <string>
#include <sstream>
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
	const char* g_sColTitles[] = { "ID", "Game Title", "Completion", "File Path" };
	const int g_nColSizes[] = { 30, 230, 110, 170 };

	const size_t NUM_COLS = sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

	const bool bCancelScan = false;
}

//static 
HWND Dlg_GameLibrary::m_hDialogBox = NULL;
std::vector<GameEntry> Dlg_GameLibrary::m_vGameEntries;
std::map<std::string, unsigned int> Dlg_GameLibrary::m_GameHashLibrary;
std::map<unsigned int, std::string> Dlg_GameLibrary::m_GameTitlesLibrary;
std::map<unsigned int, std::string> Dlg_GameLibrary::m_ProgressLibrary;

void ParseGameHashLibraryFromFile( std::map<std::string, unsigned int>& GameHashLibraryOut )
{
	FILE* pGameHashLibrary = NULL;
	fopen_s( &pGameHashLibrary, RA_DIR_DATA "gamehashlibrary.txt", "r" );
	if( pGameHashLibrary != NULL )
	{
		char buffer[256];

		DWORD nCharsRead;
		while( _ReadTil( '\n', buffer, 256, &nCharsRead, pGameHashLibrary ) )
		{
			if( nCharsRead > 32 )
			{
				std::string sHash = buffer;
				sHash = sHash.substr( 0, 32 );
				const unsigned int nGameID = atoi( buffer+33 );

				GameHashLibraryOut[ sHash ] = nGameID;
			}
		}

		fclose( pGameHashLibrary );
	}
}

void ParseGameTitlesFromFile( std::map<unsigned int, std::string>& GameTitlesListOut )
{
	FILE* pGameTitlesFile = NULL;
	fopen_s( &pGameTitlesFile, RA_DIR_DATA "gametitles.txt", "r" );
	if( pGameTitlesFile != NULL )
	{
		char buffer[256];

		DWORD nCharsRead;
		while( _ReadTil( '\n', buffer, 256, &nCharsRead, pGameTitlesFile ) )
		{
			if( nCharsRead > 2 )
			{
				const unsigned int nGameID = atoi( buffer );

				buffer[nCharsRead-1] = '\0';
				char* pTitle = NULL;
				strtok_s( buffer, ":", &pTitle );

				GameTitlesListOut[ nGameID ] = pTitle;//	Implicit alloc?
			}
		}

		fclose( pGameTitlesFile );
	}
}

void ParseMyProgressFromFile( std::map<unsigned int, std::string>& GameProgressOut )
{
	FILE* pFile = NULL;
	fopen_s( &pFile, RA_DIR_DATA "myprogress.txt", "r" );
	if( pFile != NULL )
	{
		char buffer[256];

		DWORD nCharsRead;
		while( _ReadTil( '\n', buffer, 256, &nCharsRead, pFile ) )
		{
			//	ID:Possible:EarnedNormal:EarnedHardcore\n	<-each row

			if( nCharsRead > 2 )
			{
				char* pIter = &buffer[0];
				const unsigned int nGameID = atoi( strtok_s( pIter, ":", &pIter ) );
				const unsigned int nNumPossible = atoi( strtok_s( pIter, ":", &pIter ) );
				const unsigned int nEarned = atoi( strtok_s( pIter, ":", &pIter ) );
				const unsigned int nEarnedHC = atoi( strtok_s( pIter, ":", &pIter ) );

				std::stringstream sProgress;
				sProgress << nEarned;
				if( nEarnedHC > 0 )
				{
					sProgress << "(";
					sProgress << nEarnedHC;
					sProgress << ")";
				}
				sProgress << " / ";
				sProgress << nNumPossible;

				if( nNumPossible > 0 )
				{
					const int nNumEarnedTotal = nEarned + nEarnedHC;
					char bufPct[256];
					sprintf_s( bufPct, 256, " (%1.1f%%)", ( (float)nNumEarnedTotal / (float)nNumPossible ) * 100.0f );	
					sProgress << bufPct;
				}

				GameProgressOut[ nGameID ] = sProgress.str();//	Implicit alloc?
			}
		}

		fclose( pFile );
	}
}

void Dlg_GameLibrary::SetupColumns( HWND hList )
{
	//	Remove all columns,
	while( ListView_DeleteColumn( hList, 0 ) ) {}

	//	Remove all data.
	ListView_DeleteAllItems( hList );

	char buffer[256];
	
	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	for( size_t i = 0; i < NUM_COLS; ++i )
	{
		col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
		col.cx = g_nColSizes[i];
		col.cchTextMax = 255;
		sprintf_s( buffer, 256, g_sColTitles[i] );
		col.pszText = buffer;
		col.iSubItem = i;

		col.fmt = LVCFMT_LEFT|LVCFMT_FIXED_WIDTH;
		if( i == NUM_COLS-1 )
			col.fmt |= LVCFMT_FILL; 

		ListView_InsertColumn( hList, i, (LPARAM)&col );
	}

	//ZeroMemory( &m_lbxData, sizeof(m_lbxData) );

	//BOOL bSuccess = ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT );
	//bSuccess = ListView_EnableGroupView( hList, FALSE );
}

//static
void Dlg_GameLibrary::AddTitle( const std::string& sTitle, const std::string& sFilename, unsigned int nGameID )
{
	LV_ITEM item;
	ZeroMemory( &item, sizeof( item ) );

	item.mask = LVIF_TEXT;
	item.cchTextMax = 256;
	item.iItem = m_vGameEntries.size();

	//const char* g_sColTitles[] = { "ID", "Game Title", "Completion", "File Path" };

	HWND hList = GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST );

	//	id:
	item.iSubItem = 0;
	char buffer[16];
	sprintf_s( buffer, 16, "%d", nGameID );
	item.pszText = buffer;	//	dangerous!
	item.iItem = ListView_InsertItem( hList, &item );

	//	title:
	item.iSubItem = 1;
	//item.pszText = ;
	ListView_SetItemText( hList, item.iItem, 1, (LPSTR)sTitle.c_str() );
	//ListView_SetItem( hList, &item );

	//	completion:
	item.iSubItem = 2;
	//item.pszText = (LPSTR)m_ProgressLibrary[ nGameID ].c_str();
	//ListView_SetItem( hList, &item );
	ListView_SetItemText( hList, item.iItem, 2, (LPSTR)m_ProgressLibrary[ nGameID ].c_str() );

	//	file path:
	item.iSubItem = 3;
	//item.pszText = (LPSTR)sFilename.c_str();
	//ListView_SetItem( hList, &item );
	ListView_SetItemText( hList, item.iItem, 3, (LPSTR)sFilename.c_str() );

	const GameEntry nNewGame( sTitle, sFilename, nGameID );
	m_vGameEntries.push_back( nNewGame );
}

//static
void Dlg_GameLibrary::ClearTitles()
{
	m_vGameEntries.clear();

	ListView_DeleteAllItems( GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST ) );
}

void Dlg_GameLibrary::ScanAndAddRomsRecursive( std::string sBaseDir )
{
	char sSearchDir[2048];
	sprintf_s( sSearchDir, 2048, "%s\\*.*", sBaseDir.c_str() );

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile( sSearchDir, &ffd );
	if( hFind != INVALID_HANDLE_VALUE )
	{
		unsigned int nMaxROMSize = 6*1024*1024;
		unsigned char* sROMRawData = new unsigned char[nMaxROMSize];

		do
		{
			if( KEYDOWN( VK_ESCAPE ) )
				break;

			memset( sROMRawData, 0, nMaxROMSize );
			if( strcmp( ffd.cFileName, "." ) == 0 || strcmp( ffd.cFileName, ".." ) == 0 )
			{
				//	Ignore 'this'
			}
			else if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				RA_LOG( "Directory found: %s\n", ffd.cFileName );
				
				std::string sRecurseDir = sBaseDir + "\\" + ffd.cFileName;
				//sprintf_s( sRecurseDir, 2048, "%s\\%s", sBaseDir.c_str(), ffd.cFileName );
				ScanAndAddRomsRecursive( sRecurseDir );
			}
			else
			{
				LARGE_INTEGER filesize;
				filesize.LowPart = ffd.nFileSizeLow;
				filesize.HighPart = ffd.nFileSizeHigh;
				if( filesize.QuadPart < 2048 || filesize.QuadPart > nMaxROMSize )
				{
					//	Ignore: wrong size
					RA_LOG( "Ignoring %s, wrong size\n", ffd.cFileName );
				}
				else
				{
					//	Parse as ROM!
					
					RA_LOG( "%s looks good: parsing!\n", ffd.cFileName );
					
					char sAbsFileDir[2048];
					sprintf_s( sAbsFileDir, 2048, "%s\\%s", sBaseDir.c_str(), ffd.cFileName );

					HANDLE hROMReader = CreateFile( sAbsFileDir, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

					if( hROMReader != INVALID_HANDLE_VALUE )
					{
						BY_HANDLE_FILE_INFORMATION File_Inf;
						int nSize = 0;
						if( GetFileInformationByHandle( hROMReader, &File_Inf ) )
							nSize = ( File_Inf.nFileSizeHigh << 16 ) + File_Inf.nFileSizeLow;
						
						DWORD nBytes = 0;
						BOOL bResult = ReadFile( hROMReader, sROMRawData, nSize, &nBytes, NULL );
						char sHashOut[33];
						//md5_GenerateMD5Raw( sROMRawData, nBytes, sHashOut );
						md5_GenerateMD5Raw( sROMRawData, nMaxROMSize, sHashOut );

						if( m_GameHashLibrary.find( std::string( sHashOut ) ) != m_GameHashLibrary.end() )
						{
							const unsigned int nGameID = m_GameHashLibrary[ std::string( sHashOut ) ];
							RA_LOG( "Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[ nGameID ].c_str() );
															
							//std::map<unsigned int, std::string>::iterator iter = m_ProgressLibrary.begin();
							//while( iter != m_ProgressLibrary.end() )
							//{
							//	const unsigned int nGameID2 = (*iter).first;
								const std::string& sGameTitle = m_GameTitlesLibrary[ nGameID ];
								//const std::string& sGameProgress = m_ProgressLibrary[ nGameID ];
								
								AddTitle( sGameTitle, sAbsFileDir, nGameID );
								
								SetDlgItemText( m_hDialogBox, IDC_RA_GLIB_NAME, sGameTitle.c_str() );
								InvalidateRect( m_hDialogBox, NULL, true );

							//	iter++;
							//}

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
}

void Dlg_GameLibrary::ReloadGameListData()
{
	ClearTitles();

	char sROMDir[1024];
	GetDlgItemText( m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024 );

	ScanAndAddRomsRecursive( sROMDir );
}

BOOL Dlg_GameLibrary::LaunchSelected()
{
	HWND hList = GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST );
	const int nSel = ListView_GetSelectionMark( hList );
	if( nSel != -1 )
	{
		char buffer[1024];
		ListView_GetItemText( hList, nSel, 1, buffer, 1024 );
		SetWindowText( GetDlgItem( m_hDialogBox, IDC_RA_GLIB_NAME ), buffer );

		ListView_GetItemText( hList, nSel, 3, buffer, 1024 );
		_RA_LoadROM( buffer );
		return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK Dlg_GameLibrary::s_GameLibraryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
 	switch( uMsg )
 	{
 	case WM_INITDIALOG:
	{
		m_hDialogBox = hDlg;

		HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );	
		SetupColumns( hList );

		ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );
		//ListView_SetExtendedListViewStyle( hList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT );

		//#define IDC_RA_RESCAN                   1591
		//#define IDC_RA_LBX_GAMELIST             1592
		//#define IDC_RA_ROMDIR                   1593
		//#define IDC_RA_GLIB_NAME                1594

		SetDlgItemText( hDlg, IDC_RA_ROMDIR, g_sROMDirLocation );
		SetDlgItemText( hDlg, IDC_RA_GLIB_NAME, "" );

		m_GameHashLibrary.clear();
		m_GameTitlesLibrary.clear();
		m_ProgressLibrary.clear();
		ParseGameHashLibraryFromFile( m_GameHashLibrary );
		ParseGameTitlesFromFile( m_GameTitlesLibrary );
		ParseMyProgressFromFile( m_ProgressLibrary );

		//ReloadGameListData();

		return FALSE;
	}
	break;
 
	case WM_NOTIFY:
 		switch( LOWORD(wParam) )
		{
		case IDC_RA_LBX_GAMELIST:
			{
				switch( ((LPNMHDR)lParam)->code )
				{
					case LVN_ITEMCHANGED:
					{
						//RA_LOG( "Item Changed\n" );
						HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );
						const int nSel = ListView_GetSelectionMark( hList );
						if( nSel != -1 )
						{
							char buffer[1024];
							ListView_GetItemText( hList, nSel, 1, buffer, 1024 );
							SetWindowText( GetDlgItem( hDlg, IDC_RA_GLIB_NAME ), buffer );
						}

					}
						break;
					case NM_CLICK:
						//RA_LOG( "Click\n" );
						break;
					case NM_DBLCLK:
					{
						//RA_LOG( "Double click\n" );	
						if( LaunchSelected() )
						{
							EndDialog( hDlg, TRUE );
							return TRUE;
						}
					}
						break;
					default:
						break;
				}
			}
			break;
		default:
			RA_LOG( "%08x, %08x\n", wParam, lParam );
			break;
		}

 	case WM_COMMAND:
 		switch( LOWORD(wParam) )
 		{
 		case IDOK:
 			{
				if( LaunchSelected() )
				{
					EndDialog( hDlg, TRUE );
					return TRUE;
				}
 			}
			break;

		case IDC_RA_RESCAN:
			{
				ReloadGameListData();
			}
			break;

		case IDC_RA_PICKROMDIR:
			{
				BROWSEINFO bi = { 0 };
				bi.lpszTitle = "Select your ROM directory";
				LPITEMIDLIST pidl = SHBrowseForFolder( &bi );
				if( pidl != 0 )
				{
					// get the name of the folder
					if( SHGetPathFromIDList( pidl, g_sROMDirLocation ) )
					{
						RA_LOG( "Selected Folder: %s\n", g_sROMDirLocation );
						SetDlgItemText( hDlg, IDC_RA_ROMDIR, g_sROMDirLocation );
						ReloadGameListData();
					}
					CoTaskMemFree( pidl );
				}
			}
			break;

		case IDC_RA_LBX_GAMELIST:
			{
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_GAMELIST );
				const int nSel = ListView_GetSelectionMark( hList );
				if( nSel != -1 )
				{
					char sGameTitle[1024];
					ListView_GetItemText( hList, nSel, 1, sGameTitle, 1024 );
					//const int nSel = ListBox_GetCurSel( hList );
					//char buffer[256];
					//ListView_GetText( hList, nSel, buffer );
					SetWindowText( GetDlgItem( hDlg, IDC_RA_GLIB_NAME ), sGameTitle );
				}
			}
			break;

 		//case IDCANCEL:
 		//	EndDialog( hDlg, TRUE );
 		//	return TRUE;
 		//	break;
		}
 		break;
 	case WM_CLOSE:
 		// 		if (Full_Screen)
 		// 		{
 		// 			while (ShowCursor(true) < 0);
 		// 			while (ShowCursor(false) >= 0);
 		// 		}
 
 		EndDialog( hDlg, FALSE );
 		return TRUE;
 		break;
 	}

	return FALSE;
	//return DefWindowProc( hDlg, uMsg, wParam, lParam );
}

//static
void Dlg_GameLibrary::DoModalDialog( HINSTANCE hInst, HWND hParent )
{
	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMELIBRARY), hParent, Dlg_GameLibrary::s_GameLibraryProc );
}