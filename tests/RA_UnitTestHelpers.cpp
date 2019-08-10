#include "RA_UnitTestHelpers.h"

#include "mocks\MockAudioSystem.hh"
#include "mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace mocks {

const std::wstring MockAudioSystem::BEEP = L"BEEP";

} // namespace mocks
} // namespace services
} // namespace ra

namespace ra {
namespace data {
namespace mocks {

gsl::span<uint8_t> MockEmulatorContext::s_pMemory;

} // namespace mocks
} // namespace data
} // namespace ra

void AssertContains(const std::string& sHaystack, const std::string& sNeedle)
{
    if (sHaystack.find(sNeedle) == std::string::npos)
    {
        const auto sError = ra::StringPrintf(L"\"%s\" not found in \"%s\"", ra::Widen(sNeedle), ra::Widen(sHaystack));
        Assert::Fail(sError.c_str());
    }
}
