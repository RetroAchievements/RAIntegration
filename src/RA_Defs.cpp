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
ByteAddress ByteAddressFromString(const std::string& sByteAddress)
{
    try
    {
        if (ra::StringStartsWith(sByteAddress, "0x") || ra::StringStartsWith(sByteAddress, "0X"))
        {
            if (sByteAddress.length() > 2)
                return ra::ByteAddress{ std::stoul(&sByteAddress.at(2), nullptr, 16) };
        }
        else if (ra::StringStartsWith(sByteAddress, "-"))
        {
            // negative addresses not supported
            return ra::ByteAddress{};
        }
        else if (!sByteAddress.empty())
        {
            try
            {
                return ra::ByteAddress{ std::stoul(sByteAddress.c_str()) };
            }
            catch (const std::invalid_argument&)
            {
                // try hex conversion
                return ra::ByteAddress{ std::stoul(sByteAddress.c_str(), nullptr, 16) };
            }
        }
    }
    catch (const std::invalid_argument&)
    {
        // ignore and return default
    }
    catch (const std::out_of_range&)
    {
        // ignore and return default
    }

    return ra::ByteAddress{};
}

} /* namespace ra */
