#pragma once

#include "RA_Defs.h"

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
	ActivityTypeUnknown = 0,	//	DO NOT USE
	PlayerEarnedAchievement,	//	DO NOT USE: handled at PHP level
	PlayerLoggedIn,				//	DO NOT USE: handled at PHP level
	PlayerStartedPlaying,
	UserUploadedAchievement,
	UserModifiedAchievement,
};


class RAUser
{
public:
	RAUser( const std::string& sUsername );
	virtual ~RAUser();

public:
	void FlushBitmap();
	void LoadOrFetchUserImage();

	unsigned int GetScore() const					{ return m_nScore; }
	void SetScore( unsigned int nScore )			{ m_nScore = nScore; }
	
	void SetUsername( const std::string& sUser )	{ m_sUsername = sUser; }
	const std::string& Username() const				{ return m_sUsername; }
	
	void UpdateActivity( const std::string& sAct )	{ m_sActivity = sAct; }
	const std::string& Activity() const				{ return m_sActivity; }

	HBITMAP GetUserImage() const					{ return m_hUserImage; }
	void InstallUserImage( HBITMAP hImg )			{ m_hUserImage = hImg; }

	BOOL IsFetchingUserImage() const				{ return m_bFetchingUserImage; }
	
private:
	/*const*/std::string	m_sUsername;
	std::string				m_sActivity;
	unsigned int			m_nScore;

	HBITMAP					m_hUserImage;
	BOOL					m_bFetchingUserImage;
};

class LocalRAUser : public RAUser
{
public:
	LocalRAUser( const std::string& sUser );

public:
	void AttemptLogin( bool bBlocking );

	void AttemptSilentLogin();
	void HandleSilentLoginResponse( Document& doc );

	void ProcessSuccessfulLogin( const std::string& sUser, const std::string& sToken, unsigned int nPoints, unsigned int nMessages, BOOL bRememberLogin );
	void Logout();

	void RequestFriendList();
	void OnFriendListResponse( const Document& doc );
	
	RAUser* AddFriend( const std::string& sFriend, unsigned int nScore );
	RAUser* FindFriend( const std::string& sName );

	RAUser* GetFriendByIter( size_t nOffs )				{ return nOffs < m_aFriends.size() ? m_aFriends[ nOffs ] : NULL; }
	const size_t NumFriends() const						{ return m_aFriends.size(); }

	void SetStoreToken( BOOL bStoreToken )				{ bStoreToken = bStoreToken; }

	void SetToken( const std::string& sToken )			{ m_sToken = sToken; }
	const std::string& Token() const					{ return m_sToken; }
	
	BOOL IsLoggedIn() const		{ return m_bIsLoggedIn; }

	void PostActivity( ActivityType nActivityType );
	
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
	static LocalRAUser& LocalUser()												{ return ms_LocalUser; }
	static BOOL DatabaseContainsUser( const std::string& sUser );
	static void OnUserPicDownloaded( const RequestObject& obj );

	static void RegisterUser( const std::string& sUsername, RAUser* pUser )		{ UserDatabase[ sUsername ] = pUser; }

	static RAUser* GetUser( const std::string& sUser );

private:
	static LocalRAUser ms_LocalUser;
	static std::map<std::string, RAUser*> UserDatabase;
};
