#include "RA_md5factory.h"

#include "RA_Defs.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos/src/rhash/md5.h>

namespace {
constexpr static unsigned int MD5_STRING_LEN = 32;
}

std::string RAFormatMD5(const BYTE* digest)
{
    char buffer[33] = "";
    Expects(digest != nullptr);

    sprintf_s(buffer, MD5_STRING_LEN + 1,
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
        digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    return std::string(buffer, 32);
}

std::string RAGenerateMD5(const std::string& sStringToMD5)
{
    md5_state_t pms{};
    md5_byte_t digest[16]{};

    static_assert(sizeof(md5_byte_t) == sizeof(char), "Must be equivalent for the MD5 to work!");

    const md5_byte_t* bytes;
    GSL_SUPPRESS_TYPE1 bytes = reinterpret_cast<const md5_byte_t*>(sStringToMD5.c_str());

    md5_init(&pms);
    md5_append(&pms, bytes, gsl::narrow_cast<int>(sStringToMD5.length()));
    md5_finish(&pms, digest);

    return RAFormatMD5(digest);
}

std::string RAGenerateMD5(const BYTE* pRawData, size_t nDataLen)
{
    md5_state_t pms;
    md5_byte_t digest[16];

    static_assert(sizeof(md5_byte_t) == sizeof(BYTE), "Must be equivalent for the MD5 to work!");

    md5_init(&pms);
    md5_append(&pms, pRawData, gsl::narrow_cast<int>(nDataLen));
    md5_finish(&pms, digest);

    return RAFormatMD5(digest);
}

std::string RAGenerateMD5(const std::vector<BYTE> DataIn)
{
    return RAGenerateMD5(DataIn.data(), DataIn.size());
}

std::string RAGenerateFileMD5(const std::wstring& sPath)
{
    md5_state_t pms{};
    md5_byte_t digest[16]{};

    md5_byte_t bytes[4096]{};
    static_assert(sizeof(md5_byte_t) == sizeof(uint8_t), "Must be equivalent for the MD5 to work!");

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pFile = pFileSystem.OpenTextFile(sPath);
    if (pFile == nullptr)
        return "";

    md5_init(&pms);
    do
    {
        size_t nBytes = pFile->GetBytes(static_cast<uint8_t*>(bytes), sizeof(bytes));
        if (nBytes == 0)
            break;

        md5_append(&pms, bytes, gsl::narrow_cast<int>(nBytes));
    } while (true);
    md5_finish(&pms, digest);

    return RAFormatMD5(digest);
}

