#ifndef RA_MD5FACTORY_H
#define RA_MD5FACTORY_H
#pragma once

#include "RA_Defs.h"

std::string RAGenerateMD5(const std::string& sStringToMD5);
std::string RAGenerateMD5(const BYTE* pIn, size_t nLen);
std::string RAGenerateMD5(const std::vector<BYTE> DataIn);

#endif // !RA_MD5FACTORY_H
