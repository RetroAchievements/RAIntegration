#ifndef RA_UI_UNKNOWNGAMEVIEWMODEL_H
#define RA_UI_UNKNOWNGAMEVIEWMODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class UnknownGameViewModel : public WindowViewModelBase, protected ViewModelBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 UnknownGameViewModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected game.
    /// </summary>
    static const IntModelProperty SelectedGameIdProperty;

    /// <summary>
    /// Gets the selected game ID.
    /// </summary>
    int GetSelectedGameId() const { return GetValue(SelectedGameIdProperty); }

    /// <summary>
    /// Sets the selected game ID.
    /// </summary>
    void SetSelectedGameId(int sValue) { SetValue(SelectedGameIdProperty, sValue); }
    
    /// <summary>
    /// Gets the list of known game titles and their IDs
    /// </summary>
    LookupItemViewModelCollection& GameTitles() noexcept
    {
        return m_vGameTitles;
    }
    
    /// <summary>
    /// Initializes the <see cref="GameTitles"/> collection (asynchronously).
    /// </summary>
    void InitializeGameTitles();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the new game name.
    /// </summary>
    static const StringModelProperty NewGameNameProperty;

    /// <summary>
    /// Gets the new game name.
    /// </summary>
    const std::wstring& GetNewGameName() const { return GetValue(NewGameNameProperty); }

    /// <summary>
    /// Sets the new game name.
    /// </summary>
    void SetNewGameName(const std::wstring& sValue) { SetValue(NewGameNameProperty, sValue); }

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

    /// <summary>
    /// The <see cref="ModelProperty" /> for the game name provided by the emulator.
    /// </summary>
    static const StringModelProperty EstimatedGameNameProperty;

    /// <summary>
    /// Gets the game name provided by the emulator.
    /// </summary>
    const std::wstring& GetEstimatedGameName() const { return GetValue(EstimatedGameNameProperty); }

    /// <summary>
    /// Sets the game name provided by the emulator.
    /// </summary>
    void SetEstimatedGameName(const std::wstring& sValue) { SetValue(EstimatedGameNameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the system name.
    /// </summary>
    static const StringModelProperty SystemNameProperty;

    /// <summary>
    /// Gets the system name.
    /// </summary>
    const std::wstring& GetSystemName() const { return GetValue(SystemNameProperty); }

    /// <summary>
    /// Sets the system name.
    /// </summary>
    void SetSystemName(const std::wstring& sValue) { SetValue(SystemNameProperty, sValue); }

    /// <summary>
    /// Command handler for Associate button.
    /// </summary>    
    /// <returns>
    /// <c>true</c> if the association was successfully created and the dialog should be closed, 
    /// <c>false</c> if not.
    /// </returns>
    bool Associate();

    /// <summary>
    /// Command handler for Test button.
    /// </summary>
    /// <returns><c>true</c> if the dialog should be closed, <c>false</c> if not.</returns>
    bool BeginTest();

protected:
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    LookupItemViewModelCollection m_vGameTitles;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_UNKNOWNGAMEVIEWMODEL_H
