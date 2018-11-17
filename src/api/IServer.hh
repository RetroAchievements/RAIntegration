#ifndef RA_API_ISERVER_HH
#define RA_API_ISERVER_HH
#pragma once

#include "api/Login.hh"
#include "api/Logout.hh"
#include "api/Ping.hh"
#include "api/StartSession.hh"

namespace ra {
namespace api {

class IServer
{
public:
    virtual const char* Name() const noexcept = 0;

    // === user functions ===

    virtual Login::Response Login(const Login::Request& request) noexcept = 0;
    virtual Logout::Response Logout(const Logout::Request& request) noexcept = 0;
    virtual StartSession::Response StartSession(const StartSession::Request& request) noexcept = 0;
    virtual Ping::Response Ping(const Ping::Request& request) noexcept = 0;

protected:
    IServer() noexcept = default;

public:
    virtual ~IServer() noexcept = default;
    IServer(const IServer&) = delete;
    IServer& operator=(const IServer&) = delete;
    IServer(IServer&&) noexcept = delete;
    IServer& operator=(IServer&&) noexcept = delete;
};

} // namespace api
} // namespace ra

#endif !RA_API_ISERVER_HH
