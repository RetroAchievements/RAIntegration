#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class OfflineServer : public ServerBase
{
public:
    const char* Name() const noexcept override { return "offline client"; }

    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override;

    LatestClient::Response LatestClient(const LatestClient::Request& request) override;
};

} // namespace impl
} // namespace api
} // namespace ra
