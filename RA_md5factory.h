#pragma once

#include <string>
#include "RA_Defs.h"

extern std::string RAGenerateMD5( const std::string& sStringToMD5 );
extern std::string RAGenerateMD5( const BYTE* pIn, size_t nLen );
extern std::string RAGenerateMD5( const std::vector<BYTE> DataIn );