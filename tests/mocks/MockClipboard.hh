#ifndef RA_SERVICES_MOCK_CLIPBOARD_HH
#define RA_SERVICES_MOCK_CLIPBOARD_HH
#pragma once

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockClipboard : public ra::services::IClipboard
{
public:
    MockClipboard() noexcept
        : m_Override(this)
    {
    }

    void SetText(const std::wstring& sValue) const override { m_sText = sValue; }
    const std::wstring& GetText() const noexcept { return m_sText; }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IClipboard> m_Override;
    mutable std::wstring m_sText;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_CLIPBOARD_HH
