#ifndef RA_DATA_MODELS_AUTHOREDMEMORYNOTEMODEL_H
#define RA_DATA_MODELS_AUTHOREDMEMORYNOTEMODEL_H
#pragma once

#include "MemoryNoteModel.hh"

namespace ra {
namespace data {
namespace models {

class AuthoredMemoryNoteModel
{
public:
    /// <summary>
    /// Gets the author of the note.
    /// </summary>
    const std::string& GetAuthor() const noexcept { return m_sAuthor; }

    /// <summary>
    /// Sets the author of the note.
    /// </summary>
    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }

protected:
    AuthoredMemoryNoteModel(const std::wstring& sNote)
        : m_sNote(sNote)
    {
    }

    std::wstring m_sNote;
    std::string m_sAuthor;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_AUTHOREDMEMORYNOTEMODEL_H
