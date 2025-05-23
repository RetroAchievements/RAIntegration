#include "RA_UnitTestHelpers.h"

#include "mocks\MockAudioSystem.hh"
#include "mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {

bool ServiceLocator::IsInitialized() noexcept { return true; }

namespace mocks {

const std::wstring MockAudioSystem::BEEP = L"BEEP";

} // namespace mocks
} // namespace services
} // namespace ra

void AssertContains(const std::string& sHaystack, const std::string& sNeedle)
{
    if (sHaystack.find(sNeedle) == std::string::npos)
    {
        const auto sError = ra::StringPrintf(L"\"%s\" not found in \"%s\"", ra::Widen(sNeedle), ra::Widen(sHaystack));
        Assert::Fail(sError.c_str());
    }
}

void AssertDoesNotContain(const std::string& sHaystack, const std::string& sNeedle)
{
    if (sHaystack.find(sNeedle) != std::string::npos)
    {
        const auto sError = ra::StringPrintf(L"\"%s\" found in \"%s\"", ra::Widen(sNeedle), ra::Widen(sHaystack));
        Assert::Fail(sError.c_str());
    }
}
