#include "MockEmulatorMemoryContext.hh"

#include "context/IRcClient.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace context {
namespace mocks {

void MockEmulatorMemoryContext::MockMemory(uint8_t pMemory[], size_t nBytes)
{
    m_pMemory = pMemory;
    m_mMemoryValues.clear();

    ClearMemoryBlocks();
    AddMemoryBlock(0, nBytes, ReadMemoryHelper, WriteMemoryHelper);

    if (ra::services::ServiceLocator::Exists<ra::context::IRcClient>())
    {
        auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
        auto* pGame = pClient->game;
        if (pGame && pGame->max_valid_address == 0U)
            pGame->max_valid_address = gsl::narrow_cast<uint32_t>(nBytes);
    }
}

void MockEmulatorMemoryContext::MockTotalMemorySizeChanged(size_t nNewSize)
{
    m_nTotalMemorySize = nNewSize;

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnTotalMemorySizeChanged();

        m_vNotifyTargets.Unlock();
    }
}

uint8_t MockEmulatorMemoryContext::ReadMemoryHelper(uint32_t nAddress)
{
    const auto* mockEmulator =
        dynamic_cast<const MockEmulatorMemoryContext*>(&ra::services::ServiceLocator::Get<IEmulatorMemoryContext>());

    if (!mockEmulator)
        return 0;

    if (!mockEmulator->m_mMemoryValues.empty())
    {
        auto iter = mockEmulator->m_mMemoryValues.lower_bound(nAddress);
        if (iter != mockEmulator->m_mMemoryValues.end() && nAddress == iter->first)
            return iter->second & 0xFF;

        if (iter != mockEmulator->m_mMemoryValues.begin())
        {
            --iter;
            if (nAddress - iter->first < 4)
            {
                uint32_t nValue = iter->second;
                while (nAddress > iter->first)
                {
                    nValue >>= 8;
                    nAddress--;
                }

                return nValue & 0xFF;
            }
        }

        return 0;
    }

    if (nAddress >= mockEmulator->m_nTotalMemorySize)
        return 0;

    return mockEmulator->m_pMemory[nAddress];
}

void MockEmulatorMemoryContext::WriteMemoryHelper(uint32_t nAddress, uint8_t nValue)
{
    const auto* mockEmulator =
        dynamic_cast<MockEmulatorMemoryContext*>(&ra::services::ServiceLocator::GetMutable<IEmulatorMemoryContext>());

    if (!mockEmulator)
        return;

    if (nAddress < mockEmulator->m_nTotalMemorySize)
        mockEmulator->m_pMemory[nAddress] = nValue;
}

} // namespace mocks
} // namespace context
} // namespace ra
