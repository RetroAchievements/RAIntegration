#include "RA_CodeNotes.h"

#include "RA_AchievementSet.h" // RA_Achievement
#include "RA_Core.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "api\DeleteCodeNote.hh"
#include "api\FetchCodeNotes.hh"
#include "api\UpdateCodeNote.hh"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\IAudioSystem.hh"
#include "services\ILocalStorage.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

void CodeNotes::Clear() noexcept { m_CodeNotes.clear(); }

size_t CodeNotes::Load(unsigned int nID)
{
    return m_CodeNotes.size();
}

BOOL CodeNotes::ReloadFromWeb(unsigned int nID)
{
    m_CodeNotes.clear();

    if (nID == 0)
        return FALSE;

    ra::api::FetchCodeNotes::Request request;
    request.GameId = nID;
    request.CallAsync([this](const ra::api::FetchCodeNotes::Response& response)
    {
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download code notes",
                ra::Widen(response.ErrorMessage));
            return;
        }

        for (const auto& pNote : response.Notes)
            m_CodeNotes.try_emplace(pNote.Address, CodeNoteObj(pNote.Author, pNote.Note));

        g_MemoryDialog.RepopulateMemNotesFromFile();
    });

    return TRUE;
}

bool CodeNotes::Add(const ra::ByteAddress& nAddr, const std::string& sAuthor, const std::wstring& sNote)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    ra::api::UpdateCodeNote::Request request;
    request.GameId = pGameContext.GameId();
    request.Address = nAddr;
    request.Note = sNote;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
                m_CodeNotes.try_emplace(nAddr, CodeNoteObj(sAuthor, sNote));
            else
                m_CodeNotes.at(nAddr).SetNote(sNote);

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error saving note for address %s", ra::ByteAddressToString(nAddr)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}

bool CodeNotes::Remove(const ra::ByteAddress& nAddr)
{
    if (m_CodeNotes.find(nAddr) == m_CodeNotes.end())
        return true;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    ra::api::DeleteCodeNote::Request request;
    request.GameId = pGameContext.GameId();
    request.Address = nAddr;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            m_CodeNotes.erase(nAddr);

            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error deleting note for address %s", ra::ByteAddressToString(nAddr)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}
