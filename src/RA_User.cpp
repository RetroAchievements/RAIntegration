#include "RA_User.h"

#include "RA_Achievement.h"
#include "RA_AchievementOverlay.h"
#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Login.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_Interface.h"
#include "RA_PopupWindows.h"
#include "RA_Resource.h"

#include "data\GameContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

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
    m_bIsLoggedIn(FALSE)
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
            args['t'] = Token();

            rapidjson::Document doc;
            if (RAWeb::DoBlockingRequest(RequestLogin, args, doc))
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
        ra::services::ServiceLocator::Get<ra::services::IConfiguration>().Save();
    }

}

void LocalRAUser::AttemptSilentLogin()
{
    //	NB. Don't login here: cause a login when http request returns!
    PostArgs args;
    args['u'] = Username();
    args['t'] = Token();
    RAWeb::CreateThreadedHTTPRequest(RequestLogin, args);
}

void LocalRAUser::HandleSilentLoginResponse(rapidjson::Document& doc)
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

    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    pConfiguration.SetUsername(sUser);
    if (bRememberLogin)
        pConfiguration.SetApiToken(sToken);

    m_aFriends.clear();

    ra::services::g_ImageRepository.FetchImage(ra::services::ImageType::UserPic, sUser);
    RequestFriendList();

    g_PopupWindows.AchievementPopups().AddMessage(
        MessagePopup("Welcome back " + Username() + " (" + std::to_string(nPoints) + ")",
        "You have " + std::to_string(nMessages) + " new " + std::string((nMessages == 1) ? "message" : "messages") + ".",
        PopupMessageType::PopupLogin, ra::services::ImageType::UserPic, Username()));

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    g_AchievementsDialog.OnLoad_NewRom(pGameContext.GameId());
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

void LocalRAUser::OnFriendListResponse(const rapidjson::Document& doc)
{
    if (!doc.HasMember("Friends"))
        return;

    const auto& FriendData{ doc["Friends"] };		//{"Friend":"LucasBarcelos5","RAPoints":"355","LastSeen":"Unknown"}
    for (auto& NextFriend : FriendData.GetArray())
    {
        auto pUser{ RAUsers::GetUser(NextFriend["Friend"].GetString()) };
        pUser->SetScore(std::stoul(NextFriend["RAPoints"].GetString()));
        pUser->UpdateActivity(NextFriend["LastSeen"].GetString());

        AddFriend(pUser->Username(), pUser->GetScore());
    }
}

void LocalRAUser::RequestFriendList()
{
    PostArgs args;
    args['u'] = Username();
    args['t'] = Token();

    RAWeb::CreateThreadedHTTPRequest(RequestType::RequestFriendList, args);
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

void LocalRAUser::PostActivity(ActivityType nActivityType)
{
    switch (nActivityType)
    {
        case PlayerStartedPlaying:
        {
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

            PostArgs args;
            args['u'] = Username();
            args['t'] = Token();
            args['a'] = std::to_string(nActivityType);
            args['m'] = std::to_string(pGameContext.GameId());

            RAWeb::CreateThreadedHTTPRequest(RequestPostActivity, args);
            break;
        }

        default:
            //	unhandled
            ASSERT(!"User isn't designed to handle posting this activity!");
            break;
    }
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
