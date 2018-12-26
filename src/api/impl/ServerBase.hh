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

    GSL_SUPPRESS_F6 Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override
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

    ResolveHash::Response ResolveHash(_UNUSED const ResolveHash::Request& /*request*/) noexcept override
    {
        GSL_SUPPRESS_F6
        return UnsupportedApi<ResolveHash::Response>(ResolveHash::Name());
    }

protected:
    template<typename TResponse>
    inline typename TResponse UnsupportedApi(const char* const restrict apiName) const
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
