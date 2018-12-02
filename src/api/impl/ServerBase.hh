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
    [[gsl::suppress(f.6)]] Login::Response Login(_UNUSED const Login::Request& request) override
    {
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    [[gsl::suppress(f.6)]] Logout::Response Logout(_UNUSED const Logout::Request& request) override
    {
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

    [[gsl::suppress(f.6)]] StartSession::Response StartSession(_UNUSED const StartSession::Request& request) override
    {
        return UnsupportedApi<StartSession::Response>(StartSession::Name());
    }

    [[gsl::suppress(f.6)]] Ping::Response Ping(_UNUSED const Ping::Request& request) override
    {
        return UnsupportedApi<Ping::Response>(Ping::Name());
    }

protected:
    template<typename TResponse>
    inline typename TResponse UnsupportedApi(const char* const apiName) const
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
