#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class OfflineServer : public ServerBase
{
public:
    const char* Name() const noexcept override { return "offline client"; }

    Login::Response Login(const Login::Request& request) override;
    Logout::Response Logout(const Logout::Request&) override;

    FetchGameData::Response FetchGameData(const FetchGameData::Request& request) override;
    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override;
};

} // namespace impl
} // namespace api
} // namespace ra
