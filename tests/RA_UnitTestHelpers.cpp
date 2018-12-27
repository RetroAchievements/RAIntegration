#include "RA_UnitTestHelpers.h"

#include "RA_MemManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static gsl::span<std::byte> g_pMemoryBuffer; // non-owning (stack)
static std::unique_ptr<unsigned char[]> g_pDynMemoryBuffer;
static unsigned int g_nMemorySize;

static constexpr auto ReadMemory(unsigned int nAddress)
{
    if (nAddress <= g_nMemorySize)
        return ra::etoi(g_pMemoryBuffer.at(nAddress));

    return unsigned char();
}

inline static auto ReadDynMemory(unsigned int nAddress)
{
    if (nAddress <= g_nMemorySize)
        return g_pDynMemoryBuffer[nAddress];

    return unsigned char();
}

static constexpr void SetMemory(unsigned int nAddress, unsigned int nValue)
{
    if (nAddress <= g_nMemorySize)
        g_pMemoryBuffer.at(nAddress) = ra::itoe<std::byte>(nValue);
}

inline static void SetDynMemory(unsigned int nAddress, unsigned int nValue)
{
    if (nAddress <= g_nMemorySize)
        g_pDynMemoryBuffer[nAddress] = gsl::narrow<unsigned char>(nValue);
}

void InitializeMemory(gsl::span<unsigned char> pMemory)
{
    g_pMemoryBuffer = gsl::as_writeable_bytes(pMemory);
    g_nMemorySize = gsl::narrow<unsigned int>(pMemory.size_bytes());

    g_MemManager.ClearMemoryBanks();
    g_MemManager.AddMemoryBank(0, ReadMemory, SetMemory, ra::to_unsigned(pMemory.size_bytes()));
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
