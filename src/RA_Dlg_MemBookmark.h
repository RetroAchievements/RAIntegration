#pragma once

#include <wtypes.h>
#include <vector>

#include "RA_Defs.h"

class MemBookmark
{
public:
    // if you're worried about speed just delete copying, the pointer is always
    // corrupted, I don't know how you guys got it to work before. It still needs
    // to be movable
    MemBookmark() noexcept = default;
    ~MemBookmark() noexcept = default;
    MemBookmark(const MemBookmark&) = delete;
    MemBookmark& operator=(const MemBookmark&) = delete;
    MemBookmark(MemBookmark&&) noexcept = default;
    MemBookmark& operator=(MemBookmark&&) noexcept = default;



    // setters technically return a reference (copy construction) so we can't use [[noreturn]] here
    // i.e. for SetDescription. ->decltype(m_sDescription = string) would make auto, std::string&

    // Anyway constexpr functions are guaranteed to be inline.

    void SetDescription(_In_ const std::wstring& string) noexcept { m_sDescription = string; }
    _CONSTANT_FN SetAddress(_In_ unsigned int nVal) noexcept { m_nAddress = nVal; }
    _CONSTANT_FN SetType(_In_ unsigned int nVal) noexcept { m_nType = nVal; }
    _CONSTANT_FN SetValue(_In_ unsigned int nVal) noexcept { m_sValue = nVal; }
    _CONSTANT_FN SetPrevious(_In_ unsigned int nVal) noexcept { m_sPrevious = nVal; }
    _CONSTANT_FN IncreaseCount() noexcept { m_nCount++; }
    _CONSTANT_FN ResetCount() noexcept { m_nCount = 0; }

    _CONSTANT_FN SetFrozen(_In_ bool b) noexcept { m_bFrozen = b; }
    _CONSTANT_FN SetDecimal(_In_ bool b) noexcept { m_bDecimal = b; }

    _NODISCARD inline auto& Description() const noexcept { return m_sDescription; }
    _NODISCARD _CONSTANT_FN Address() const noexcept { return m_nAddress; }
    _NODISCARD _CONSTANT_FN Type() const noexcept { return m_nType; }
    _NODISCARD _CONSTANT_FN Value() const noexcept { return m_sValue; }
    _NODISCARD _CONSTANT_FN Previous() const noexcept { return m_sPrevious; }
    _NODISCARD _CONSTANT_FN Count() const noexcept { return m_nCount; }

    _NODISCARD _CONSTANT_FN Frozen() const noexcept { return m_bFrozen; }
    _NODISCARD _CONSTANT_FN Decimal() const noexcept { return m_bDecimal; }

    // Though copying is disabled we do need to clone sometimes (adding to vector and then map)
    void Clone(_Out_ MemBookmark& b) noexcept
    {
        // we might have to do it manually
        b.m_sDescription = this->m_sDescription; // the copy constructor is not constexpr
        b.SetAddress(this->m_nAddress);
        b.SetType(this->m_nType);
        b.SetValue(this->m_sValue);
        b.SetPrevious(this->m_sPrevious);
        b.m_nCount = this->m_nCount;
        b.SetFrozen(this->m_bFrozen);
        b.SetDecimal(this->m_bDecimal);
    }
private:
    // OK, seeing these as gigantic numbers isn't a good sign
    std::wstring m_sDescription;
    unsigned int m_nAddress{ 0U };
    unsigned int m_nType{ 0U };
    unsigned int m_sValue{ 0U };
    unsigned int m_sPrevious{ 0U };
    unsigned int m_nCount{ 0U };
    bool m_bFrozen{ false }; // int is larger than bool
    bool m_bDecimal{ false };


    // From previous observations it seems like the Address is the unique identifier
    _NODISCARD _CONSTANT_VAR friend operator==(_In_ const MemBookmark& a, _In_ const MemBookmark& b) noexcept
        ->decltype(a.m_nAddress == b.m_nAddress)
    {
        return{ a.m_nAddress == b.m_nAddress };
    }
    _NODISCARD _CONSTANT_VAR friend operator<(_In_ const MemBookmark& a, _In_ const MemBookmark& b) noexcept
        ->decltype(a.m_nAddress < b.m_nAddress)
    {
        return{ a.m_nAddress < b.m_nAddress };
    }

    friend auto swap(_Inout_ MemBookmark& a, _Inout_ MemBookmark& b) noexcept
        ->decltype(std::swap(a.m_nAddress, b.m_nAddress));
};

// No return doesn't work everywhere, it must be a bug

class Dlg_MemBookmark
{
    // one is const and one isn't const, isn't that an error in logic? Like if I
    // put Bookmarks as the mapped type it won't be const anymore.
    using MyBookmarks    = std::vector<MemBookmark>;
    using MyBookmarkMap  = std::map<ByteAddress, MyBookmarks>;
    using iterator       = MyBookmarkMap::iterator;
    using const_iterator = MyBookmarkMap::const_iterator;

public:
    // compiler generated constructor throws on all except move assignment and
    // destructor so we can't use = default - Samer
    Dlg_MemBookmark() noexcept {};
    ~Dlg_MemBookmark() noexcept = default;
    Dlg_MemBookmark(const Dlg_MemBookmark&) = delete;
    // probably doesn't need moving either
    Dlg_MemBookmark& operator=(const Dlg_MemBookmark&) = delete;
    Dlg_MemBookmark(Dlg_MemBookmark&&) noexcept = delete;
    Dlg_MemBookmark& operator=(Dlg_MemBookmark&&) noexcept = delete;



    _NODISCARD static INT_PTR CALLBACK s_MemBookmarkDialogProc(_In_ HWND, _In_ UINT,
        _In_ WPARAM, _In_ LPARAM);
    _NODISCARD INT_PTR CALLBACK MemBookmarkDialogProc(_In_ HWND, _In_ UINT,
        _In_ WPARAM, _In_ LPARAM);

    void InstallHWND(_In_ HWND hWnd) { m_hMemBookmarkDialog = hWnd; }
    _NODISCARD _CONSTANT_FN& GetHWND() const { return m_hMemBookmarkDialog; }
    _NODISCARD BOOL IsActive() const;

    // Needs non-const reference version for EditProcBM
    _NODISCARD auto& Bookmarks() { return m_vBookmarks; }
    _NODISCARD auto& Bookmarks() const { return m_vBookmarks; }

    _NODISCARD auto end() const noexcept
        ->decltype(std::declval<MyBookmarkMap&>().cend())
    {
        return m_BookmarkMap.cend();
    }

    void UpdateBookmarks(_In_ bool bForceWrite);
    auto AddBookmark(MemBookmark&& newBookmark) noexcept // ok it's void
        ->decltype(std::declval<MyBookmarks&>().push_back(std::move(newBookmark)))
    {
        MemBookmark myClone;
        newBookmark.Clone(myClone);
        m_vBookmarks.push_back(std::move(newBookmark));
        AddBookmarkMap(std::move(myClone));

    }
    void WriteFrozenValue(_In_ const MemBookmark& Bookmark);

    void OnLoad_NewRom();

private:
    int m_nNumOccupiedRows{ 0 };

    void PopulateList();
    void SetupColumns(_In_ HWND hList);
    _NODISCARD BOOL EditLabel(_In_ int nItem, _In_ int nSubItem);
    void AddAddress();
    void ClearAllBookmarks() noexcept;
    _NODISCARD unsigned int GetMemory(_In_ unsigned int nAddr, _In_ int type);

    void ExportJSON();
    void ImportFromFile(_In_ std::string&& filename);
    void GenerateResizes(_Inout_ HWND& hDlg);
    _NODISCARD std::string ImportDialog();

    void AddBookmarkMap(_In_ MemBookmark&& bookmark) noexcept;

public:
    _NODISCARD auto FindBookmark(_In_ const ByteAddress nAddr) const noexcept
        ->decltype(std::declval<const MyBookmarkMap&>().find(nAddr))
    {
        return m_BookmarkMap.find(nAddr);
    }
private:
    HWND m_hMemBookmarkDialog{ nullptr };
    MyBookmarkMap m_BookmarkMap;
    MyBookmarks m_vBookmarks;
};

extern Dlg_MemBookmark g_MemBookmarkDialog;
