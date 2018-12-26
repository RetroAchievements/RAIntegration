#ifndef RA_DLG_MEMBOOKMARK_H
#define RA_DLG_MEMBOOKMARK_H
#pragma once

#include "ra_fwd.h"

class MemBookmark
{
public:
    void SetDescription(const std::wstring& string) { m_sDescription = string; }
    void SetAddress(unsigned int nVal) noexcept { m_nAddress = nVal; }
    void SetType(unsigned int nVal) noexcept { m_nType = nVal; }
    void SetValue(unsigned int nVal) noexcept { m_sValue = nVal; }
    void SetPrevious(unsigned int nVal) noexcept { m_sPrevious = nVal; }
    void IncreaseCount() noexcept { m_nCount++; }
    void ResetCount() noexcept { m_nCount = 0; }

    void SetFrozen(bool b) noexcept { m_bFrozen = b; }
    void SetDecimal(bool b) noexcept { m_bDecimal = b; }

    inline const std::wstring& Description() const noexcept { return m_sDescription; }
    unsigned int Address() const noexcept { return m_nAddress; }
    unsigned int Type() const noexcept { return m_nType; }
    unsigned int Value() const noexcept { return m_sValue; }
    unsigned int Previous() const noexcept { return m_sPrevious; }
    unsigned int Count() const noexcept { return m_nCount; }

    bool Frozen() const noexcept { return m_bFrozen; }
    bool Decimal() const noexcept { return m_bDecimal; }

private:
    std::wstring m_sDescription;
    unsigned int m_nAddress  = 0U;
    unsigned int m_nType     = 0U;
    unsigned int m_sValue    = 0U;
    unsigned int m_sPrevious = 0U;
    unsigned int m_nCount    = 0U;
    bool         m_bFrozen   = false;
    bool         m_bDecimal  = false;
};

class Dlg_MemBookmark
{
    enum class SubItems
    {
        Desc,
        Address,
        Value,
        Previous,
        Changes
    };

public:

    static INT_PTR CALLBACK s_MemBookmarkDialogProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR MemBookmarkDialogProc(HWND, UINT, WPARAM, LPARAM);

    void InstallHWND(HWND hWnd) noexcept { m_hMemBookmarkDialog = hWnd; }
    HWND GetHWND() const noexcept { return m_hMemBookmarkDialog; }
    BOOL IsActive() const noexcept;

    std::vector<MemBookmark*> Bookmarks() { return m_vBookmarks; }
    void UpdateBookmarks(bool bForceWrite);
    void AddBookmark(MemBookmark* newBookmark) { m_vBookmarks.push_back(newBookmark); }
    void WriteFrozenValue(const MemBookmark& Bookmark);

    void OnLoad_NewRom();

private:
    int m_nNumOccupiedRows;

    void PopulateList();
    void SetupColumns(HWND hList);
    BOOL EditLabel(int nItem, int nSubItem);
    void AddAddress();
    void ClearAllBookmarks();
    unsigned int GetMemory(unsigned int nAddr, int type);

    void ExportJSON();
    void ImportFromFile(std::wstring filename);
    void GenerateResizes(HWND hDlg);
    std::wstring ImportDialog();

    void AddBookmarkMap(_In_ const MemBookmark* bookmark)
    {
        if (m_BookmarkMap.find(bookmark->Address()) == m_BookmarkMap.end())
            m_BookmarkMap.insert(std::map<ra::ByteAddress, std::vector<const MemBookmark*>>::value_type(bookmark->Address(), std::vector<const MemBookmark*>()));

        std::vector<const MemBookmark*> *v = &m_BookmarkMap[bookmark->Address()];
        v->push_back(bookmark);
    }

public:
    const MemBookmark* FindBookmark(const ra::ByteAddress& nAddr) const
    {
        std::map<ra::ByteAddress, std::vector<const MemBookmark*>>::const_iterator iter = m_BookmarkMap.find(nAddr);
        return(iter != m_BookmarkMap.end() && iter->second.size() > 0) ? iter->second.back() : nullptr;
    }

private:
    HWND m_hMemBookmarkDialog{};
    std::map<ra::ByteAddress, std::vector<const MemBookmark*>> m_BookmarkMap;
    std::vector<MemBookmark*> m_vBookmarks;
};

extern Dlg_MemBookmark g_MemBookmarkDialog;

#endif // !RA_DLG_MEMBOOKMARK_H
