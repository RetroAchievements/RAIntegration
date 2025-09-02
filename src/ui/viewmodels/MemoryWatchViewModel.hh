#ifndef RA_UI_MEMORYWATCHVIEWMODEL_H
#define RA_UI_MEMORYWATCHVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"

#include "ui\Types.hh"
#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryWatchViewModel : public LookupItemViewModel,
    protected ra::data::context::EmulatorContext::DispatchesReadMemory
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for description of the watched memory.
    /// </summary>
    static const StringModelProperty DescriptionProperty;

    /// <summary>
    /// Gets the description of the watched memory.
    /// </summary>
    const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

    /// <summary>
    /// Sets the description of the watched memory.
    /// </summary>
    void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether the description differs from the note at the watched address.
    /// </summary>
    static const BoolModelProperty IsCustomDescriptionProperty;

    /// <summary>
    /// Gets whether the description differs from the note at the watched address.
    /// </summary>
    bool IsCustomDescription() const { return GetValue(IsCustomDescriptionProperty); }

    /// <summary>
    /// Sets whether the description differs from the note at the watched address.
    /// </summary>
    void SetIsCustomDescription(bool bValue) { SetValue(IsCustomDescriptionProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the note at the watched address.
    /// </summary>
    static const StringModelProperty RealNoteProperty;

    /// <summary>
    /// Gets the note at the watched address.
    /// </summary>
    const std::wstring& GetRealNote() const { return GetValue(RealNoteProperty); }

    /// <summary>
    /// Sets the note at the watched address.
    /// </summary>
    void SetRealNote(const std::wstring& sValue) { SetValue(RealNoteProperty, sValue); }

    /// <summary>
    /// Sets the <see cref="RealNoteProperty" /> based on the <see cref="CurrentAddressProperty" />.
    /// </summary>
    void UpdateRealNote();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the watched address.
    /// </summary>
    static const IntModelProperty AddressProperty;

    /// <summary>
    /// Gets the watched address.
    /// </summary>
    ByteAddress GetAddress() const noexcept { return m_nAddress; }

    /// <summary>
    /// Sets the watched address.
    /// </summary>
    void SetAddress(ByteAddress value) { SetValue(AddressProperty, value); }

    /// <summary>
    /// Sets a serialized definition for an indirect watched address.
    /// </summary>
    void SetIndirectAddress(const std::string& sSerialized);

    /// <summary>
    /// Gets the serialized definition of the indirect watched address.
    /// </summary>
    /// <returns></returns>
    const std::string& GetIndirectAddress() const noexcept { return m_sIndirectAddress; }

    /// <summary>
    /// Gets whether or not the watched address is indirect.
    /// </summary>
    bool IsIndirectAddress() const noexcept { return !m_sIndirectAddress.empty(); }

    /// <summary>
    /// Gets whether or not the indirect address chain of the watched memory contains valid pointers.
    /// </summary>
    bool IsIndirectAddressChainValid() const noexcept { return m_bIndirectAddressValid; }

    /// <summary>
    /// Updates the current address from the indirect address chain.
    /// </summary>
    /// <returns><c>true</c> if the indirect address chain was valid, <c>false</c> if not.</returns>
    /// <remarks>Address will be updated to whatever the chain evaluates to, regardless of the chain's validity</remarks>
    bool UpdateCurrentAddressFromIndirectAddress();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the watched memory size.
    /// </summary>
    static const IntModelProperty SizeProperty;

    /// <summary>
    /// Gets the watched memory size.
    /// </summary>
    MemSize GetSize() const noexcept { return m_nSize; }

    /// <summary>
    /// Sets the watched memory size.
    /// </summary>
    void SetSize(MemSize value) { SetValue(SizeProperty, ra::etoi(value)); }

    /// <summary>
    /// Gets the number of bytes being watched.
    /// </summary>
    uint32_t GetSizeBytes() const noexcept
    {
        return (m_nSize == MemSize::Text) ? MaxTextBookmarkLength : ra::data::MemSizeBytes(m_nSize);
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the watched memory format.
    /// </summary>
    static const IntModelProperty FormatProperty;

    /// <summary>
    /// Gets the watched memory format.
    /// </summary>
    MemFormat GetFormat() const { return ra::itoe<MemFormat>(GetValue(FormatProperty)); }

    /// <summary>
    /// Sets the watched memory format.
    /// </summary>
    void SetFormat(MemFormat value) { SetValue(FormatProperty, ra::etoi(value)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current value of the watched memory.
    /// </summary>
    static const StringModelProperty CurrentValueProperty;

    /// <summary>
    /// Gets the current value of the watched memory. 
    /// </summary>
    const std::wstring& GetCurrentValue() const { return GetValue(CurrentValueProperty); }

    /// <summary>
    /// Sets the current value of the watched memory.
    /// </summary>
    bool SetCurrentValue(const std::wstring& sValue, _Out_ std::wstring& sError);

    /// <summary>
    /// Gets the unformatted current value of the watched memory.
    /// </summary>
    uint32_t GetCurrentValueRaw() const noexcept { return m_nValue; }

    /// <summary>
    /// Sets the unformatted current value of the watched memory.
    /// </summary>
    /// <remarks>Bypasses behavioral triggers.</remarks>
    void SetCurrentValueRaw(uint32_t nValue);

    /// <summary>
    /// Reads the watched memory and updates the <see cref="CurrentValueProperty" />.
    /// </summary>
    /// <remarks>
    /// Should only be called in response to the watched memory being changed by the user.
    /// Bypasses behavioral triggers.
    /// </remarks>
    void UpdateCurrentValue();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the previous value of the watched memory.
    /// </summary>
    static const StringModelProperty PreviousValueProperty;

    /// <summary>
    /// Gets the previous value of the watched memory.
    /// </summary>
    const std::wstring& GetPreviousValue() const { return GetValue(PreviousValueProperty); }

    /// <summary>
    /// Sets the previous value of the watched memory.
    /// </summary>
    void SetPreviousValue(const std::wstring& sValue) { SetValue(PreviousValueProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the number of times the value of the watched memory has changed.
    /// </summary>
    static const IntModelProperty ChangesProperty;

    /// <summary>
    /// Gets the number of times the value of the watched memory has changed.
    /// </summary>
    unsigned int GetChanges() const { return gsl::narrow_cast<unsigned int>(GetValue(ChangesProperty)); }

    /// <summary>
    /// Sets the number of times the value of the watched memory has changed.
    /// </summary>
    void SetChanges(unsigned int value) { SetValue(ChangesProperty, value); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the <see cref="CurrentValueProperty"/> is read only.
    /// </summary>
    static const BoolModelProperty ReadOnlyProperty;

    /// <summary>
    /// Gets whether or not the <see cref="CurrentValueProperty"/> is read only. 
    /// </summary>
    /// <remarks>Determined by the <see cref="SizeProperty"/>.
    bool IsReadOnly() const { return GetValue(ReadOnlyProperty); }

    /// <summary>
    /// Sets whether or not the <see cref="CurrentValueProperty"/> is read only.
    /// </summary>
    void SetReadOnly(bool bValue) { SetValue(ReadOnlyProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the row color.
    /// </summary>
    static const IntModelProperty RowColorProperty;

    /// <summary>
    /// Gets the row color.
    /// </summary>
    Color GetRowColor() const { return Color(ra::to_unsigned(GetValue(RowColorProperty))); }

    /// <summary>
    /// Sets the row color.
    /// </summary>
    void SetRowColor(Color value) { SetValue(RowColorProperty, ra::to_signed(value.ARGB)); }

    /// <summary>
    /// Gets whether or not the attributes of the watched memory (address/size/format) have been
    /// modified since the last call to <see cref="ResetModified"/>.
    /// </summary>
    bool IsModified() const noexcept { return m_bModified; }

    /// <summary>
    /// Clears the modified flag.
    /// </summary>
    void ResetModified() noexcept { m_bModified = false; }

    /// <summary>
    /// Reads the watched memory and returns whether or not it has changed since the last time it was read.
    /// </summary>
    /// <returns><c>true</c> if the memory has changed, <c>false</c> if not.</returns>
    bool DoFrame();

    /// <summary>
    /// Starts initialization of the view model.
    /// </summary>
    /// <remarks>Prevents event dispatching while initializing the view model.</remarks>
    void BeginInitialization() noexcept { m_bInitialized = false; }

    /// <summary>
    /// Finishes initialization of the view model.
    /// </summary>
    void EndInitialization();

    static constexpr int MaxTextBookmarkLength = 8;

    static const BoolModelProperty IsWritingMemoryProperty;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    void OnValueChanged();
    void OnSizeChanged();

    uint32_t ReadValue();

    void SetAddressWithoutUpdatingValue(ra::ByteAddress nNewAddress);

    virtual bool IgnoreValueChange(uint32_t) noexcept(false) { return false; }
    virtual bool ChangeValue(uint32_t nNewValue);

private:
    std::wstring BuildCurrentValue() const;
    static std::wstring ExtractDescriptionHeader(const std::wstring& sFullNote);

    // keep address/size/value fields directly accessible for speed - also keep in ValueProperty for binding
    ra::ByteAddress m_nAddress = 0;
    uint32_t m_nValue = 0;
    MemSize m_nSize = MemSize::EightBit;
    bool m_bModified = false;
    bool m_bInitialized = false;
    bool m_bSyncingDescriptionHeader = false;
    bool m_bIndirectAddressValid = false;

    std::string m_sIndirectAddress;
    std::unique_ptr<uint8_t[]> m_pBuffer;
    rc_value_t* m_pValue = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYWATCHVIEWMODEL_H
