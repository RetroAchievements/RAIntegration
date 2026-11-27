#include "MD5.hh"

#include "Strings.hh"

#include <rcheevos/src/util/md5.h>

namespace ra {
namespace util {

static_assert(sizeof(md5_byte_t) == sizeof(char), "Must be equivalent for MD5::Generate(std::string)");

struct MD5::Impl : public md5_state_t
{
};

GSL_SUPPRESS_F6
MD5::MD5() noexcept
{
    m_pState = std::make_unique<Impl>();
    md5_init(m_pState.get());
}

MD5::~MD5()
{
}

void MD5::Append(const uint8_t* pBytes, size_t nBytes) noexcept
{
    md5_append(m_pState.get(), pBytes, gsl::narrow_cast<int>(nBytes));
}

void MD5::Generate(uint8_t pDigest[16]) noexcept
{
    md5_finish(m_pState.get(), pDigest);
}

std::string MD5::Generate()
{
    uint8_t pDigest[16];
    Generate(pDigest);
    return ra::util::String::FormatHex(pDigest, sizeof(pDigest));
}

} // namespace util
} // namespace ra
