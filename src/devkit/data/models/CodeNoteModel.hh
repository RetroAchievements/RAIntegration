#ifndef RA_DATA_MODELS_CODENOTEMODEL_H
#define RA_DATA_MODELS_CODENOTEMODEL_H
#pragma once

#include "context/IEmulatorMemoryContext.hh"

namespace ra {
namespace data {
namespace models {

class CodeNoteModel
{
public:
	CodeNoteModel() noexcept;
	virtual ~CodeNoteModel();
	CodeNoteModel(const CodeNoteModel&) noexcept = delete;
    CodeNoteModel& operator=(const CodeNoteModel&) noexcept = delete;
    CodeNoteModel(CodeNoteModel&&) noexcept;
    CodeNoteModel& operator=(CodeNoteModel&&) noexcept;

    /// <summary>
    /// Gets the author of the note.
    /// </summary>
    const std::string& GetAuthor() const noexcept { return m_sAuthor; }

    /// <summary>
    /// Sets the author of the note.
    /// </summary>
    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }

    /// <summary>
    /// Gets the full note.
    /// </summary>
    const std::wstring& GetNote() const noexcept { return m_sNote; }

    /// <summary>
    /// Sets the full note.
    /// </summary>
    void SetNote(const std::wstring& sNote, bool bImpliedPointer = false);

    /// <summary>
    /// Gets the address/offset of the note.
    /// </summary>
    const ra::data::ByteAddress GetAddress() const noexcept { return m_nAddress; }

    /// <summary>
    /// Sets the address/offset of the note.
    /// </summary>
    void SetAddress(ra::data::ByteAddress nAddress) noexcept { m_nAddress = nAddress; }

    /// <summary>
    /// Gets the number of bytes that the note is associated to.
    /// </summary>
    const unsigned int GetBytes() const noexcept { return m_nBytes; }

    /// <summary>
    /// Gets the primary size of the note.
    /// </summary>
    const Memory::Size GetMemSize() const noexcept { return m_nMemSize; }

    /// <summary>
    /// Sets the primary size of the note.
    /// </summary>
    void SetMemSize(Memory::Size nMemSize) noexcept { m_nMemSize = nMemSize; }

    /// <summary>
    /// Gets whether values associated to the note should be shown in hexadecimal or decimal.
    /// </summary>
    const Memory::Format GetDefaultMemFormat() const noexcept { return m_nMemFormat; }

    /// <summary>
    /// Gets the non-pointed-at-data portion of the note.
    /// </summary>
    std::wstring GetPrimaryNote() const;


    /// <summary>
    /// Gets the sub-note text associated with the specified enum value.
    /// </summary>
    std::wstring_view GetEnumText(uint32_t nValue) const;


    /// <summary>
    /// Gets whether or not the note contains information about data the note is pointing at.
    /// </summary>
    bool IsPointer() const noexcept { return m_pPointerData != nullptr; }

    /// <summary>
    /// Gets the first line of the note.
    /// </summary>
    std::wstring GetPointerDescription() const;

    /// <summary>
    /// Gets whether or not the RawPointerValue has been set.
    /// </summary>
    bool HasRawPointerValue() const noexcept;

    /// <summary>
    /// Gets the raw pointer value.
    /// </summary>
    uint32_t GetRawPointerValue() const noexcept;

    typedef std::function<void(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const CodeNoteModel&)> NoteMovedFunction;
    /// <summary>
    /// Updates the raw pointer value by reading from memory.
    /// </summary>
    /// <param name="nAddress">The address of the pointer data. For root pointers, this will be the note's address. For nested pointers, it will be the note's offset + the parent pointer's value.</param>
    /// <param name="pMemoryContext">Where to read the new value from.</param>
    /// <param name="fNoteMovedCallback">Function to call if the PointerAddress changes.</param>
    void UpdateRawPointerValue(ra::data::ByteAddress nAddress, const ra::context::IEmulatorMemoryContext& pMemoryContext, NoteMovedFunction fNoteMovedCallback);

    /// <summary>
    /// Gets the last known value of the pointer.
    /// </summary>
    ra::data::ByteAddress GetPointerAddress() const noexcept;

    /// <summary>
    /// Gets whether or not the note has pointers in its field list.
    /// </summary>
    bool HasNestedPointers() const noexcept;

    /// <summary>
    /// Get the subnote for the field at the specified offset.
    /// </summary>
    /// <returns>Requested subnote, <c>null</c> if not found.</returns>
    const CodeNoteModel* GetPointerNoteAtOffset(int nOffset) const;

    /// <summary>
    /// Gets the subnote for the specified address.
    /// </summary>
    /// <returns>A pair representing the nested note and its actual address, or an empty pair if not found.</returns>
    std::pair<ra::data::ByteAddress, const CodeNoteModel*> GetPointerNoteAtAddress(ra::data::ByteAddress nAddress) const;

    /// <summary>
    /// Attempts to build a path from the provided root note to this note.
    /// </summary>
    /// <param name="vChain">If a path is found, it will be captured here (root first).</param>
    /// <param name="pRootNote">The root note that is suspected to contain this note.</param>
    /// <returns><c>true</c> if a path is found, <c>false</c> if not.</returns>
    virtual bool GetPointerChain(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pRootNote) const;

    /// <summary>
    /// Gets the address of the closest subnote before the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    bool GetPreviousAddress(ra::data::ByteAddress nBeforeAddress, ra::data::ByteAddress& nPreviousAddress) const;

    /// <summary>
    /// Gets the address of the closest subnote after the specified address.
    /// </summary>
    /// <returns>
    /// <c>true</c> if a subnote was found (<see cref="nPreviousAddress"/> will be set).
    /// <c>false</c> if not (<see cref="nPreviousAddress"/> may be uninitialized).
    /// </returns>
    bool GetNextAddress(ra::data::ByteAddress nAfterAddress, ra::data::ByteAddress& nNextAddress) const;

    /// <summary>
    /// Calls the provided callback for each subnote.
    /// </summary>
    void EnumeratePointerNotes(std::function<bool(ra::data::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const;

    /// <summary>
    /// Removes the size annotation from a note string.
    /// </summary>
    /// <param name="sNote">The note string to process.</param>
    /// <param name="bKeepPointer"><c>true</c> to prefix the result with '[pointer]' if a pointer annotation was seen.</param>
    static std::wstring TrimSize(const std::wstring& sNote, bool bKeepPointer);

protected:
    class Parser
    {
    public:
        Parser(const std::wstring& sNote, size_t nStartIndex, size_t nEndIndex) noexcept :
            m_sNote(sNote), m_nIndex(nStartIndex), m_nEndIndex(nEndIndex)
        {
        }

        enum TokenType
        {
            None = 0,
            Number,
            Bits,
            Bytes,
            Float,
            Double,
            MBF,
            BigEndian,
            LittleEndian,
            BCD,
            Hex,
            ASCII,
            HexNumber,
            Other,
        };

        TokenType NextToken(std::wstring& sWord) const;
        wchar_t Peek() const { return (m_nIndex < m_nEndIndex) ? m_sNote.at(m_nIndex) : 0; }

    private:
        const std::wstring& m_sNote;
        mutable size_t m_nIndex;
        size_t m_nEndIndex;
    };

private:
    std::string m_sAuthor; // TODO: make this a reference to data stored in the CodeNotesModel.
    std::wstring m_sNote;
    ra::data::ByteAddress m_nAddress = 0; // address of root nodes, offset to indirect nodes
    unsigned int m_nBytes = 1;
    Memory::Size m_nMemSize = Memory::Size::Unknown;
    Memory::Format m_nMemFormat = Memory::Format::Dec;

    enum EnumState {
        None,
        Hex,
        Dec,
        Unknown,
    };
    mutable EnumState m_nEnumState = EnumState::Unknown;
    static EnumState DetermineEnumState(const std::wstring_view svNote);

    struct PointerData;
    std::unique_ptr<PointerData> m_pPointerData;

    bool GetPointerChainRecursive(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pParentNote) const;

    void EnumeratePointerNotes(ra::data::ByteAddress nPointerAddress,
        std::function<bool(ra::data::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const;

    void ProcessIndirectNotes(const std::wstring& sNote, size_t nIndex);
    void ExtractSize(const std::wstring& sNote, bool bIsPointer);
    static Memory::Size GetImpliedPointerSize();
    void CheckForHexEnum(size_t nStartIndex);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_CODENOTEMODEL_H
