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
LocalRAUser RAUsers::ms_LocalUser;
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
