#pragma once

#include "api/IServer.hh"

#include "ra_fwd.h"

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
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

    StartSession::Response StartSession(_UNUSED const StartSession::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<StartSession::Response>(StartSession::Name());
    }

    Ping::Response Ping(_UNUSED const Ping::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<Ping::Response>(Ping::Name());
    }

protected:
    template<typename TResponse>
    GSL_SUPPRESS(f.6) 
    inline typename TResponse UnsupportedApi(const char* const restrict apiName) const
    {
        static_assert(std::is_base_of<ApiResponseBase, TResponse>::value, "TResponse must derive from ApiResponseBase");

        TResponse response;
        response.Result       = ApiResult::Unsupported;
        response.ErrorMessage = ra::StringPrintf("%s is not supported by %s.", apiName, Name());
        return std::move(response);
    }
};

} // namespace impl
} // namespace api
} // namespace ra
