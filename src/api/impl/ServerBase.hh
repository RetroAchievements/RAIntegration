#ifndef SERVERBASE_HH
#define SERVERBASE_HH
#pragma once

#include "api/IServer.hh"

#include "RA_StringUtils.h"
#include "RA_StringUtils.h"

#include <string>

namespace ra {
namespace api {
namespace impl {

class ServerBase : public IServer
{
public:
    // === user functions ===
    Login::Response Login(_UNUSED const Login::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

    StartSession::Response StartSession(_UNUSED const StartSession::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<StartSession::Response>(StartSession::Name());
    }

    Ping::Response Ping(_UNUSED const Ping::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<Ping::Response>(Ping::Name());
    }

    FetchUserUnlocks::Response FetchUserUnlocks(_UNUSED const FetchUserUnlocks::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<FetchUserUnlocks::Response>(FetchUserUnlocks::Name());
    }

    AwardAchievement::Response AwardAchievement(_UNUSED const AwardAchievement::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<AwardAchievement::Response>(AwardAchievement::Name());
    }

    // === game functions ===

    ResolveHash::Response ResolveHash(_UNUSED const ResolveHash::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<ResolveHash::Response>(ResolveHash::Name());
    }

    FetchGameData::Response FetchGameData(_UNUSED const FetchGameData::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<FetchGameData::Response>(FetchGameData::Name());
    }

    // === other functions ===

    LatestClient::Response LatestClient(_UNUSED const LatestClient::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<LatestClient::Response>(LatestClient::Name());
    }

protected:
    template<typename TResponse>
    inline typename TResponse UnsupportedApi(const char* const restrict apiName) const noexcept
    {
        static_assert(std::is_base_of<ApiResponseBase, TResponse>::value, "TResponse must derive from ApiResponseBase");

        TResponse response;
        response.Result = ApiResult::Unsupported;
        GSL_SUPPRESS_F6
        response.ErrorMessage = ra::StringPrintf("%s is not supported by %s.", apiName, Name());
        return response;
    }
};

} // namespace impl
} // namespace api
} // namespace ra

#endif /* !SERVERBASE_HH */
