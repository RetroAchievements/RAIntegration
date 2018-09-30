#include "RA_UnitTestHelpers.h"

#include "RA_MemManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static unsigned char* g_pMemoryBuffer;
static size_t g_nMemorySize;

static unsigned char ReadMemory(unsigned int nAddress)
{
    if (nAddress <= g_nMemorySize)
        return g_pMemoryBuffer[nAddress];

    return '\0';
}

static void SetMemory(unsigned int nAddress, unsigned int nValue)
{
    if (nAddress <= g_nMemorySize)
        g_pMemoryBuffer[nAddress] = static_cast<unsigned char>(nValue);
}

void InitializeMemory(unsigned char* pMemory, size_t nMemorySize)
{
    g_pMemoryBuffer = pMemory;
    g_nMemorySize = nMemorySize;

    g_MemManager.ClearMemoryBanks();
    g_MemManager.AddMemoryBank(0, ReadMemory, SetMemory, nMemorySize);
}

void AssertContains(const std::string& sHaystack, const std::string& sNeedle)
{
    if (sHaystack.find(sNeedle) == std::string::npos)
    {
        std::wstring sError = L"\"" + ra::Widen(sNeedle) + L"\" not found in \"" + ra::Widen(sHaystack) + L"\"";
        Assert::Fail(sError.c_str());
    }
}