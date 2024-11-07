#ifndef RA_DATA_CODE_NOTE_MODEL_H
#define RA_DATA_CODE_NOTE_MODEL_H
#pragma once

#include "data/Types.hh"

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
    const unsigned int GetBytes() const noexcept { return m_nBytes; }
    const MemSize GetMemSize() const noexcept { return m_nMemSize; }

    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }
    void SetNote(const std::wstring& sNote);

    bool IsPointer() const noexcept { return m_pPointerData != nullptr; }
    ra::ByteAddress GetPointerAddress() const noexcept;
    uint32_t GetRawPointerValue() const noexcept;
    bool SetRawPointerValue(uint32_t nValue);
    const CodeNoteModel* GetPointerNoteAtOffset(int nOffset) const;
    std::pair<ra::ByteAddress, const CodeNoteModel*> GetPointerNoteAtAddress(ra::ByteAddress nAddress) const;

    bool GetPreviousAddress(ra::ByteAddress nBeforeAddress, ra::ByteAddress& nPreviousAddress) const;
    bool GetNextAddress(ra::ByteAddress nAfterAddress, ra::ByteAddress& nNextAddress) const;

    void EnumeratePointerNotes(ra::ByteAddress nPointerAddress,
                               std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> callback) const;
    void EnumeratePointerNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> callback) const;

private:
    std::string m_sAuthor;
    std::wstring m_sNote;
    unsigned int m_nBytes = 1;
    MemSize m_nMemSize = MemSize::Unknown;

    struct PointerData;
    std::unique_ptr<PointerData> m_pPointerData;

private:
    void ProcessIndirectNotes(const std::wstring& sNote, size_t nIndex);
    void ExtractSize(const std::wstring& sNote);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_CODE_NOTE_MODEL_H
