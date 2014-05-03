#include "RA_User.h"

#include <stdio.h>
#include <wincodec.h>
#include <direct.h>

#include "RA_Interface.h"

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"

#include "RA_Achievement.h"
#include "RA_ImageFactory.h"
#include "RA_PopupWindows.h"
#include "RA_AchievementOverlay.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Login.h"
#include "RA_httpthread.h"

LocalRAUser g_LocalUser;

void OnUserPicDownloaded( void* pvObj )
{
	RequestObject* pObj = (RequestObject*)pvObj;
	if( strncmp( pObj->m_sRequestPageName, "UserPic", 7 ) == 0 )
	{
		SetCurrentDirectory( g_sHomeDir );
		BOOL bIsLocal = ( pObj->m_nUserRef == 1 );

		if( bIsLocal )
			g_LocalUser.FlushBitmap();

		SetCurrentDirectory( g_sHomeDir );

		//	Write this image to local, signal overlay that new data has arrived.
		//pObj->m_sResponse contains the raw data for a .png
		char sTargetDir[1024];
		sprintf_s( sTargetDir, 1024, RA_DIR_DATA"%s", pObj->m_sRequestPageName+8 );

		FILE* pFile;
		if( fopen_s( &pFile, sTargetDir, "wb" ) == 0 )
		{
			fwrite( pObj->m_sResponse, 1, pObj->m_nBytesRead, pFile );
			fclose( pFile );
		}

		sTargetDir[ strlen(sTargetDir)-4 ] = '\0';	//	Remove the '.png'

		const char* sUsername = strchr( sTargetDir, '\\' );
		while( sUsername != NULL && strchr( sUsername+1, '\\' ) != NULL )
			sUsername = strchr( sUsername+1, '\\' );

		sUsername++;

		//const char* sUsername = sTargetDir+6;		//	Remove the '.\\cache\\'

		g_AchievementOverlay.OnHTTP_UserPic( sUsername );

		if( bIsLocal )
		{
			g_LocalUser.LoadUserImageFromFile();
			g_LocalUser.m_bFetchingUserImage = FALSE;
		}
		else
		{
			//	Find friend, set flag as 'fetched'
			for( size_t i = 0; i < g_LocalUser.NumFriends(); ++i )
			{
				RAUser* pUser = g_LocalUser.GetFriend( i );
				if( pUser != NULL &&
					strcmp( pUser->Username(), sUsername ) == 0 )
		 		{
		 			pUser->LoadUserImageFromFile();
		 			pUser->m_bFetchingUserImage = FALSE;
		 			break;
		 		}
			}
		}
	}
}


RAUser::RAUser()
{
	m_hUserImage = NULL;
	Clear();
}

void RAUser::Clear()
{
	FlushBitmap();
	m_sUsername[0] = '\0';
	m_sActivity[0] = '\0';
	m_nLatestScore = 0;
	m_bFetchingUserImage = FALSE;
}

void RAUser::FlushBitmap()
{
	if( m_hUserImage != NULL )
		DeleteObject( m_hUserImage );
	m_hUserImage = NULL;
}

void RAUser::SetUsername( const char* sUsername )
{
	strcpy_s( m_sUsername, 64, sUsername );
}

void RAUser::UpdateActivity( const char* sActivity )
{
	sprintf_s( m_sActivity, 256, " %s ", sActivity );
	//and?
}

void RAUser::RequestAndStoreUserImage()
{
	if( m_bFetchingUserImage )
		return;

	if( m_hUserImage == NULL )
	{
		char buffer[256];
		sprintf_s( buffer, 256, "UserPic/%s.png", m_sUsername );

		if( !HTTPRequestExists( buffer ) )
			CreateHTTPRequestThread( buffer, "", HTTPRequest_Get, (this==&g_LocalUser), OnUserPicDownloaded );
		
		m_bFetchingUserImage = TRUE;
	}
}

void RAUser::LoadUserImageFromFile()
{
	SetCurrentDirectory( g_sHomeDir );
	char sPath[1024];
	sprintf_s( sPath, 1024, RA_DIR_DATA "%s.png", m_sUsername );

	m_hUserImage = LoadLocalPNG( sPath, 64, 64 );
}

LocalRAUser::LocalRAUser() : RAUser()
{
}

void LocalRAUser::AttemptLogin()
{
	g_LocalUser.m_bIsLoggedIn = FALSE;

	if( g_LocalUser.m_sUsername != NULL && g_LocalUser.m_sUsername[0] != '\0' )
	{
		AttemptSilentLogin();
		//g_LocalUser.m_bIsLoggedIn = TRUE;
	}
	else
	{
		//	Push dialog to get them to login!
		DialogBox( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_LOGIN), g_RAMainWnd, LoginProc );
		_RA_SavePreferences();
	}
}

void LocalRAUser::AttemptSilentLogin()
{
	//	Don't login here: cause a login when requestlogin.php returns!
	//g_LocalUser.Login( bufferUser, bufferToken, true );

	char sRequest[512];
	sprintf_s( sRequest, 512, "u=%s&t=%s", m_sUsername, m_sToken );

	//	Attempt a sign in as well, in order to fetch score and latest messages etc
	CreateHTTPRequestThread( "requestlogin.php", sRequest, HTTPRequest_Post, 1, NULL );

	m_bStoreToken = TRUE;	//	Store it! We just fetched it!
}

//	Store user/pass, issue login commands
void LocalRAUser::Login( const char* sUser, const char* sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages )
{
	char sTitle[256];
	char sSubtitle[256];

	g_LocalUser.m_bIsLoggedIn = TRUE;
	sprintf_s( g_LocalUser.m_sUsername, 50, sUser );
	sprintf_s( g_LocalUser.m_sToken, 50, sToken );

	//	Used only for persistence: always store in memory (we need it!)
	g_LocalUser.m_bStoreToken = bRememberLogin;

 	for( size_t i = 0; i < m_Friends.size(); ++i )
 	{
		m_Friends[i].FlushBitmap();
 	}
 	m_Friends.clear();

	RequestAndStoreUserImage();
	RequestFriendList();
	
	g_LocalUser.m_nLatestScore = nPoints;

	sprintf_s( sTitle, 256,		" Welcome back %s (%d) ", g_LocalUser.m_sUsername, nPoints );
	sprintf_s( sSubtitle, 256,	" You have %d new %s. ", nMessages, (nMessages==1) ? "message" : "messages" );

	g_PopupWindows.AchievementPopups().AddMessage( sTitle, sSubtitle, MSG_LOGIN );

	g_AchievementsDialog.OnLoad_NewRom( g_pActiveAchievements->GameID() );
	g_AchievementEditorDialog.OnLoad_NewRom();
	g_AchievementOverlay.OnLoad_NewRom();

	RA_RebuildMenu();
	_RA_UpdateAppTitle();
}

void LocalRAUser::Logout()
{
	m_bIsLoggedIn = FALSE;
	FlushBitmap();
	Clear();
	RA_RebuildMenu();
	_RA_UpdateAppTitle( "" );
	MessageBox( NULL, "You are now logged out.", "Info", MB_OK );
}

void LocalRAUser::s_OnFriendListCB( void* pData )
{
	RequestObject* pObj = (RequestObject*)pData;

	g_LocalUser.OnFriendListCB( pObj );
}

void LocalRAUser::OnFriendListCB( RequestObject* pObj )
{
	if( pObj->m_bResponse )
	{
		if( strncmp( pObj->m_sResponse, "OK:", 3 ) == 0 )
		{
			if( pObj->m_sResponse[3] != '\0' )
			{
				char* cpIter = &(pObj->m_sResponse[3]);
				unsigned int nCharsRead = 0;

				do
				{
					char* pUser = _ReadStringTil( '&', cpIter, TRUE );
					char* pPoints = _ReadStringTil( '&', cpIter, TRUE );
					char* pActivity = _ReadStringTil( '\n', cpIter, TRUE );

					unsigned int nScore = strtol( pPoints, NULL, 10 );

					if( !pActivity || strlen( pActivity ) < 2 || strcmp( pActivity, "_" ) == 0 )
						pActivity = "Unknown!";

					if( pUser && pPoints && pActivity )
					{
						RAUser& NewFriend = AddFriend( pUser, nScore );	//TBD
						NewFriend.UpdateActivity( pActivity );	//TBD
					}

				} while ( (*cpIter) != '\0' );
			}
		}
		else
		{
			//	Issues?!
			//assert(0);
		}
	}
}

void LocalRAUser::RequestFriendList()
{
 	char sPost[512];
	sprintf_s( sPost, 512, "u=%s&t=%s", m_sUsername, m_sToken );
 	CreateHTTPRequestThread( "requestfriendlist.php", sPost, HTTPRequest_Post, 0, &s_OnFriendListCB );
}

RAUser& LocalRAUser::AddFriend( const char* sFriend, unsigned int nScore )
{
 	for( size_t i = 0; i < m_Friends.size(); ++i )
 	{
		if( strcmp( m_Friends[i].m_sUsername, sFriend ) == 0 )
 		{
 			//	Friend already added = just update score
			m_Friends[i].m_nLatestScore = nScore;
 			return m_Friends[i];
 		}
 	}
 
 	RAUser NewFriend;
	strcpy_s( NewFriend.m_sUsername, 64, sFriend );
	NewFriend.m_nLatestScore = nScore;
 	m_Friends.push_back( NewFriend );
 	return m_Friends.at( m_Friends.size()-1 );
}
 
void LocalRAUser::PostActivity( enum ActivityType nActivityType )
{
	char sPostString[512];
 
	switch( nActivityType )
	{
		case ActivityType_StartPlaying:
		{
			sprintf_s( sPostString, 512, "u=%s&t=%s&a=%d&m=%d",
				m_sUsername, 
				m_sToken, 
				(int)nActivityType,
				g_pActiveAchievements->m_nGameID );
			
			CreateHTTPRequestThread( "requestpostactivity.php", sPostString, HTTPRequest_Post, 0, NULL );
			break;
		}
		default:
		{
			//	unhandled
			assert(0);
			break;
		}
	}
}

void LocalRAUser::Clear()
{
	RAUser::Clear();
	g_LocalUser.m_sToken[0] = '\0';
	g_LocalUser.m_bIsLoggedIn = FALSE;
}

RAUser* LocalRAUser::GetFriend( unsigned int nOffs )
{
	if( nOffs < m_Friends.size() ) 
		return &m_Friends[nOffs];
	
	return NULL;
}


API bool _RA_UserLoggedIn()
{
	return (g_LocalUser.m_bIsLoggedIn == TRUE);
}
