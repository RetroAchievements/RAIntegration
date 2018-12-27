#ifndef RA_DLG_MEMBOOKMARK_H
#define RA_DLG_MEMBOOKMARK_H
#pragma once

#include "ra_fwd.h"
#include "ra_type_traits.h"

#include "ui/viewmodels/MessageBoxViewModel.hh"

class MemBookmark
{
public: // Too much trouble making this a literal type
    MemBookmark() noexcept = default;
    ~MemBookmark() noexcept = default;
    // Copying deleted to enforce uniqueness and speed
    MemBookmark(const MemBookmark&) = delete;
    MemBookmark& operator=(const MemBookmark&) = delete;
    inline constexpr MemBookmark(MemBookmark&&) noexcept = default;
    inline constexpr MemBookmark& operator=(MemBookmark&&) noexcept = default;

    inline auto SetDescription(const std::wstring& string) { m_sDescription = string; }
    // rvalues are sometimes used
    inline auto SetDescription(std::wstring&& string) noexcept { m_sDescription = std::move(string); }
    _CONSTANT_FN SetAddress(unsigned int nVal) noexcept { m_nAddress = nVal; }
    _CONSTANT_FN SetType(unsigned int nVal) noexcept { m_nType = nVal; }
    _CONSTANT_FN SetValue(unsigned int nVal) noexcept { m_sValue = nVal; }
    _CONSTANT_FN SetPrevious(unsigned int nVal) noexcept { m_sPrevious = nVal; }
    _CONSTANT_FN IncreaseCount() noexcept { m_nCount++; }
    _CONSTANT_FN ResetCount() noexcept { m_nCount = 0; }

    _CONSTANT_FN SetFrozen(bool b) noexcept { m_bFrozen = b; }
    _CONSTANT_FN SetDecimal(bool b) noexcept { m_bDecimal = b; }

    _NODISCARD inline auto& Description() const noexcept { return m_sDescription; }
    _NODISCARD _CONSTANT_FN Address() const noexcept { return m_nAddress; }
    _NODISCARD _CONSTANT_FN Type() const noexcept { return m_nType; }
    _NODISCARD _CONSTANT_FN Value() const noexcept { return m_sValue; }
    _NODISCARD _CONSTANT_FN Previous() const noexcept { return m_sPrevious; }
    _NODISCARD _CONSTANT_FN Count() const noexcept { return m_nCount; }

    _NODISCARD _CONSTANT_FN Frozen() const noexcept { return m_bFrozen; }
    _NODISCARD _CONSTANT_FN Decimal() const noexcept { return m_bDecimal; }

    static constexpr auto INVALID_ADDRESS = std::numeric_limits<unsigned int>::max();

private:
    std::wstring m_sDescription;
    unsigned int m_nAddress = INVALID_ADDRESS;
    unsigned int m_nType = 0U;
    unsigned int m_sValue = 0U;
    unsigned int m_sPrevious = 0U;
    unsigned int m_nCount = 0U;
    bool m_bFrozen = false;
    bool m_bDecimal = false;

    // Nonmember helper functions to not need a map
    friend _CONSTANT_FN operator==(const MemBookmark& a, const MemBookmark& b) noexcept;
    friend _CONSTANT_FN operator==(const MemBookmark& value, unsigned int key) noexcept;
    friend _CONSTANT_FN operator==(unsigned int key, const MemBookmark& value) noexcept;
    friend _CONSTANT_FN operator<(const MemBookmark& a, const MemBookmark& b) noexcept;
    friend _CONSTANT_FN operator<(unsigned int key, const MemBookmark& value) noexcept;
    friend _CONSTANT_FN operator<(const MemBookmark& value, unsigned int key) noexcept;
};

// These operators assume the address is unique
// These are all needed for either std::find, std::sort, and std::binary_search
_NODISCARD _CONSTANT_FN operator==(const MemBookmark& a, const MemBookmark& b) noexcept
{
    return (a.m_nAddress == b.m_nAddress);
}

_NODISCARD _CONSTANT_FN operator==(const MemBookmark& value, unsigned int key) noexcept
{
    return (value.m_nAddress == key);
}
_NODISCARD _CONSTANT_FN operator==(unsigned int key, const MemBookmark& value) noexcept
{
    return (key == value.m_nAddress);
}

_NODISCARD _CONSTANT_FN operator<(const MemBookmark& a, const MemBookmark& b) noexcept
{
    return (a.m_nAddress < b.m_nAddress);
}

_NODISCARD _CONSTANT_FN operator<(unsigned int key, const MemBookmark& value) noexcept
{
    return (key < value.m_nAddress);
}

_NODISCARD _CONSTANT_FN operator<(const MemBookmark& value, unsigned int key) noexcept
{
    return (value.m_nAddress < key);
}

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
    void InstallHWND(HWND hWnd) noexcept { m_hMemBookmarkDialog = hWnd; }
    inline auto GetHWND() const noexcept { return m_hMemBookmarkDialog; }
    BOOL IsActive() const noexcept;

    _NODISCARD inline auto& Bookmarks() noexcept { return m_vBookmarks; }
    _NODISCARD inline auto& Bookmarks() const noexcept { return m_vBookmarks; }
    void UpdateBookmarks(bool bForceWrite);
    inline auto AddBookmark(MemBookmark&& newBookmark)
    {
        if (BookmarkExists(newBookmark.Address()))
        {
            auto duplicate = std::move(newBookmark); // move it so it can be destroyed
            using ra::ui::viewmodels::MessageBoxViewModel;
            MessageBoxViewModel::ShowWarningMessage(L"A bookmark with the same address has already been added!");

            return;
        }
        m_vBookmarks.push_back(std::move(newBookmark));
        Sort(); // sort after adding
    }
    void WriteFrozenValue(const MemBookmark& Bookmark);

    void OnLoad_NewRom();

    static INT_PTR CALLBACK s_MemBookmarkDialogProc(HWND, UINT, WPARAM, LPARAM);

protected:
    _NODISCARD INT_PTR MemBookmarkDialogProc(HWND, UINT, WPARAM, LPARAM);

private:
    void PopulateList();
    void SetupColumns(HWND hList);
    BOOL EditLabel(int nItem, int nSubItem);
    void AddAddress();
    void ClearAllBookmarks() noexcept;
    unsigned int GetMemory(unsigned int nAddr, int type);

    void ExportJSON();
    void ImportFromFile(std::wstring&& filename);
    void GenerateResizes(HWND hDlg);
    std::wstring ImportDialog();

public:
    bool BookmarkExists(const ra::ByteAddress& nAddr) const
    {
        // Should be sorted
        return (std::binary_search(m_vBookmarks.cbegin(), m_vBookmarks.cend(), nAddr));
    }

    auto& FindBookmark(const ra::ByteAddress& nAddr) const
    {
        static const MemBookmark dummyBookmark;

        if (BookmarkExists(nAddr)) // Should be sorted when it was added (if it exists)
            // TBD: There could be a better algorithm but this should be OK
            return *std::find(m_vBookmarks.cbegin(), m_vBookmarks.cend(), nAddr);

        return dummyBookmark;
    }

    void Sort() { std::sort(m_vBookmarks.begin(), m_vBookmarks.end()); }

private:
    int m_nNumOccupiedRows{};
    HWND m_hMemBookmarkDialog{};
    std::vector<MemBookmark> m_vBookmarks;
};

extern Dlg_MemBookmark g_MemBookmarkDialog;

#endif // !RA_DLG_MEMBOOKMARK_H
