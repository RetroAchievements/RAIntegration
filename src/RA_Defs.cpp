#include "RA_Defs.h"

#include <iomanip>

namespace ra {

std::string ByteAddressToString(ByteAddress nAddr)
{
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(6) << std::hex << nAddr;
    return oss.str();
}

} /* namespace ra */
