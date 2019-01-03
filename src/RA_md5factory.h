#ifndef RA_MD5FACTORY_H
#define RA_MD5FACTORY_H
#pragma once

_NODISCARD std::string RAGenerateMD5(const std::string& sStringToMD5);
_NODISCARD std::string RAGenerateMD5(std::unique_ptr<BYTE[]> pIn, size_t nLen);
_NODISCARD std::string RAGenerateMD5(gsl::span<const BYTE> pIn);

#endif // !RA_MD5FACTORY_H
