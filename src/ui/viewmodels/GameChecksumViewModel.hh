#ifndef RA_UI_GAMECHECKSUMVIEWMODEL_H
#define RA_UI_GAMECHECKSUMVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class GameChecksumViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 GameChecksumViewModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the message.
    /// </summary>
    static const StringModelProperty ChecksumProperty;

    /// <summary>
    /// Gets the checksum.
    /// </summary>
    const std::wstring& GetChecksum() const { return GetValue(ChecksumProperty); }

    /// <summary>
    /// Sets the checksum.
    /// </summary>
    void SetChecksum(const std::wstring& sValue) { SetValue(ChecksumProperty, sValue); }
    
    /// <summary>
    /// Copies the checksum to the clipboard.
    /// </summary>
    void CopyChecksumToClipboard() const;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_GAMECHECKSUMVIEWMODEL_H
