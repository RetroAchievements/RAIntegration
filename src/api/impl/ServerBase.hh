#ifndef SERVERBASE_HH
#define SERVERBASE_HH
#pragma once

#include "api/IServer.hh"

#include "RA_StringUtils.h"
#include "ra_fwd.h"
#include "RA_StringUtils.h"

#include <string>

namespace ra {
namespace api {
namespace impl {

class ServerBase : public IServer
{
public:
    // === user functions ===
    GSL_SUPPRESS(f.6) Login::Response Login(_UNUSED const Login::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    GSL_SUPPRESS(f.6) Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

    GSL_SUPPRESS(f.6)
    StartSession::Response StartSession(_UNUSED const StartSession::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<StartSession::Response>(StartSession::Name());
    }

    GSL_SUPPRESS(f.6) Ping::Response Ping(_UNUSED const Ping::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<Ping::Response>(Ping::Name());
    }

    GSL_SUPPRESS(f.6) ResolveHash::Response ResolveHash(_UNUSED const ResolveHash::Request& /*request*/) noexcept override
    {
        return UnsupportedApi<ResolveHash::Response>(ResolveHash::Name());
    }

protected:
    template<typename TResponse>
    GSL_SUPPRESS(f.6)
    inline typename TResponse UnsupportedApi(const char* const restrict apiName) const
    {
        static_assert(std::is_base_of<ApiResponseBase, TResponse>::value, "TResponse must derive from ApiResponseBase");

        TResponse response;
        response.Result = ApiResult::Unsupported;
        response.ErrorMessage = ra::StringPrintf("%s is not supported by %s.", apiName, Name());
        return response;
    }
};

} // namespace impl
} // namespace api
} // namespace ra

#endif /* !SERVERBASE_HH */
