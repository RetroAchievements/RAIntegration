#ifndef RA_UTIL_MD5_HH
#define RA_UTIL_MD5_HH
#pragma once

#include "GSL.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace ra {
namespace util {

class MD5
{
public:
    MD5() noexcept;
    ~MD5() noexcept;
    MD5(const MD5& other) noexcept = delete;
    MD5& operator=(const MD5&) noexcept = delete;
    MD5(MD5&& other) noexcept = delete;
    MD5& operator=(MD5&&) noexcept = delete;

    /// <summary>
    /// Adds the provided data to the current state.
    /// </summary>
    void Append(const uint8_t* pBytes, size_t nBytes) noexcept;

    /// <summary>
    /// Finalizes the state and populates <paramref name="pDigest" /> with the raw MD5.
    /// </summary>
    void Generate(uint8_t pDigest[16]) noexcept;

    /// <summary>
    /// Finalizes the state and returns a 32-character MD5 string.
    /// </summary>
    std::string Generate();

    /// <summary>
    /// Generates a raw MD5 for a series of bytes.
    /// </summary>
    static void Generate(const uint8_t* pBytes, size_t nBytes, uint8_t pDigest[16]) noexcept
    {
        MD5 md5;
        md5.Append(pBytes, nBytes);
        md5.Generate(pDigest);
    }

    /// <summary>
    /// Generates a 32-character MD5 string for a series of bytes.
    /// </summary>
    static std::string Generate(const uint8_t* pBytes, size_t nBytes)
    {
        MD5 md5;
        md5.Append(pBytes, nBytes);
        return md5.Generate();
    }

    /// <summary>
    /// Generates a 32-character MD5 string for a string.
    /// </summary>
    static std::string Generate(const char* sText, size_t nChars)
    {
        GSL_SUPPRESS_TYPE1
        return MD5::Generate(reinterpret_cast<const uint8_t*>(sText), nChars);
    }

    /// <summary>
    /// Generates a 32-character MD5 string for a string.
    /// </summary>
    static std::string Generate(const std::string_view sText)
    {
        return MD5::Generate(sText.data(), sText.length());
    }

private:
    struct Impl;
    std::unique_ptr<Impl> m_pState;
};

} // namespace util
} // namespace ra

#endif // !RA_UTIL_MD5_HH
