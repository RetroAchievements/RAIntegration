#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class DisconnectedServer : public ServerBase
{
public:
    explicit DisconnectedServer(const std::string& sHost) : m_sHost(sHost) {}

    const char* Name() const noexcept override { return "disconnected client"; }

    const std::string& Host() const noexcept { return m_sHost; }

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
