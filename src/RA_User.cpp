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

#include "api\Login.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

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

static void HandleLoginResponse(LocalRAUser& user, const ra::api::Login::Response& response)
{
    if (response.Succeeded())
    {
        user.ProcessSuccessfulLogin(response.Username, response.ApiToken, response.Score, response.NumUnreadMessages, true);
    }
    else if (!response.ErrorMessage.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            L"Login Failed", ra::Widen(response.ErrorMessage)
        );
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            L"Login Failed", L"Please login again."
        );
    }
}

void LocalRAUser::AttemptLogin(bool bBlocking)
{
    m_bIsLoggedIn = FALSE;

    if (!Username().empty() && !Token().empty())
    {
        ra::api::Login::Request request;
        request.Username = Username();
        request.ApiToken = Token();

        if (bBlocking)
        {
            ra::api::Login::Response response = request.Call();
            HandleLoginResponse(*this, response);
        }
        else
        {
            request.CallAsync([this](const ra::api::Login::Response& response)
            {
                HandleLoginResponse(*this, response);
            });
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
    ra::api::Login::Request request;
    request.Username = Username();
    request.ApiToken = Token();
    request.CallAsync([this](const ra::api::Login::Response& response)
    {
        HandleLoginResponse(*this, response);
    });
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

    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    pImageRepository.FetchImage(ra::ui::ImageType::UserPic, sUser);
    RequestFriendList();

    g_PopupWindows.AchievementPopups().AddMessage(
        MessagePopup("Welcome back " + Username() + " (" + std::to_string(nPoints) + ")",
        "You have " + std::to_string(nMessages) + " new " + std::string((nMessages == 1) ? "message" : "messages") + ".",
        PopupMessageType::PopupLogin, ra::ui::ImageType::UserPic, Username()));

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    g_AchievementsDialog.OnLoad_NewRom(pGameContext.GameId());
    g_AchievementEditorDialog.OnLoad_NewRom();
    g_AchievementOverlay.OnLoad_NewRom();

    auto& pSessionTracker = ra::services::ServiceLocator::GetMutable<ra::data::SessionTracker>();
    pSessionTracker.Initialize(sUser);

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
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    pImageRepository.FetchImage(ra::ui::ImageType::UserPic, sUser);

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
