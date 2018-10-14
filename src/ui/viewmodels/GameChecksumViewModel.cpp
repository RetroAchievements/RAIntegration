#include "GameChecksumViewModel.hh"

#include "RA_Core.h"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty GameChecksumViewModel::ChecksumProperty("GameChecksumViewModel", "Checksum", L"No game loaded.");

GameChecksumViewModel::GameChecksumViewModel() noexcept
{
    SetChecksum(ra::Widen(g_sCurrentROMMD5));
}

void GameChecksumViewModel::CopyChecksumToClipboard() const
{
    auto& pClipboard = ra::services::ServiceLocator::Get<ra::services::IClipboard>();
    pClipboard.SetText(GetChecksum());
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
