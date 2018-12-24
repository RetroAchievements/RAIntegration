#include "RA_UnitTestHelpers.h"

#include "RA_MemManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static unsigned char* g_pMemoryBuffer;
static std::unique_ptr<unsigned char[]> g_pDynMemoryBuffer;
static size_t g_nMemorySize;

static constexpr unsigned char ReadMemory(unsigned int nAddress) noexcept
{
    if (nAddress <= g_nMemorySize)
       return g_pMemoryBuffer[nAddress];

    return unsigned char();
}

inline static auto ReadDynMemory(unsigned int nAddress)
{
    if (nAddress <= g_nMemorySize)
        return g_pDynMemoryBuffer[nAddress];

    return unsigned char();
}

static constexpr void SetMemory(unsigned int nAddress, unsigned int nValue) noexcept
{
    if (nAddress <= g_nMemorySize)
        g_pMemoryBuffer[nAddress] = gsl::narrow_cast<unsigned char>(nValue);
}

inline static void SetDynMemory(unsigned int nAddress, unsigned int nValue)
{
    if (nAddress <= g_nMemorySize)
        g_pDynMemoryBuffer[nAddress] = gsl::narrow<unsigned char>(nValue);
}

// TODO: restrict speeds up the tests significantly but there is pointer aliasing in some tests which makes it fail in release mode. We'll have to look into it later.
void InitializeMemory(unsigned char* const pMemory, size_t nMemorySize)
{
    g_pMemoryBuffer = pMemory;
    g_nMemorySize = nMemorySize;

    g_MemManager.ClearMemoryBanks();
    g_MemManager.AddMemoryBank(0, ReadMemory, SetMemory, nMemorySize);
}

void InitializeMemory(std::unique_ptr<unsigned char[]> pMemory, size_t nMemorySize)
{
    g_pDynMemoryBuffer = std::move(pMemory);
    g_nMemorySize = nMemorySize;

    g_MemManager.ClearMemoryBanks();
    g_MemManager.AddMemoryBank(0, ReadDynMemory, SetDynMemory, nMemorySize);
}

void AssertContains(const std::string& sHaystack, const std::string& sNeedle)
{
    if (sHaystack.find(sNeedle) == std::string::npos)
    {
        const auto sError = ra::StringPrintf(L"\"%s\" not found in \"%s\"", ra::Widen(sNeedle), ra::Widen(sHaystack));
        Assert::Fail(sError.c_str());
    }
}
