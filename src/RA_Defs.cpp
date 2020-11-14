#include "RA_Defs.h"

#include "data\context\EmulatorContext.hh"

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ByteAddress nAddr)
{
#ifndef RA_UTEST
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.TotalMemorySize() > 0x10000)
        return ra::StringPrintf("0x%06x", nAddr);
    else
#endif
        return ra::StringPrintf("0x%04x", nAddr);
}

_Use_decl_annotations_
ByteAddress ByteAddressFromString(const std::string& sByteAddress)
{
    ra::ByteAddress address{};

    if (!ra::StringStartsWith(sByteAddress, "-")) // negative addresses not supported
    {
        char* pEnd;
        address = std::strtoul(sByteAddress.c_str(), &pEnd, 16);
        Ensures(pEnd != nullptr);
        if (*pEnd)
        {
            // hex parse failed
            address = {};
        }
    }

    return address;
}

} /* namespace ra */
