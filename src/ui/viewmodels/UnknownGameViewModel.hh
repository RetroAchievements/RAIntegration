#ifndef RA_UI_UNKNOWNGAMEVIEWMODEL_H
#define RA_UI_UNKNOWNGAMEVIEWMODEL_H
#pragma once

#include "RAInterface\RA_Consoles.h"

#include "data\AsyncObject.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class UnknownGameViewModel : public WindowViewModelBase, protected ra::data::AsyncObject
{
public:
    GSL_SUPPRESS_F6 UnknownGameViewModel() noexcept;
    ~UnknownGameViewModel()
    {
        ra::data::AsyncObject::BeginDestruction();
    }

    UnknownGameViewModel(const UnknownGameViewModel&) noexcept = delete;
    UnknownGameViewModel& operator=(const UnknownGameViewModel&) noexcept = delete;
    UnknownGameViewModel(UnknownGameViewModel&&) noexcept = delete;
    UnknownGameViewModel& operator=(UnknownGameViewModel&&) noexcept = delete;

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
    void SetSelectedGameId(int nValue) { SetValue(SelectedGameIdProperty, nValue); }
    
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
    /// Initializes the <see cref="GameTitles"/> collection (asynchronously).
    /// </summary>
    void InitializeGameTitles(ConsoleID nConsoleId);

    /// <summary>
    /// Freezes the selected game.
    /// </summary>
    void InitializeTestCompatibilityMode();

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not a game can be selected.
    /// </summary>
    static const BoolModelProperty IsSelectedGameEnabledProperty;

    /// <summary>
    /// Gets whether or not a game can be selected.
    /// </summary>
    bool IsSelectedGameEnabled() const { return GetValue(IsSelectedGameEnabledProperty); }

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
    /// The <see cref="ModelProperty" /> for the problem header.
    /// </summary>
    static const StringModelProperty ProblemHeaderProperty;

    /// <summary>
    /// Gets the problem header.
    /// </summary>
    const std::wstring& GetProblemHeader() const { return GetValue(ProblemHeaderProperty); }

    /// <summary>
    /// Sets the problem header.
    /// </summary>
    void SetProblemHeader(const std::wstring& sValue) { SetValue(ProblemHeaderProperty, sValue); }

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
    /// The <see cref="ModelProperty" /> for the filter text.
    /// </summary>
    static const StringModelProperty FilterTextProperty;

    /// <summary>
    /// Gets the filter text.
    /// </summary>
    const std::wstring& GetFilterText() const { return GetValue(FilterTextProperty); }

    /// <summary>
    /// Sets the filter text.
    /// </summary>
    void SetFilterText(const std::wstring& sValue) { SetValue(FilterTextProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter results.
    /// </summary>
    static const StringModelProperty FilterResultsProperty;

    /// <summary>
    /// Gets the filter results.
    /// </summary>
    const std::wstring& GetFilterResults() const { return GetValue(FilterResultsProperty); }

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
    /// The <see cref="ModelProperty" /> for the whether test mode is selected.
    /// </summary>
    static const BoolModelProperty TestModeProperty;

    /// <summary>
    /// Gets whether test mode is selected.
    /// </summary>
    bool GetTestMode() const { return GetValue(TestModeProperty); }

    /// <summary>
    /// Sets whether test mode is selected.
    /// </summary>
    void SetTestMode(bool bValue) { SetValue(TestModeProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the link button is enabled.
    /// </summary>
    static const BoolModelProperty IsAssociateEnabledProperty;

    /// <summary>
    /// Gets whether or not the link button is enabled.
    /// </summary>
    bool IsAssociateEnabled() const { return GetValue(IsAssociateEnabledProperty); }

    /// <summary>
    /// Command handler for Associate button.
    /// </summary>    
    void Associate();

    /// <summary>
    /// Command handler for Test button.
    /// </summary>
    void BeginTest();

    static unsigned int GetPreviousAssociation(const std::wstring& sHash);

protected:
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    void CheckForPreviousAssociation();
    void ApplyFilter();

    static unsigned DecodeID(const std::string& sEncoded, const std::wstring& sHash, ConsoleID nConsoleId);
    static std::string EncodeID(unsigned nId, const std::wstring& sHash, ConsoleID nConsoleId);

    LookupItemViewModelCollection m_vGameTitles;
    bool m_bSelectingGame = false;

    std::vector<std::pair<uint32_t, std::wstring>> m_vAllGameTitles;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_UNKNOWNGAMEVIEWMODEL_H
