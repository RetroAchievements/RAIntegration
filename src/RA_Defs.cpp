#include "RA_Defs.h"

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ByteAddress nAddr, std::streamsize nPrecision, bool bShowBase)
{
    std::ostringstream oss;
    auto showingBase = std::showbase;
    if (!bShowBase)
        showingBase = std::noshowbase;
    oss << std::setfill('0') << std::setw(nPrecision) << std::hex << showingBase << nAddr;
    return oss.str();
}

} /* namespace ra */
