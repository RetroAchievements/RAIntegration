#pragma once

#include <Windows.h>
#include <vector>
#include <map>
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
	RAUser( const std::string& sUsername );
	virtual ~RAUser();

public:
	void FlushBitmap();
	void LoadUserImageFromFile();
	void RequestAndStoreUserImage();

	unsigned int GetScore() const					{ return m_nLatestScore; }
	void SetScore( unsigned int nScore )			{ m_nLatestScore = nScore; }
	
	const std::string& Username() const				{ return m_sUsername; }
	
	void UpdateActivity( const std::string& sAct )	{ m_sActivity = sAct; }
	const std::string& Activity() const				{ return m_sActivity; }

	HBITMAP GetUserImage() const					{ return m_hUserImage; }
	void InstallUserImage( HBITMAP hImg )			{ m_hUserImage = hImg; }

	BOOL IsFetchingUserImage() const				{ return m_bFetchingUserImage; }
	
	static void s_OnUserPicCB( void* pvObj );
	void OnUserPicCB( RequestObject* pObj );

private:
	const std::string	m_sUsername;
	std::string			m_sActivity;
	unsigned int		m_nLatestScore;

	HBITMAP				m_hUserImage;
	BOOL				m_bFetchingUserImage;
};

class LocalRAUser : public RAUser
{
public:
	LocalRAUser( const std::string& sUser );
	virtual ~LocalRAUser();

public:
	void AttemptLogin();
	void AttemptSilentLogin();
	void Login( const std::string& sUser, const std::string& sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages );
	void Logout();

	void RequestFriendList();
	static void s_OnFriendListCB( void* pvObj );
	void OnFriendListCB( RequestObject* pObj );
	
	RAUser* AddFriend( const std::string& sFriend, unsigned int nScore );
	RAUser* FindFriend( const std::string& sName );

	RAUser* GetFriendByIter( size_t nOffs )				{ return nOffs < m_aFriends.size() ? m_aFriends[ nOffs ] : NULL; }
	const size_t NumFriends() const						{ return m_aFriends.size(); }

	const std::string& Token() const					{ return m_sToken; }
	
	BOOL IsLoggedIn() const		{ return m_bIsLoggedIn; }

	void PostActivity( enum ActivityType nActivityType );
	
	void Clear();

private:
	std::string				m_sToken;		//	AppToken Issued by server
	BOOL					m_bIsLoggedIn;
	BOOL					m_bStoreToken;	//	Preference: Store the token/'password' for next time
	std::vector<RAUser*>	m_aFriends;
	std::vector<RAMessage>	m_aMessages;
};

class RAUsers
{
public:
	static LocalRAUser LocalUser;
	static std::map<std::string, RAUser*> UserDatabase;

	static BOOL DatabaseContainsUser( const std::string& sUser );
	static void ProcessSuccessfulLogin( const std::string& sUser, const std::string& sToken, BOOL bRememberLogin, unsigned int nPoints, unsigned int nMessages );
};



//	Exposed to DLL
extern "C"
{
	API extern bool _RA_UserLoggedIn();
}
