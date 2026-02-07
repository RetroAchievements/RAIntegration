#include "GameChecksumViewModel.hh"

#include "util\Strings.hh"

#include "data\context\GameContext.hh"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty GameChecksumViewModel::ChecksumProperty("GameChecksumViewModel", "Checksum", L"No game loaded.");

GameChecksumViewModel::GameChecksumViewModel() noexcept
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto& sGameHash = pGameContext.GameHash();
    if (!sGameHash.empty())
        SetChecksum(ra::util::String::Widen(pGameContext.GameHash()));
}

void GameChecksumViewModel::CopyChecksumToClipboard() const
{
    const auto& pClipboard = ra::services::ServiceLocator::Get<ra::services::IClipboard>();
    pClipboard.SetText(GetChecksum());
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
