#include "RA_User.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Defs.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_ImageFactory.h"
#include "RA_Interface.h"
#include "RA_Resource.h"
#include "RA_httpthread.h"

#include "api\Login.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

// static
LocalRAUser RAUsers::ms_LocalUser;
std::vector<RAUser> RAUsers::UserDatabase;

// static
BOOL RAUsers::DatabaseContainsUser(const std::string& sUser) { return (FindUser(sUser) != UserDatabase.end()); }

// static
RAUser& RAUsers::GetUser(const std::string& sUser)
{
    if (DatabaseContainsUser(sUser) == FALSE)
        RegisterUser(RAUser{sUser});

    // don't need to check here since it was already
    return *FindUser(sUser);
}

RAUser::RAUser(const std::string& sUsername) : m_sUsername{sUsername}
{
    // Register
    if (sUsername.length() > 2)
    {
        Expects(!RAUsers::DatabaseContainsUser(sUsername));
        RAUsers::RegisterUser(std::move(*this));
    }
}

