#include "RA_md5factory.h"

#include "RA_Defs.h"

_CONSTANT_VAR MD5_STRING_LEN = 32;

std::string RAGenerateMD5(const std::string& sStringToMD5)
{
    md5_state_t pms{};
    std::array<md5_byte_t, 16> digest{};

    auto pDataBuffer = std::make_unique<md5_byte_t[]>(sStringToMD5.length());
    ASSERT(pDataBuffer != nullptr);
    if (pDataBuffer == nullptr)
        return {};

    std::memcpy(pDataBuffer.get(), sStringToMD5.c_str(), sStringToMD5.length());

    md5_init(&pms);
    md5_append(&pms, pDataBuffer.get(), sStringToMD5.length());
    md5_finish(&pms, digest.data());

    std::array<char, MD5_STRING_LEN + 1> buffer{};
    Ensures(sprintf_s(buffer.data(), buffer.size(),
                      "%02x%02x%02x%02x%02x%02x%02x%02x"
                      "%02x%02x%02x%02x%02x%02x%02x%02x",
                      digest.at(0), digest.at(1), digest.at(2), digest.at(3), digest.at(4), digest.at(5), digest.at(6),
                      digest.at(7), digest.at(8), digest.at(9), digest.at(10), digest.at(11), digest.at(12),
                      digest.at(13), digest.at(14), digest.at(15)) >= 0);

    pDataBuffer.reset();
    return buffer.data();
}

std::string RAGenerateMD5(const BYTE* pRawData, size_t nDataLen)
{
    md5_state_t pms{};
    std::array<md5_byte_t, 16> digest{};

    static_assert(ra::is_same_size_v<md5_byte_t, BYTE>, "Must be equivalent for the MD5 to work!");

    md5_init(&pms);
    md5_append(&pms, pRawData, nDataLen);
    md5_finish(&pms, digest.data());

    std::array<char, MD5_STRING_LEN + 1> buffer{};
    Ensures(sprintf_s(buffer.data(), buffer.size(),
                      "%02x%02x%02x%02x%02x%02x%02x%02x"
                      "%02x%02x%02x%02x%02x%02x%02x%02x",
                      digest.at(0), digest.at(1), digest.at(2), digest.at(3), digest.at(4), digest.at(5), digest.at(6),
                      digest.at(7), digest.at(8), digest.at(9), digest.at(10), digest.at(11), digest.at(12),
                      digest.at(13), digest.at(14), digest.at(15)) >= 0);

    return buffer.data(); //	Implicit promotion to std::string
}

std::string RAGenerateMD5(const std::vector<BYTE> DataIn) { return RAGenerateMD5(DataIn.data(), DataIn.size()); }
