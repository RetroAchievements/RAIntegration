#include "EmulatorViewModel.hh"

#include "data/context/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void EmulatorViewModel::UpdateWindowTitle()
{
    if (!m_bAppTitleManaged)
        return;

    auto sTitle = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().GetAppTitle(m_sAppTitleMessage);
    SetWindowTitle(sTitle);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
