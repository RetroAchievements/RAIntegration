#ifndef RA_MD5FACTORY_H
#define RA_MD5FACTORY_H
#pragma once

#include "ra_fwd.h"
#include <vector>

_NODISCARD std::string RAGenerateMD5(_In_ const std::string& sStringToMD5) noexcept;
_NODISCARD std::string RAGenerateMD5(_In_ const BYTE* const pIn, _In_ size_t nLen) noexcept;
_NODISCARD std::string RAGenerateMD5(_In_ const std::vector<BYTE>& DataIn) noexcept;

#endif // !RA_MD5FACTORY_H
