#ifndef RA_SERVICES_MESSAGE_DISPATCHER_HH
#define RA_SERVICES_MESSAGE_DISPATCHER_HH
#pragma once

#include "services\IMessageDispatcher.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace services {
namespace impl {

class MessageDispatcher : public IMessageDispatcher
{
public:
    /// <summary>
    /// Dispatches a message.
    /// </summary>
    void ReportMessage(const std::wstring& sSummary, const std::wstring& sDetail) const override
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowMessage(sSummary, sDetail);
    }

    /// <summary>
    /// Dispatches an error message.
    /// </summary>
    void ReportErrorMessage(const std::wstring& sSummary, const std::wstring& sDetail) const override
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(sSummary, sDetail);
    }

};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MESSAGE_DISPATCHER_HH
