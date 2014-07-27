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
	const char* g_sColTitles[] = { "ID", "Game Title", "Completion", "File Path" };
	const int g_nColSizes[] = { 30, 230, 110, 170 };

	const size_t NUM_COLS = sizeof( g_sColTitles ) / sizeof( g_sColTitles[0] );

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
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    std::string spec;
    std::stack<std::string> directories;

    directories.push(path);

    while (!directories.empty()) {
        path = directories.top();
        spec = path + "\\" + mask;
        directories.pop();

        hFind = FindFirstFile(spec.c_str(), &ffd);
        if( hFind == INVALID_HANDLE_VALUE )
            return false;

        do {
			if( ( strcmp( ffd.cFileName, "." ) == 0 ) ||
				( strcmp( ffd.cFileName, ".." ) == 0 ) )
				continue;

			if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				directories.push( path + "\\" + ffd.cFileName );
			}
			else
			{
				rFileListOut.push_front( path + "\\" + ffd.cFileName );
			}

		} while (FindNextFile(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES)
		{
            FindClose(hFind);
            return false;
        }

        FindClose(hFind);
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
 :	m_hDialogBox( NULL )
{
}

Dlg_GameLibrary::~Dlg_GameLibrary()
{
}

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

void Dlg_GameLibrary::ClearTitles()
{
	nNumParsed = 0;

	m_vGameEntries.clear();

	ListView_DeleteAllItems( GetDlgItem( m_hDialogBox, IDC_RA_LBX_GAMELIST ) );
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

		FILE* pFile = NULL;
		if( fopen_s( &pFile, FilesToScan.front().c_str(), "rb" ) == 0 )
		{
			// obtain file size:
			fseek( pFile, 0, SEEK_END );
			DWORD nSize = ftell( pFile );
			rewind( pFile );

			//	No point.. ffs
			//unsigned char* fileBuf = (unsigned char*)malloc(sizeof(unsigned char)*nSize);
			
			unsigned char* fileBuf = (unsigned char*)malloc(6 * 1024 * 1024);
			memset( fileBuf, 0, 6 * 1024 * 1024);
			//memset(Rom_Data, 0, 6 * 1024 * 1024);

			if( fileBuf != NULL )
			{
				fread( fileBuf, 1, nSize, pFile );

				//std::string sResult = FilesToScan.front().c_str();
				//sResult.append( " -> \t" );
				//sResult.append( md5( buffer, nSize ) );
				
				char md5Buffer[33];
				md5_GenerateMD5Raw( fileBuf, 6 * 1024 * 1024, md5Buffer );

				Results[FilesToScan.front()] = md5Buffer;
				
				SendMessage( g_GameLibrary.GetHWND(), WM_TIMER, NULL, NULL );

				free( fileBuf );
			}

			fclose( pFile );
		}
		
		mtx.lock();
		FilesToScan.pop_front();
		mtx.unlock();
	}
	
	Dlg_GameLibrary::ThreadProcessingActive = false;
	ExitThread( 0 );
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

	SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, "Scanning complete" );
}

void Dlg_GameLibrary::ReloadGameListData()
{
	ClearTitles();

	char sROMDir[1024];
	GetDlgItemText( m_hDialogBox, IDC_RA_ROMDIR, sROMDir, 1024 );
	
	mtx.lock();
	while( FilesToScan.size() > 0 )
		FilesToScan.pop_front();
	mtx.unlock();

	if( ListFiles( sROMDir, "*", FilesToScan ) )
	{
		//ScanAndAddRomsRecursive( sROMDir );
		std::thread scanner( &Dlg_GameLibrary::ThreadedScanProc );
		scanner.detach();
	}
}

void Dlg_GameLibrary::RefreshList()
{
	//ClearTitles();

	//for( int i = 0; i > Results.size(); ++i )
	std::map<std::string,std::string>::iterator iter = Results.begin();
	while( iter != Results.end() )
	{
		const std::string& filepath = (*iter).first;
		const std::string& md5 = (*iter).second;

		if( VisibleResults.find( filepath ) == VisibleResults.end() )
		{
			if( m_GameHashLibrary.find( md5 ) != m_GameHashLibrary.end() )
			{
				const unsigned int nGameID = m_GameHashLibrary[ md5 ];
				RA_LOG( "Found one! Game ID %d (%s)", nGameID, m_GameTitlesLibrary[ nGameID ].c_str() );
															
				//std::map<unsigned int, std::string>::iterator iter = m_ProgressLibrary.begin();
				//while( iter != m_ProgressLibrary.end() )
				//{
				//	const unsigned int nGameID2 = (*iter).first;
					const std::string& sGameTitle = m_GameTitlesLibrary[ nGameID ];
					//const std::string& sGameProgress = m_ProgressLibrary[ nGameID ];
								
					AddTitle( sGameTitle, filepath, nGameID );
					

					//SetDlgItemText( m_hDialogBox, IDC_RA_GLIB_NAME, sGameTitle.c_str() );
					SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, sGameTitle.c_str() );


					//InvalidateRect( m_hDialogBox, NULL, true );

				//	iter++;
				//}
				VisibleResults[filepath] = md5;	//	Copy to VisibleResults
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
		char buffer[1024];
		ListView_GetItemText( hList, nSel, 1, buffer, 1024 );
		SetWindowText( GetDlgItem( m_hDialogBox, IDC_RA_GLIB_NAME ), buffer );

		ListView_GetItemText( hList, nSel, 3, buffer, 1024 );
		_RA_LoadROM( buffer );
		return TRUE;
	}

	return FALSE;
}

void Dlg_GameLibrary::LoadAll()
{
	mtx.lock();
	FILE* pLoadIn = NULL;
	fopen_s( &pLoadIn, RA_DIR_DATA "gamelibraryfound.txt", "r" );
	if( pLoadIn != NULL )
	{
		//while( nCharsRead > 2 )
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
	FILE* pSaveOut = NULL;
	fopen_s( &pSaveOut, RA_DIR_DATA "gamelibraryfound.txt", "w" );
	if( pSaveOut != NULL )
	{
		std::map<std::string,std::string>::iterator iter = Results.begin();
		while( iter != Results.end() )
		{
			const std::string& sFilepath = iter->first;
			const std::string& sMD5 = iter->second;
			
			fwrite( sFilepath.c_str(), sizeof(char), strlen(sFilepath.c_str()), pSaveOut );
			fwrite( "\n", sizeof(char), 1, pSaveOut );
			fwrite( sMD5.c_str(), sizeof(char), strlen(sMD5.c_str()), pSaveOut );
			fwrite( "\n", sizeof(char), 1, pSaveOut );

			iter++;
		}

		fclose( pSaveOut );
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
		//	m_hDialogBox = hDlg;//	?

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
		
		int msBetweenRefresh = 1000;	//	auto?

		//SetTimer( hDlg, 1, msBetweenRefresh, (TIMERPROC)g_GameLibrary.s_GameLibraryProc );
		RefreshList();

		//ReloadGameListData();

		return FALSE;
	}
	break;

	case WM_TIMER:
		if( ( g_GameLibrary.GetHWND() != NULL ) && ( IsWindowVisible( g_GameLibrary.GetHWND() ) ) )
			RefreshList();
		//ReloadGameListData();
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
				
				mtx.lock();	//?
				SetDlgItemText( m_hDialogBox, IDC_RA_SCANNERFOUNDINFO, "Scanning..." );
				mtx.unlock();
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
						//ReloadGameListData();
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

		case IDC_RA_REFRESH:
			{
				RefreshList();
			}
			break;

 		//case IDCANCEL:
 		//	EndDialog( hDlg, TRUE );
 		//	return TRUE;
 		//	break;
		}
 		break;

	case WM_PAINT:
		if( nNumParsed != Results.size() )
		{
			nNumParsed = Results.size();
			//	Repopulate

			//RefreshList();
		}
 		return DefWindowProc( hDlg, uMsg, wParam, lParam );

 	case WM_CLOSE:
 		// 		if (Full_Screen)
 		// 		{
 		// 			while (ShowCursor(true) < 0);
 		// 			while (ShowCursor(false) >= 0);
 		// 		}
 
 		EndDialog( hDlg, FALSE );
 		return TRUE;
 		break;

	case WM_USER:

		break;
 	}

	return FALSE;
	//return DefWindowProc( hDlg, uMsg, wParam, lParam );
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