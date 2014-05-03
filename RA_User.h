#ifndef _RAUSER_H_
#define _RAUSER_H_

#include <Windows.h>
#include <vector>
#include "RA_Core.h"

//////////////////////////////////////////////////////////////////////////
//	RAUser

typedef struct
{
	char m_Sender[256];
	char m_sDateSent[256];
	char m_sTitle[256];
	char m_sMessage[256];
	BOOL m_bRead;
} RAMessage;

class RequestObject;

enum ActivityType
{
	ActivityType_Unknown = 0,	//	DO NOT USE
	ActivityType_Achievement,	//	DO NOT USE: handled at PHP level
	ActivityType_Login,			//	DO NOT USE: handled at PHP level
	ActivityType_StartPlaying,
	ActivityType_UploadAch,
	ActivityType_ModifyAch,
};


class RAUser
{
public:
	RAUser();

public:
	void FlushBitmap();
	void SetUsername( const char* sUsername );
	void UpdateActivity( const char* sActivity );
	void LoadUserImageFromFile();
	void RequestAndStoreUserImage();
	void Clear();

	const char* Username() const		{ return m_sUsername; }
	const char* Activity() const		{ return m_sActivity; }
	unsigned int Score() const			{ return m_nLatestScore; }
	HBITMAP Image() const				{ return m_hUserImage; }
	BOOL IsFetchingUserImage() const	{ return m_bFetchingUserImage; }
	
	static void s_OnUserPicCB( void* pvObj );
	void OnUserPicCB( RequestObject* pObj );

public:
	char		 m_sUsername[64];
	char		 m_sActivity[256];
	unsigned int m_nLatestScore;
	HBITMAP		 m_hUserImage;
	BOOL		 m_bFetchingUserImage;
};



class LocalRAUser : public RAUser
{
public:
	LocalRAUser();

public:
	void AttemptLogin();
	void AttemptSilentLogin();
	void Login( const char* sUser, const char* sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages );
	void Logout();

	void RequestFriendList();
	static void s_OnFriendListCB( void* pvObj );
	void OnFriendListCB( RequestObject* pObj );
	
	RAUser& AddFriend( const char* sFriend, unsigned int nScore );
	RAUser* GetFriend( unsigned int nOffs );
	const size_t NumFriends() const		{ return m_Friends.size(); }
	
	void PostActivity( enum ActivityType nActivityType );

	void Clear();

public:
	char					m_sToken[64];	//	AppToken Issued by server
	BOOL					m_bIsLoggedIn;
	BOOL					m_bStoreToken;	//	Store the token/'password' for next time
	std::vector<RAUser>		m_Friends;
	std::vector<RAMessage>	m_Messages;
};
extern LocalRAUser g_LocalUser;


//	Exposed to DLL
extern "C"
{

API extern bool _RA_UserLoggedIn();

}

#endif //_RAUSER_H_