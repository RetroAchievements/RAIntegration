#ifndef RA_DATA_CODE_NOTE_MODEL_H
#define RA_DATA_CODE_NOTE_MODEL_H
#pragma once

#include "data/Types.hh"

#include "data/context/EmulatorContext.hh"

namespace ra {
namespace data {
namespace models {

class CodeNoteModel
{
public:
	CodeNoteModel() noexcept;
	~CodeNoteModel();
	CodeNoteModel(const CodeNoteModel&) noexcept = delete;
    CodeNoteModel& operator=(const CodeNoteModel&) noexcept = delete;
    CodeNoteModel(CodeNoteModel&&) noexcept;
    CodeNoteModel& operator=(CodeNoteModel&&) noexcept;

    const std::string& GetAuthor() const noexcept { return m_sAuthor; }
    const std::wstring& GetNote() const noexcept { return m_sNote; }
    const ra::ByteAddress GetAddress() const noexcept { return m_nAddress; }
    const unsigned int GetBytes() const noexcept { return m_nBytes; }
    const MemSize GetMemSize() const noexcept { return m_nMemSize; }

    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }
    void SetAddress(ra::ByteAddress nAddress) noexcept { m_nAddress = nAddress; }
    void SetNote(const std::wstring& sNote);

    bool IsPointer() const noexcept { return m_pPointerData != nullptr; }
    std::wstring GetPointerDescription() const;
    ra::ByteAddress GetPointerAddress() const noexcept;
    uint32_t GetRawPointerValue() const noexcept;
    const CodeNoteModel* GetPointerNoteAtOffset(int nOffset) const;
    const CodeNoteModel* GetPointerNoteAtAddress(ra::ByteAddress nAddress) const;

    bool GetPointerChain(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pRootNote) const;

    typedef std::function<void(ra::ByteAddress nOldAddress, ra::ByteAddress nNewAddress, const CodeNoteModel&)> NoteMovedFunction;
    void UpdateRawPointerValue(ra::ByteAddress nAddress, const ra::data::context::EmulatorContext& pEmulatorContext, NoteMovedFunction fNoteMovedCallback);

    bool GetPreviousAddress(ra::ByteAddress nBeforeAddress, ra::ByteAddress& nPreviousAddress) const;
    bool GetNextAddress(ra::ByteAddress nAfterAddress, ra::ByteAddress& nNextAddress) const;

    void EnumeratePointerNotes(ra::ByteAddress nPointerAddress,
                               std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const;
    void EnumeratePointerNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const;

private:
    std::string m_sAuthor;
    std::wstring m_sNote;
    ra::ByteAddress m_nAddress = 0; // address of root nodes, offset to indirect nodes
    unsigned int m_nBytes = 1;
    MemSize m_nMemSize = MemSize::Unknown;

    struct PointerData;
    std::unique_ptr<PointerData> m_pPointerData;

private:
    bool GetPointerChainRecursive(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pParentNote) const;

    void ProcessIndirectNotes(const std::wstring& sNote, size_t nIndex);
    void ExtractSize(const std::wstring& sNote);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_CODE_NOTE_MODEL_H
