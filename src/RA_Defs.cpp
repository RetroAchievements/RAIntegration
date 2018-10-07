#include "RA_Defs.h"

#ifndef PCH_H
#include <iomanip>  
#endif /* !PCH_H */

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ByteAddress nAddr)
{
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(6) << std::hex << nAddr;
    return oss.str();
}

} /* namespace ra */
