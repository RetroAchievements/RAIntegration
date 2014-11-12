#pragma once

#include <string>
#include "RA_Defs.h"

namespace RA
{
	std::string GenerateMD5( const std::string& sStringToMD5 );
	std::string GenerateMD5( const BYTE* pIn, size_t nLen );
	std::string GenerateMD5( const std::vector<BYTE> DataIn );
}