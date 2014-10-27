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

//static 
LocalRAUser RAUsers::LocalUser;
std::map<std::string, RAUser*> RAUsers::UserDatabase;

//static 
BOOL RAUsers::DatabaseContainsUser( const std::string& sUser )
{
	return( UserDatabase.find( sUser ) != UserDatabase.end() );
}

//static
void RAUsers::ProcessSuccessfulLogin( const std::string& sUser, const std::string& sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages )
{
	RAUsers::LocalUser = LocalRAUser( sUser );
}

void OnUserPicDownloaded( void* pReqObj )
{
	RequestObject* pObj = static_cast<RequestObject*>( pReqObj );
	if( pObj->GetRequestType() == RequestUserPic )
	{
		DataStream& Data = pObj->GetResponse();

		const std::string& sTargetURL = pObj->GetPageURL();
		const std::string sURLMinusExt = sTargetURL.substr( sTargetURL.length() - 3 );
		const std::string sUser = sURLMinusExt.substr( sURLMinusExt.rfind( '/' ) + 1 );
		if( !RAUsers::DatabaseContainsUser( sUser ) )
			RAUsers::UserDatabase[ sUser ] = new RAUser( sUser );
		
		RAUser* pUser = RAUsers::UserDatabase[ sUser ];
		pUser->FlushBitmap();

		SetCurrentDirectory( g_sHomeDir );

		//	Write this image to local, signal overlay that new data has arrived.
		const std::string sTargetFile = RA_DIR_DATA + sUser + ".png";
		_WriteBufferToFile( sTargetFile, Data );

		g_AchievementOverlay.OnHTTP_UserPic( sUser.c_str() );
		
		pUser->LoadUserImageFromFile();
	}
}

RAUser::RAUser( const std::string& sUsername ) :
	m_sUsername( sUsername ),
	m_nLatestScore( 0 ),
	m_hUserImage( NULL ),
	m_bFetchingUserImage( false )
{
	//	Register
	ASSERT( g_UserDatabase.find( sUsername ) == g_UserDatabase.end() );
	RAUsers::UserDatabase[sUsername] = this;
}

RAUser::~RAUser()
{
	FlushBitmap();
}

void RAUser::FlushBitmap()
{
	if( m_hUserImage != NULL )
		DeleteObject( m_hUserImage );
	m_hUserImage = NULL;
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
			CreateHTTPRequestThread( buffer, "", HTTPRequest_Get, (this==&g_LocalUser) );
		
		m_bFetchingUserImage = TRUE;
	}
}

void RAUser::LoadUserImageFromFile()
{
	SetCurrentDirectory( g_sHomeDir );
	char sPath[1024];
	sprintf_s( sPath, 1024, RA_DIR_DATA "%s.png", m_sUsername );

	m_hUserImage = LoadLocalPNG( sPath, 64, 64 );
	m_bFetchingUserImage = false;
}

LocalRAUser::LocalRAUser( const std::string& sUser ) : 
	RAUser( sUser ),
	m_bIsLoggedIn( FALSE ),
	m_bStoreToken( FALSE )

{
}

//virtual 
LocalRAUser::~LocalRAUser()
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
	CreateHTTPRequestThread( "requestlogin.php", sRequest, HTTPRequest_Post, 1 );

	m_bStoreToken = TRUE;	//	Store it! We just fetched it!
}

//	Store user/pass, issue login commands
void LocalRAUser::Login( const std::string& sUser, const std::string& sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages )
{
	m_bIsLoggedIn = TRUE;
	sprintf_s( g_LocalUser.m_sUsername, 50, sUser );
	SetToken( sToken );

	//	Used only for persistence: always store in memory (we need it!)
	m_bStoreToken = bRememberLogin;
 	m_Friends.clear();

	RequestAndStoreUserImage();
	RequestFriendList();
	
	g_LocalUser.m_nLatestScore = nPoints;
	
	char sTitle[256];
	char sSubtitle[256];
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
						RAUser* pNewFriend = AddFriend( pUser, nScore );	//TBD
						pNewFriend->UpdateActivity( pActivity );	//TBD
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
 	//CreateHTTPRequestThread( "requestfriendlist.php", sPost, HTTPRequest_Post, 0, &s_OnFriendListCB );
 	CreateHTTPRequestThread( "requestfriendlist.php", sPost, HTTPRequest_Post, 0 );
}

RAUser* LocalRAUser::AddFriend( const std::string& sFriend, unsigned int nScore )
{
	if( !RAUsers::DatabaseContainsUser( sFriend ) )
		RAUsers::UserDatabase[ sFriend ] = new RAUser( sFriend );

 	for( size_t i = 0; i < m_Friends.size(); ++i )
 	{
		if( strcmp( m_Friends[i].m_sUsername, sFriend ) == 0 )
 		{
 			//	Friend already added = just update score
			m_Friends[i].m_nLatestScore = nScore;
 			return &m_Friends[i];
 		}
 	}
 
 	RAUser NewFriend;
	strcpy_s( NewFriend.m_sUsername, 64, sFriend );
	NewFriend.m_nLatestScore = nScore;
 	m_Friends.push_back( NewFriend );
 	//return m_Friends.at( m_Friends.size()-1 );
 	return &m_Friends.back();
}
 
void LocalRAUser::PostActivity( enum ActivityType nActivityType )
{
	char sPostString[512];
 
	switch( nActivityType )
	{
		case ActivityType_StartPlaying:
		{
			PostArgs args;
			args['u'] = Username();
			args['t'] = Token();
			args['a'] = std::to_string( nActivityType );
			args['m'] = std::to_string( g_pActiveAchievements->m_nGameID );

			RAWeb::CreateThreadedHTTPRequest( RequestPostActivity, args );
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

RAUser* LocalRAUser::GetFriendByIter( size_t nOffs )
{
	if( nOffs < m_Friends.size() ) 
		return &m_Friends.at( nOffs );
	
	return NULL;
}

RAUser* LocalRAUser::FindFriend( const std::string& sName )
{
	std::vector<RAUser>::iterator iter = m_Friends.begin();
	while( iter != m_Friends.end() )
	{
		if( sName.compare( (*iter).Username() ) == 0 )
			return &(*iter);
	}
	return NULL;
}

API bool _RA_UserLoggedIn()
{
	return (g_LocalUser.m_bIsLoggedIn == TRUE);
}
