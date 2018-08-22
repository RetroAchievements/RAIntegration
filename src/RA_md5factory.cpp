#include "RA_md5factory.h"
#include "md5.h"

#include <memory>
#include <cassert>
#include "ra_utility.h"

namespace ra {
_CONSTANT_VAR MD5_STRING_LEN = 32U;
} // namespace ra

_Use_decl_annotations_
std::string RAGenerateMD5(const std::string& sStringToMD5) noexcept
{
    char buffer[33]{};

    auto pms{ std::make_unique<md5_state_t>() };
    auto digest{ std::make_unique<md5_byte_t[]>(16) };
    auto pDataBuffer{ std::make_unique<md5_byte_t[]>(sStringToMD5.length()) };

    assert(pDataBuffer != nullptr); // this should honestly be impossible via make_unique
    if (pDataBuffer == nullptr)
        return std::string{};

    std::memcpy(static_cast<void*>(pDataBuffer.get()),
                static_cast<const void*>(sStringToMD5.c_str()),
                sStringToMD5.length());

    md5_init(pms.get());
    md5_append(pms.get(), pDataBuffer.get(), sStringToMD5.length());
    md5_finish(pms.get(), digest.get());

    std::memset(static_cast<void*>(buffer), 0, ra::MD5_STRING_LEN + 1U);
    sprintf_s(buffer, ra::MD5_STRING_LEN + 1U,
              "%02x%02x%02x%02x%02x%02x%02x%02x"
              "%02x%02x%02x%02x%02x%02x%02x%02x",
              digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
              digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    return buffer;
}

_Use_decl_annotations_
std::string RAGenerateMD5(const BYTE* pRawData, size_t nDataLen) noexcept
{
    // after some experimentation, it does seem that releasing unique_ptr causes a leak so the return string will be
    // statically allocated. The rest did not leak.
    char buffer[33]{};
    auto pms{ std::make_unique<md5_state_t>() };
    auto digest{ std::make_unique<md5_byte_t[]>(16) };

    // quick-info will automatically tell you if it's true or not
    static_assert(ra::is_same_size_v<md5_byte_t, BYTE>, "Must be equivalent for the MD5 to work!");

    md5_init(pms.get());
    md5_append(pms.get(), static_cast<const md5_byte_t*>(pRawData), ra::to_signed(nDataLen));
    md5_finish(pms.get(), digest.get());

    std::memset(static_cast<void*>(buffer), 0, ra::MD5_STRING_LEN + 1U);
    sprintf_s(buffer, ra::MD5_STRING_LEN + 1U,
              "%02x%02x%02x%02x%02x%02x%02x%02x"
              "%02x%02x%02x%02x%02x%02x%02x%02x",
              digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
              digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    return buffer; // Implicit promotion to std::string
}

std::string RAGenerateMD5(const std::vector<BYTE>& DataIn) noexcept
{
    return RAGenerateMD5(DataIn.data(), DataIn.size());
}
