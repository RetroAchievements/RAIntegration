#include "GameChecksumViewModel.hh"

#include "data\GameContext.hh"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty GameChecksumViewModel::ChecksumProperty("GameChecksumViewModel", "Checksum", L"No game loaded.");

GameChecksumViewModel::GameChecksumViewModel() noexcept
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    SetChecksum(ra::Widen(pGameContext.GameHash()));
}

void GameChecksumViewModel::CopyChecksumToClipboard() const
{
    auto& pClipboard = ra::services::ServiceLocator::Get<ra::services::IClipboard>();
    pClipboard.SetText(GetChecksum());
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
