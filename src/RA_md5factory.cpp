#include "RA_md5factory.h"

#include "RA_Defs.h"

_CONSTANT_VAR MD5_STRING_LEN = 32;

std::string RAGenerateMD5(const std::string& sStringToMD5)
{
    md5_state_t pms{};
    std::array<md5_byte_t, 16> digest{};

    std::basic_string<md5_byte_t> sData;
    for (const md5_byte_t& c : sStringToMD5)
        sData.push_back(c);

    md5_init(&pms);
    md5_append(&pms, sData.data(), sStringToMD5.length());
    md5_finish(&pms, digest.data());

    std::array<char, MD5_STRING_LEN + 1> buffer{};
    sprintf_s(buffer.data(), MD5_STRING_LEN + 1,
              "%02x%02x%02x%02x%02x%02x%02x%02x"
              "%02x%02x%02x%02x%02x%02x%02x%02x",
              digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7], digest[8],
              digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    return buffer.data(); // Implicit promotion to std::string
}

std::string RAGenerateMD5(std::unique_ptr<BYTE[]> pIn, size_t nLen)
{
    auto owner = std::move(pIn);
    return RAGenerateMD5(gsl::make_span(owner.get(), ra::to_signed(nLen)));
}

std::string RAGenerateMD5(gsl::span<const BYTE> pRawData)
{
    static_assert(sizeof(md5_byte_t) == sizeof(BYTE), "Must be equivalent for the MD5 to work!");

    md5_state_t pms{};
    std::array<md5_byte_t, 16> digest{};

    md5_init(&pms);
    md5_append(&pms, pRawData.data(), gsl::narrow<int>(pRawData.size_bytes()));
    md5_finish(&pms, digest.data());

    std::array<char, MD5_STRING_LEN + 1> buffer{};
    sprintf_s(buffer.data(), MD5_STRING_LEN + 1,
              "%02x%02x%02x%02x%02x%02x%02x%02x"
              "%02x%02x%02x%02x%02x%02x%02x%02x",
              digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7], digest[8],
              digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    return buffer.data(); // Implicit promotion to std::string
}
