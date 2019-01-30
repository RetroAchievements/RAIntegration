#include "RA_Defs.h"

#include "RA_MemManager.h"

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ByteAddress nAddr)
{
#ifndef RA_UTEST
    if (g_MemManager.TotalBankSize() > 0x10000)
        return ra::StringPrintf("0x%06x", nAddr);
    else
#endif
        return ra::StringPrintf("0x%04x", nAddr);
}

_Use_decl_annotations_
ByteAddress ByteAddressFromString(const std::string& sByteAddress) noexcept
{
    ra::ByteAddress address{};

    if (!ra::StringStartsWith(sByteAddress, "-")) // negative addresses not supported
    {
        char* pEnd;
        address = std::strtoul(sByteAddress.c_str(), &pEnd, 10);
        assert(pEnd != nullptr);
        if (*pEnd)
        {
            // decimal parse failed, try hex
            address = std::strtoul(sByteAddress.c_str(), &pEnd, 16);
            assert(pEnd != nullptr);
            if (*pEnd)
            {
                // hex parse failed
                address = {};
            }
        }
    }

    return address;
}

} /* namespace ra */
