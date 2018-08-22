#include "RA_User.h"

#include "RA_Achievement.h"
#include "RA_AchievementOverlay.h"
#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Login.h"
#include "RA_GameData.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_Interface.h"
#include "RA_PopupWindows.h"
#include "RA_Resource.h"

//static 
LocalRAUser RAUsers::ms_LocalUser("");
std::map<std::string, RAUser*> RAUsers::UserDatabase;

//static 
BOOL RAUsers::DatabaseContainsUser(const std::string& sUser)
{
    return(UserDatabase.find(sUser) != UserDatabase.end());
}

//static 
RAUser* RAUsers::GetUser(const std::string& sUser)
{
    if (DatabaseContainsUser(sUser) == FALSE)
        UserDatabase[sUser] = new RAUser(sUser);

    return UserDatabase[sUser];
}


RAUser::RAUser(const std::string& sUsername) :
    m_sUsername(sUsername),
    m_nScore(0)
{
    //	Register
    if (sUsername.length() > 2)
    {
        ASSERT(!RAUsers::DatabaseContainsUser(sUsername));
        RAUsers::RegisterUser(sUsername, this);
    }
}

LocalRAUser::LocalRAUser(const std::string& sUser) :
    RAUser(sUser),
    m_bIsLoggedIn(FALSE),
    m_bStoreToken(FALSE)
{
}

void LocalRAUser::AttemptLogin(bool bBlocking)
{
    m_bIsLoggedIn = FALSE;

    if (!Username().empty() && !Token().empty())
    {
        if (bBlocking)
        {
            PostArgs args;
            args['u'] = Username();
            args['t'] = Token();		//	Plaintext password(!)

            Document doc;
            if (RAWeb::DoBlockingRequest(ra::RequestType::Login, args, doc))
            {
                HandleSilentLoginResponse(doc);
            }
        }
        else
        {
            AttemptSilentLogin();
        }
    }
    else
    {
        //	Push dialog to get them to login!
        DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_LOGIN), g_RAMainWnd, RA_Dlg_Login::RA_Dlg_LoginProc);
        _RA_SavePreferences();
    }

}

void LocalRAUser::AttemptSilentLogin()
{
    //	NB. Don't login here: cause a login when http request returns!
    PostArgs args;
    args['u'] = Username();
    args['t'] = Token();
    RAWeb::CreateThreadedHTTPRequest(ra::RequestType::Login, args);

    m_bStoreToken = TRUE;	//	Store it! We just used it!
}

void LocalRAUser::HandleSilentLoginResponse(Document& doc)
{
    if (doc.HasMember("Success") && doc["Success"].GetBool())
    {
        const std::string& sUser = doc["User"].GetString();
        const std::string& sToken = doc["Token"].GetString();
        const unsigned int nPoints = doc["Score"].GetUint();
        const unsigned int nUnreadMessages = doc["Messages"].GetUint();
        ProcessSuccessfulLogin(sUser, sToken, nPoints, nUnreadMessages, TRUE);
    }
    else if (doc.HasMember("Error"))
    {
        MessageBox(nullptr, NativeStr(doc["Error"].GetString()).c_str(), TEXT("Login Failed"), MB_OK);
    }
    else
    {
        MessageBox(nullptr, TEXT("Login failed, please login again."), TEXT("Login Failed"), MB_OK);
    }
}

void LocalRAUser::ProcessSuccessfulLogin(const std::string& sUser, const std::string& sToken, unsigned int nPoints, unsigned int nMessages, BOOL bRememberLogin)
{
    m_bIsLoggedIn = TRUE;

    SetUsername(sUser);
    SetToken(sToken);
    SetScore(nPoints);
    //SetUnreadMessageCount( nMessages );

    //	Used only for persistence: always store in memory (we need it!)
    SetStoreToken(bRememberLogin);

    m_aFriends.clear();

    ra::services::g_ImageRepository.FetchImage(ra::services::ImageType::UserPic, sUser);
    FriendList();

    // this thing is still leaking...
    std::ostringstream oss;
    oss << "Welcome back " << Username() << " (" << nPoints << ")";
    auto sTitle{ oss.str() };
    oss.str("");
    std::string sNumMsgs{ (nMessages == 1) ? "message" : "messages" };
    oss << "You have " << nMessages << " new " << sNumMsgs << ".";
    auto sSubTitle{ oss.str() };

    // lets try something
    MessagePopup myPopup{
        sTitle, sSubTitle, ra::PopupMessageType::Login, ra::services::ImageType::UserPic, Username()
    };
    g_PopupWindows.AchievementPopups().AddMessage(std::move(myPopup));

    g_AchievementsDialog.OnLoad_NewRom(g_pCurrentGameData->GetGameID());
    g_AchievementEditorDialog.OnLoad_NewRom();
    g_AchievementOverlay.OnLoad_NewRom();

    RA_RebuildMenu();
    _RA_UpdateAppTitle();
}

void LocalRAUser::Logout()
{
    Clear();
    RA_RebuildMenu();
    _RA_UpdateAppTitle("");

    m_bIsLoggedIn = FALSE;

    MessageBox(nullptr, TEXT("You are now logged out."), TEXT("Info"), MB_OK);
}

void LocalRAUser::OnFriendListResponse(const Document& doc)
{
    if (!doc.HasMember("Friends"))
        return;

    const Value& FriendData = doc["Friends"];		//{"Friend":"LucasBarcelos5","RAPoints":"355","LastSeen":"Unknown"}

    for (SizeType i = 0; i < FriendData.Size(); ++i)
    {
        const Value& NextFriend = FriendData[i];

        RAUser* pUser = RAUsers::GetUser(NextFriend["Friend"].GetString());
        pUser->SetScore(strtol(NextFriend["RAPoints"].GetString(), nullptr, 10));
        pUser->UpdateActivity(NextFriend["LastSeen"].GetString());

        AddFriend(pUser->Username(), pUser->GetScore());
    }
}

void LocalRAUser::FriendList()
{
    PostArgs args;
    args['u'] = Username();
    args['t'] = Token();

    RAWeb::CreateThreadedHTTPRequest(ra::RequestType::FriendList, args);
}

RAUser* LocalRAUser::AddFriend(const std::string& sUser, unsigned int nScore)
{
    RAUser* pUser = RAUsers::GetUser(sUser);
    pUser->SetScore(nScore);
    ra::services::g_ImageRepository.FetchImage(ra::services::ImageType::UserPic, sUser);

    std::vector<RAUser*>::const_iterator iter = m_aFriends.begin();
    while (iter != m_aFriends.end())
    {
        if ((*iter) == pUser)
            break;

        iter++;
    }

    if (iter == m_aFriends.end())
        m_aFriends.push_back(pUser);

    return pUser;
}

void LocalRAUser::PostActivity(ra::ActivityType eActivityType)
{
    if (eActivityType == ra::ActivityType::PlayerStartedPlaying)
    {
        PostArgs args;
        args['u'] = Username();
        args['t'] = Token();
        args['a'] = ra::etos(eActivityType);
        args['m'] = std::to_string(g_pCurrentGameData->GetGameID());

        RAWeb::CreateThreadedHTTPRequest(ra::RequestType::PostActivity, args);
    }
    else //	unhandled
        ASSERT(!"User isn't designed to handle posting this activity!");
}

void LocalRAUser::Clear()
{
    SetToken("");
    m_bIsLoggedIn = FALSE;
}

RAUser* LocalRAUser::FindFriend(const std::string& sName)
{
    std::vector<RAUser*>::iterator iter = m_aFriends.begin();
    while (iter != m_aFriends.end())
    {
        if (sName.compare((*iter)->Username()) == 0)
            return *iter;
    }
    return nullptr;
}
