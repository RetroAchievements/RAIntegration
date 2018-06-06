#ifndef RA_HANDLES_H
#define RA_HANDLES_H
#pragma once

#include <memory>
#include <WTypes.h>
#include <winhttp.h>
#include <WindowsX.h>

// w/e
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#include "ra_type_traits"
#include "ra_utility"
#include "ra_errors.h"

#ifndef _NORETURN
#define _NORETURN [[noreturn]]
#endif // !_NORETURN



// Some notes, the most important thing is that parameters in calling functions should be pointers.
// Usually with std::unique_ptr, the pointer type is T* but window handles are already pointers

// std::default_delete uses the delete operator which calls T's destructor but
// Windows Handles don't have destructors so we have to make our own deleter
// wrappers.

// Parameters in these classes are pointers because they are going to be owned
// and freed when their use is done.

// It's much better to have pointers being returned in calling functions (not here)

namespace ra {


namespace detail {

// Basically all wrappers will be the same except for the constructors that call Windows functions
/// <summary>
///   <para>
///     This class provides the type traits for the underlying handle and base
///     functionality for all derived classes.
///   </para>
///   <para>
///     Derived classes will need to implement their constructors to initialize
///     the owned handle.
///   </para>
///   <para>
///     The owned handle will be destroyed at the end of its lifetime scope.
///   </para>
///   <para>Non-member callbacks of constructors is recommended.</para>
/// </summary>
/// <typeparam name="HandleType">
///   The type of handle to be managed, it can either be a pointer or an object.
///   The pointer or object must satisfy the
///   <a href="http://en.cppreference.com/w/cpp/concept/NullablePointer">NullablePointer</a>
///   concept requirements.
/// </typeparam>
/// <typeparam name="DeleterType">
///   The type of deleter to be used to destroy a
///   <typeparamref name="HandleType" />. The deleter must satisfy the
///   <a href="http://en.cppreference.com/w/cpp/concept/FunctionObject">FunctionObject</a>
///   concept requirements.
/// </typeparam>
/// <remarks>
///   Only use this class as base and not directly. If the pointer-to-object you
///   wish to manage has a destructor, use
///   <a href="https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique">std::make_unique</a>
///   instead. Much more complex handle wrappers should be put in the
///   <see cref="ra::detail" /> namespace.
/// </remarks>
template<typename HandleType, typename DeleterType>
class IHandle
{
public:
#pragma region Traits
    using pointer      = HandleType;
    using element_type = std::remove_pointer_t<pointer>;
    using deleter_type = DeleterType;
    using handle_type  = std::unique_ptr<element_type, deleter_type>;
#pragma endregion

#pragma region Converting Constructors
    explicit operator bool() const noexcept { return get() != pointer{}; }
    explicit operator pointer() const noexcept { return static_cast<pointer>(this); }
    explicit operator std::nullptr_t() const noexcept
    {
        if (!get())
            return static_cast<std::nullptr_t>(this);
    }
#pragma endregion

    // If you get an error while comparing try typing "using namespace std::rel_ops;"
#pragma region Operators




    _NODISCARD auto operator*() const { return { *this->get() }; }
    _NODISCARD auto operator->() const noexcept { return (this->get()); }

#pragma endregion

    // For the constructs here all you need to do is inherit them via the following
    // "using IHandle<pointer, deleter_type>::IHandle<pointer, deleter_type>;"
#pragma region Constructors
    virtual ~IHandle() noexcept
    {
        if (ihandle_)
            this->get_deleter()(this->get());
    } // end destructor


    /// <summary>
    ///   Initializes a new instance of the <see cref="IHandle" /> class with a
    ///   <c>nullptr</c>.
    /// </summary>
    /// <param name="">The <c>nullptr</c> constant.</param>
    IHandle(std::nullptr_t) noexcept : ihandle_{ pointer{} } {}

    /// <summary>
    /// Assigns <see cref="ihandle_"/> a <c>nullptr</c>.
    /// </summary>
    /// <param name="">The <c>nullptr</c> constant.</param>
    /// <returns></returns>
    IHandle& operator=(std::nullptr_t) noexcept
    {
        reset();
        return (*this);
    } // end copy assignment

    explicit IHandle(pointer p) noexcept : ihandle_{ p } {}


    IHandle(const IHandle&) = delete;
    IHandle& operator=(const IHandle&) = delete;
    IHandle(IHandle&&) noexcept = default;
    IHandle& operator=(IHandle&&) noexcept = default;

#pragma endregion


    _NODISCARD auto get() const noexcept { return ihandle_.get(); }
    _NORETURN auto reset(pointer p = pointer{}) noexcept { ihandle_.reset(p); }
    auto release() noexcept { return ihandle_.release(); }
    _NORETURN auto swap(HandleType& b) noexcept { ihandle_.swap(b.ihandle_); }
    _NODISCARD auto& get_deleter() noexcept { return ihandle_.get_deleter(); }
    _NODISCARD const auto& get_deleter() const noexcept { return ihandle_.get_deleter(); }

    // It's protected so inheritors can use it
    IHandle() noexcept = default;
protected:
    handle_type ihandle_;
};

// Only these comparisons operators are needed, if you need to compare anything other than == or < type in
// "using namespace std::rel_ops;" before making the comparison
#pragma region IHandle Comparison Operators
template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
inline constexpr bool operator==(const IHandle<HandleType, DeleterType>& a, std::nullptr_t) noexcept { return !a; }

template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
inline constexpr bool operator==(std::nullptr_t, const IHandle<HandleType, DeleterType>& b) noexcept { return !b; }

template<
    typename HandleTypeA,
    typename DeleterTypeA,
    typename HandleTypeB,
    typename DeleterTypeB,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleTypeA> and is_function_object_v<DeleterTypeA>) and
    ((is_nullable_pointer_v<HandleTypeB> and is_function_object_v<DeleterTypeB>))>
>
inline constexpr bool operator==(const IHandle<HandleTypeA, DeleterTypeA>& a,
    const IHandle<HandleTypeB, DeleterTypeB>& b)
{
    return { a.get() == b.get() };
}

template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
inline constexpr bool operator<(const IHandle<HandleType, DeleterType>& a, std::nullptr_t) noexcept
{
    using pointer = typename decltype(a)::pointer;
    return std::less<pointer>()(a.get(), nullptr);
}

template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
inline constexpr bool operator<(std::nullptr_t, const IHandle<HandleType, DeleterType>& b) noexcept
{
    using pointer = typename decltype(b)::pointer;
    return std::less<pointer>()(nullptr, b.get());
}

template<
    typename HandleTypeA,
    typename DeleterTypeA,
    typename HandleTypeB,
    typename DeleterTypeB,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleTypeA> and is_function_object_v<DeleterTypeA>) and
    ((is_nullable_pointer_v<HandleTypeB> and is_function_object_v<DeleterTypeB>))>
>
inline constexpr bool operator<(
    const IHandle<HandleTypeA, DeleterTypeA>& a,
    const IHandle<HandleTypeB, DeleterTypeB>& b)
{
    using pointer1    = typename decltype(a)::pointer;
    using pointer2    = typename decltype(b)::pointer;
    using common_type = typename std::common_type_t<pointer1, pointer2>;
    return std::less<common_type>()(a.get(), b.get());
}
#pragma endregion


template<typename HandleType, class = std::enable_if_t<is_nullable_pointer_v<HandleType>>>
struct IDeleter {
    typedef HandleType pointer;

    virtual ~IDeleter() noexcept = default;
    [[noreturn]] _Use_decl_annotations_ virtual void operator()(_In_ pointer p) const noexcept = 0;
};



} // namespace detail

struct CFileDeleter {
    typedef FILE* pointer;

    [[noreturn]] void operator()(pointer p) noexcept {
        if (p)
            fclose(p);
    }
};

// On for C File Streams, calling CFileH because there's gonna be one that's just
// FileH for HANDLE which will be an alias for HandleH
class CFileH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<FILE>, CFileDeleter>;
    using element_type = typename handle_type::element_type;
    using pointer      = typename handle_type::pointer;
    using deleter_type = typename handle_type::deleter_type;

    explicit operator bool() const noexcept { return get() != pointer{}; }

    // It's getting kind of annoying
    bool operator==(std::nullptr_t) const noexcept { return get(); }

    CFileH(std::nullptr_t) noexcept : cfile_h{ pointer{} } {}

    // You can only assign a null pointer
    CFileH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    explicit CFileH(pointer p) noexcept : cfile_h{ p } {}

    // does fopen_s return FILE*? I don't think so!
#pragma warning(disable : 4996)
    CFileH(const std::string& fileName, const char* openMode) :
        cfile_h{ std::fopen(fileName.c_str(), openMode) } {
        if (!cfile_h)
            ThrowLastError();
    }

    // I really hope this is ok
    CFileH(std::string&& fileName, const char* openMode)
    {
        auto myStr = std::move(fileName);
        cfile_h.reset(std::fopen(myStr.c_str(), openMode));

        if (!cfile_h)
            ThrowLastError();
    }

    CFileH(const char* fileName, const char* openMode) :
        cfile_h{ std::fopen(fileName, openMode) } {
        if (!cfile_h)
            ThrowLastError();
    }
#pragma warning(default : 4996)
    ~CFileH() noexcept {
        if (cfile_h)
            reset();
    }

    CFileH(const CFileH&) = delete;
    CFileH& operator=(const CFileH&) = delete;
    CFileH(CFileH&&) noexcept = default;
    CFileH& operator=(CFileH&&) noexcept = default;
    constexpr CFileH() noexcept = default;

    _NODISCARD pointer get() const noexcept { return cfile_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { cfile_h.reset(p); }
    pointer release() noexcept { return cfile_h.release(); }

    // the capacity of buffer must be set ahead of time, if the size is 0
    template<typename String, class = std::enable_if_t<std::_Is_character<typename String::value_type>::value>>
    [[noreturn]] void write(const String& buffer, typename const String::size_type count = 16U) {
        using char_t = typename String::value_type;
        // if it's a standard container this should work
        auto cap = buffer.capacity();
        cap -= count;
        auto chars = std::fwrite(buffer.data(), sizeof(char_t), cap, get());
        if (chars != count)
            throw std::out_of_range{ std::string{"An error occurred while write to file '"} +buffer + "'" };
    }

private:
    handle_type cfile_h;
};

_NODISCARD CFileH CALLBACK make_cfile(_In_ const std::string& fileName, _In_ const char* openMode);
_NODISCARD CFileH CALLBACK make_cfile(_In_ const char* fileName, _In_ const char* openMode);

// There aren't SAL annotations for consume or forward parameters
_NODISCARD CFileH CALLBACK make_cfile(std::string&& fileName, _In_ const char* openMode);
// alright making this use a base is too complicated right now, we can refactor later
// std::unique_ptr does some of this stuff differently, we're just doing what we need to do

// it didn't satisfy unique_ptr we'll make a deleter
struct ModuleDeleter {
    typedef HMODULE pointer; // HMODULE and HINSTANCE are the same in Win32

    [[noreturn]] void operator()(pointer p) const noexcept {
        ::FreeLibrary(p);
    }

};


class ModuleH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<HMODULE>, ModuleDeleter>;
    using element_type = typename handle_type::element_type;
    using pointer      = typename handle_type::pointer;
    using deleter_type = typename handle_type::deleter_type;


    explicit operator bool() const noexcept { return get() != pointer{}; }
    ModuleH(std::nullptr_t) noexcept : cfile_h{ pointer{} } {}
    ModuleH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    explicit ModuleH(pointer p) noexcept : cfile_h{ p } {}
    explicit ModuleH(LPCTSTR lpLibraryName) noexcept : cfile_h{ ::LoadLibrary(lpLibraryName) } {}


    ~ModuleH() noexcept {
        if (cfile_h)
            reset();
    }

    ModuleH(const ModuleH&) = delete;
    ModuleH& operator=(const ModuleH&) = delete;
    ModuleH(ModuleH&&) noexcept = default;
    ModuleH& operator=(ModuleH&&) noexcept = default;
    constexpr ModuleH() noexcept = default;

    _NODISCARD pointer get() const noexcept { return cfile_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { cfile_h.reset(p); }
    pointer release() noexcept { return cfile_h.release(); }

private:
    handle_type cfile_h;
};

static_assert(is_unique_ptr_v<ModuleH, ModuleH::pointer>);

// make_module CALLBACK
_NODISCARD ModuleH CALLBACK make_module(_In_ LPCTSTR lpLibraryName) noexcept;

struct MenuDeleter {
    typedef HMENU pointer;

    _Use_decl_annotations_
        [[noreturn]] void operator()(_In_ pointer p) const noexcept { ::DestroyMenu(p); }
};


class MenuH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    using element_type = typename handle_type::element_type;
    using deleter_type = typename handle_type::deleter_type;
    using pointer      = typename handle_type::pointer;


    explicit operator bool() const noexcept { return get() != pointer{}; }

    // not sure if this is OK or not but I did see a HMENU being converted to an UINT_PTR
    explicit operator UINT_PTR() const noexcept { return reinterpret_cast<UINT_PTR>(get()); }


    MenuH(std::nullptr_t) noexcept : menu_h{ pointer{} } {}
    MenuH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    explicit MenuH(pointer p) noexcept : menu_h{ p } {}

    MenuH() noexcept : menu_h{ ::CreatePopupMenu() } {}

    ~MenuH() noexcept {
        if (menu_h)
            reset();
    } // end destructor


    MenuH(const MenuH&) = delete;
    MenuH& operator=(const MenuH&) = delete;
    MenuH(MenuH&&) noexcept = default;
    MenuH& operator=(MenuH&&) noexcept = default;

    _NODISCARD pointer get() const noexcept { return menu_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { menu_h.reset(p); }
    pointer release() noexcept { return menu_h.release(); }

private:
    handle_type menu_h;
};
static_assert(is_unique_ptr_v<MenuH, MenuH::pointer>);

_NODISCARD MenuH CALLBACK make_menu(_In_ HMENU hmenu = ::CreatePopupMenu()) noexcept;

// Originally this was three classes, it's now just one but in the detail namespace to prevent mistakes
// Use the callback functions instead

#pragma region HINTERNET
namespace detail {

struct InternetDeleter {
    typedef HINTERNET pointer;

    void operator()(pointer p) const noexcept { ::WinHttpCloseHandle(p); }
};

class InternetH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<HINTERNET>, InternetDeleter>;
    using pointer      = typename handle_type::pointer;
    using element_type = typename handle_type::element_type;
    using deleter_type = typename handle_type::deleter_type;



    explicit operator bool() const noexcept { return get() != pointer{}; }


    // can't be constexpr since it's fundamentally void*
    InternetH(std::nullptr_t) noexcept : internet_h{ pointer{} } {}
    InternetH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    explicit InternetH(pointer p) noexcept : internet_h{ p } {}


    _Use_decl_annotations_
        InternetH(_In_opt_z_ LPCWSTR pszAgentW,
            _In_ DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            _In_opt_z_ LPCWSTR pszProxyW = nullptr,
            _In_opt_z_ LPCWSTR pszProxyBypassW = nullptr,
            _In_ DWORD dwFlags = DWORD{}) noexcept :
        internet_h{ ::WinHttpOpen(pszAgentW, dwAccessType, pszProxyW, pszProxyW, dwFlags) } {
    } // end constructor

    // Copying is deleted so it can be passed by value cheaply, not sure if WinHttpConnect deallocates so we are
    // just calling get instead of release just to be safe. It can only move from one place to another

    _Use_decl_annotations_
        InternetH(_In_ HINTERNET hSession,
            _In_ LPCWSTR pswzServerName,
            _In_ INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT,
            _In_ DWORD dwReserved = DWORD{}) noexcept :
        internet_h{ ::WinHttpConnect(hSession, pswzServerName, nServerPort, dwReserved) } {
    } // end constructor

    _Use_decl_annotations_
        InternetH(_In_ HINTERNET hConnect,
            _In_ LPCWSTR pwszVerb,
            _In_ LPCWSTR pwszObjectName,
            _In_ LPCWSTR pwszVersion = nullptr,
            _In_opt_ LPCWSTR pwszReferrer = nullptr,
            _In_opt_ LPCWSTR far* ppwszAcceptTypes = nullptr,
            _In_ DWORD dwFlags = DWORD{}) noexcept :
        internet_h{ ::WinHttpOpenRequest(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer,
            ppwszAcceptTypes, dwFlags) } {
    } // end constructor

    ~InternetH() noexcept {
        if (internet_h)
            reset();
    } // end destructor

#pragma region Constructors
    InternetH(const InternetH&) = delete;
    InternetH& operator=(const InternetH&) = delete;
    InternetH(InternetH&&) noexcept = default;
    InternetH& operator=(InternetH&&) noexcept = default;
    constexpr InternetH() noexcept = default;
#pragma endregion


    _NODISCARD pointer get() const noexcept { return internet_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { internet_h.reset(p); }
    pointer release() noexcept { return internet_h.release(); }

private:
    handle_type internet_h;
};
static_assert(is_unique_ptr_v<InternetH, InternetH::pointer>);

} // namespace detail

// __stdcall and __thiscall are similar, stack cleanup-wise

// All the same type just aliases to prevent confusion
using SessionH = detail::InternetH;
using ConnectH = detail::InternetH;
using RequestH = detail::InternetH;

// The HINTENET owner handle must be an rvalue reference, or value

// Makes an owner to a session handle
SessionH CALLBACK make_session(LPCWSTR pszAgentW,
    _In_ DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    _In_opt_z_ LPCWSTR pszProxyW = nullptr,
    _In_opt_z_ LPCWSTR pszProxyBypassW = nullptr,
    _In_ DWORD dwFlags = DWORD{}) noexcept;

// Makes an owner to a connect
// release ownership from the SessionH that owns hSession
ConnectH CALLBACK make_connect(_In_ HINTERNET hSession,
    _In_ LPCWSTR pswzServerName,
    _In_ INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT,
    _In_ DWORD dwReserved = DWORD{}) noexcept;

// Makes an owner to a request handle
// Do not release ownership from the ConnectH that owns hConnect, just call get
RequestH CALLBACK make_request(_In_ HINTERNET hConnect,
    _In_ LPCWSTR pwszVerb,
    _In_ LPCWSTR pwszObjectName,
    _In_ LPCWSTR pwszVersion = nullptr,
    _In_opt_ LPCWSTR pwszReferrer = nullptr,
    _In_opt_ LPCWSTR far* ppwszAcceptTypes = nullptr,
    _In_ DWORD dwFlags = DWORD{}) noexcept;

#pragma endregion

// putting this as one class because we couldn't use the macros in WindowsX
// There will be alias templates and helper functions



#pragma region GDI Handles
namespace detail {

// HDC will need it's own deleter and will not be in GDIObjH because it has two

// You know what? We can make function object to make the compiler stop complaining
// Sure we don't use all these GDI Handles but better to get prepared so we don't have to change it later
template<
    typename HGDIObj,
    class = std::enable_if_t<
    ((std::is_same_v<HGDIObj, HGDIOBJ>) or (std::is_same_v<HGDIObj, HBITMAP>)  or
    (std::is_same_v<HGDIObj, HPEN>)     or (std::is_same_v<HGDIObj, HBRUSH>)   or
        (std::is_same_v<HGDIObj, HFONT>)    or (std::is_same_v<HGDIObj, HPALETTE>) or
        (std::is_same_v<HGDIObj, HRGN>))   and !std::is_null_pointer_v<HGDIObj>
    >
>
struct GDIDeleter {
    // required
    typedef HGDIObj pointer;

    // Really doubt the wrong type will be used with all those checks
    _Use_decl_annotations_
        [[noreturn]] void operator()(_In_ pointer p) const noexcept {
        // can't use switch on types
        // we could use enums like we did for HDC, but since HDC is one type it needed enums
        if (std::is_same_v<HGDIObj, HBITMAP>)
            ::DeleteBitmap(p);
        else if (std::is_same_v<HGDIObj, HPEN>)
            ::DeletePen(p);
        else if (std::is_same_v<HGDIObj, HBRUSH>)
            ::DeleteBrush(p);
        else if (std::is_same_v<HGDIObj, HFONT>)
            ::DeleteFont(p);
        else if (std::is_same_v<HGDIObj, HPALETTE>)
            ::DeletePalette(p);
        else if (std::is_same_v<HGDIObj, HRGN>)
            ::DeleteRgn(p);
        else
            ::DeleteObject(p);
    }
};



// Some parameters are rearranged putting the optional ones last in the order they appear
// COM attributes are replaced with SAL annotations because the compiler will say they are deprecated
// It's more apparent in the .NET Direct3D API but not here (they use SAL in the interop code)

// This will not have restrictions class-wide since it will mess things up, but there will be restrictions on the constructors
template<typename GDIObj>
class GDIObjH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<GDIObj>, GDIDeleter<GDIObj>>;
    using pointer      = typename handle_type::pointer;
    using element_type = typename handle_type::element_type;
    using deleter_type = typename handle_type::deleter_type;


    // let's give a bool operator so we don't have to put == nullptr each time
    explicit operator bool() const noexcept { return get() != pointer{}; }



    // with nullptr
    GDIObjH(std::nullptr_t) noexcept : gdiobj_h{ pointer{} } {}

    // Pointer specific, not recommended but needs to be here
    explicit GDIObjH(pointer p) noexcept : gdiobj_h{ p } {}

    GDIObjH& operator=(nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy constructor

    // We're not going to put all the possibilities, just the ones we need or feel like

    template<class = std::enable_if_t<std::is_same_v<pointer, HBITMAP>>>
    GDIObjH(_In_ int nWidth, _In_ int nHeight, _In_ UINT nPlanes, _In_ UINT nBitCount,
        _In_opt_ const void* lpBit = nullptr) noexcept :
        gdiobj_h{ ::CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBit) } {
    } // end constructor

    template<class = std::enable_if_t<std::is_same_v<pointer, HBITMAP>>>
    GDIObjH(_In_ CONST BITMAPINFO* lpbmi, _In_ UINT usage, _In_ DWORD offset,
        _In_opt_ HDC hdc = nullptr, _Deref_out_opt_ VOID** ppvBits = nullptr,
        _In_opt_ HANDLE hSection = nullptr) noexcept :
        gdiobj_h{ ::CreateDIBSection(hdc, lpbmi, usage, ppvBits, hSection, offset) } {
    } // end constructor

    // and another one
    // CreateCompatibleBitmap
    template<class = std::enable_if_t<std::is_same_v<pointer, HBITMAP>>>
    GDIObjH(_In_ HDC hdc, _In_ int cx, _In_ int cy) noexcept :
        gdiobj_h{ ::CreateCompatibleBitmap(hdc, cx, cy) } {
    } // end constructor


    template<class = std::enable_if_t<std::is_same_v<pointer, HPEN>>>
    GDIObjH(_In_ int iStyle, _In_ int cWidth, _In_ COLORREF color) noexcept :
        gdiobj_h{ ::CreatePen(iStyle, cWidth, color) } {
    } // end constructor

    // Gets a stock something, this one would be difficult to filter
    GDIObjH(_In_ int i) noexcept :
        gdiobj_h{ make_stock_object<GDIObj>(i).release() } // wonder if this will work
    {
    } // end constructor



    template<class = std::enable_if_t<std::is_same_v<pointer, HBRUSH>>>
    GDIObjH(_In_ COLORREF color) noexcept :
        gdiobj_h{ ::CreateSolidBrush(color) } {
    } // end constructor

    template<class = std::enable_if_t<std::is_same_v<pointer, HFONT>>>
    GDIObjH(_In_ int cHeight, _In_ int cWidth, _In_ int cEscapement, _In_ int cOrientation, _In_ int cWeight,
        _In_ DWORD bItalic, _In_ DWORD bUnderline, _In_ DWORD bStrikeOut, _In_ DWORD iCharSet,
        _In_ DWORD iOutPrecision, _In_ DWORD iClipPrecision, _In_ DWORD iQuality, _In_ DWORD iPitchAndFamily,
        _In_opt_ LPCTSTR pszFaceName = nullptr) noexcept :
        gdiobj_h{ ::CreateFont(cHeight,cWidth,cEscapement,cOrientation,cWeight,bItalic,bUnderline,bStrikeOut,
            iCharSet,iOutPrecision, iClipPrecision,iQuality,iPitchAndFamily,pszFaceName) } {
    } // end constructor

    // lots of selects, the handles are explicit because of mutual exclusiveness.
    // Using the typename would mean we'd have to use SelectObject but that might
    // cause some warnings.

    template<class = std::enable_if_t<std::is_same_v<pointer, HGDIOBJ>>>
    GDIObjH(_In_ HDC hdc, _In_ HGDIOBJ hbo) noexcept : gdiobj_h{ ::SelectObject(hdc, hbo) } {}

    template<class = std::enable_if_t<std::is_same_v<pointer, HBITMAP>>>
    GDIObjH(_In_ HDC hdc, _In_ HBITMAP hbm) noexcept : gdiobj_h{ SelectBitmap(hdc, hbm) } {}

    template<class = std::enable_if_t<std::is_same_v<pointer, HBRUSH>>>
    GDIObjH(_In_ HDC hdc, _In_ HBRUSH hbr) noexcept : gdiobj_h{ SelectBrush(hdc, hbr) } {}

    template<class = std::enable_if_t<std::is_same_v<pointer, HPEN>>>
    GDIObjH(_In_ HDC hdc, _In_ HPEN hpen) noexcept : gdiobj_h{ SelectPen(hdc, hpen) } {}

    template<class = std::enable_if_t<std::is_same_v<pointer, HFONT>>>
    GDIObjH(_In_ HDC hdc, _In_ HFONT hfont) noexcept : gdiobj_h{ SelectFont(hdc, hfont) } {}

    // It's trivial so we'll do the rest, nope is a bit different
    template<class = std::enable_if_t<std::is_same_v<pointer, HPALETTE>>>
    GDIObjH(_In_ HDC hdc, _In_ HPALETTE hpen, _In_ bool bForceBkgd) noexcept :
        gdiobj_h{ ::SelectPalette(hdc, hpen, bForceBkgd) } {
    }


    ~GDIObjH() noexcept {
        if (gdiobj_h)
            reset();
    } // end destructor


    GDIObjH(const GDIObjH&) = delete;
    GDIObjH& operator=(const GDIObjH&) = delete;
    GDIObjH(GDIObjH&&) noexcept = default;
    GDIObjH& operator=(GDIObjH&&) noexcept = default;
    inline constexpr GDIObjH() noexcept = default;


    _NODISCARD pointer get() const noexcept { return gdiobj_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { gdiobj_h.reset(p); }
    pointer release() noexcept { return gdiobj_h.release(); }
    deleter_type get_deleter() noexcept { return gdiobj_h.get_deleter(); }

    // The default would outright delete b's pointer so we have to do something a
    // little different unless you want to delete copying, though it is a bit incorrect (unique_ptr shouldn't be passed by const reference)
    [[noreturn]] inline constexpr void swap(GDIObjH& b) noexcept {
        auto myPtr{ b.get() };
        gdiobj_h.reset(b.get());
        b.reset(myPtr);

        // this will not delete the pointer owned by gdiobj_h, but the pointer that was copied
        if (myPtr)
            get_deleter()(myPtr);
    }

    // this will do the default, for unique_ptr (not shared_ptr) passing by value
    // or rvalue reference is the same thing since you have to move them anyway
    [[noreturn]] inline constexpr void swap(GDIObjH&& b) noexcept { gdiobj_h.swap(std::forward<GDIObjH>(b)); }

private:
    handle_type gdiobj_h;

};

} // namespace detail


using HGDIObjH = detail::GDIObjH<HGDIOBJ>;
using BitmapH  = detail::GDIObjH<HBITMAP>;
using BrushH   = detail::GDIObjH<HBRUSH>;
using PenH     = detail::GDIObjH<HPEN>;
using FontH    = detail::GDIObjH<HFONT>;

// If it works for one it works for all of them
static_assert(is_unique_ptr_v<HGDIObjH, HGDIOBJ>);

// OK, the make stock w/e's couldn't be auto deduced they'll be their own functions

_NODISCARD HGDIObjH CALLBACK make_stock_object(_In_ int i) noexcept;
_NODISCARD PenH CALLBACK make_stock_pen(_In_ int i) noexcept;
_NODISCARD BrushH CALLBACK make_stock_brush(_In_ int i) noexcept;
_NODISCARD FontH CALLBACK make_stock_font(_In_ int i) noexcept;


// normally we'd just forward all the arguments but window handles are strange and have to be dealt with somewhat
// explicitly since the GDI we're using is in C

_NODISCARD HGDIObjH CALLBACK make_gdiobject(_In_ HDC hdc, _In_ HGDIOBJ hbo) noexcept;

#pragma region Bitmap handle callbacks
// Adding more as we see them, the "select" ones will just be "make_handlename"
_NODISCARD BitmapH CALLBACK make_bitmap(_In_ int nWidth, _In_ int nHeight, _In_ UINT nPlanes,
    _In_ UINT nBitCount, _In_opt_ const void* lpBit = nullptr) noexcept;
_NODISCARD BitmapH CALLBACK make_bitmap(_In_ HDC hdc, _In_ HBITMAP hbm) noexcept;
_NODISCARD BitmapH CALLBACK make_dib_section(_In_ CONST BITMAPINFO* lpbmi, _In_ UINT usage,
    _In_ DWORD offset, _In_opt_ HDC hdc = nullptr, _Deref_out_opt_ VOID** ppvBits = nullptr,
    _In_opt_ HANDLE hSection = nullptr);
_NODISCARD BitmapH CALLBACK make_bitmap(_In_ HDC hdc, _In_ int cx, _In_ int cy) noexcept;
#pragma endregion

#pragma region Brush callbacks
_NODISCARD BrushH CALLBACK make_brush(_In_ COLORREF color) noexcept;
_NODISCARD BrushH CALLBACK make_brush(_In_ HDC hdc, _In_ HBRUSH hbm) noexcept;
#pragma endregion

#pragma region Pen callbacks
_NODISCARD PenH CALLBACK make_pen(_In_ int iStyle, _In_ int cWidth, _In_ COLORREF color) noexcept;
_NODISCARD PenH CALLBACK make_pen(_In_ HDC hdc, _In_ HPEN hbm) noexcept;
#pragma endregion

#pragma region font callbacks
_NODISCARD FontH CALLBACK make_font(_In_ int cHeight, _In_ int cWidth, _In_ int cEscapement, _In_ int cOrientation,
    _In_ int cWeight, _In_ DWORD bItalic, _In_ DWORD bUnderline, _In_ DWORD bStrikeOut, _In_ DWORD iCharSet,
    _In_ DWORD iOutPrecision, _In_ DWORD iClipPrecision, _In_ DWORD iQuality, _In_ DWORD iPitchAndFamily,
    _In_opt_ LPCTSTR pszFaceName = nullptr) noexcept;
_NODISCARD FontH CALLBACK make_font(_In_ HDC hdc, _In_ HFONT hfont) noexcept;
#pragma endregion

// HDC will pretty much have the comments copy and pasted from MSDN with a few improvements

#pragma region HDC

// Put it in detail because it's easy to mess up, use the aliases in ra instead
namespace detail {

// really doubt this enum will be used besides for aliasing
/// <summary>
///   HDC will need it's own class because of how complex it is, these enums will
///   be used to filter out what type of HDC needs to be created
/// </summary>
enum class HDCType {
    RegularDC,
    MemoryDC,
    EnhMetaFileDC,
    MetafileDC,
    WindowDC
};

// Because of the complexity of HDC it will need it's own function object deleter
template<HDCType dc_type>
struct DCDeleter {
    typedef HDC pointer;

    _Use_decl_annotations_
        [[noreturn]] void operator()(_In_ pointer p, _In_opt_ HWND hwnd = nullptr) const noexcept {
        switch (dc_type) {
            case HDCType::RegularDC:
                ::DeleteDC(p);
                break;
            case HDCType::MemoryDC:
                ::DeleteDC(p);
                break;
            case HDCType::EnhMetaFileDC:
                ::CloseEnhMetaFile(p);
                break;
            case HDCType::MetafileDC:
                ::CloseMetaFile(p);
                break;
            case HDCType::WindowDC:
                ::ReleaseDC(hwnd, p);
        }
    }
};

/// <summary>Handle owner for a <c>HDC</c></summary>
/// <typeparam name="dc_type">
///   The type of <c>HDC</c> you want managed.
/// </typeparam>
template<HDCType dc_type = HDCType::MemoryDC>
class DCH {
public:
    using handle_type  = std::unique_ptr<std::remove_pointer_t<HDC>, DCDeleter<dc_type>>;
    using pointer      = typename handle_type::pointer;
    using deleter_type = typename handle_type::deleter_type;
    using element_type = typename handle_type::element_type;
#pragma region Constructors

    explicit operator bool() const noexcept { return get() != pointer{}; }

    // Forgot to give it a destructor
    ~DCH() noexcept {
        if (dc_h)
            reset();
    }

    // can construct with a nullptr, i.e.,
    // DCH dc_h{nullptr};
    DCH(std::nullptr_t) noexcept : dc_h{ pointer{} } {} // with nullptr

    // can assign as nullptr, i.e. ->
    // dc_h.reset() -> DCH dc_h = nullptr;
    DCH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    // Needs a special restriction because of the one above
    template<class = std::enable_if_t<(dc_type == HDCType::MemoryDC)>>
    explicit DCH(_In_ HDC p) noexcept : dc_h{ ::CreateCompatibleDC(p) } {} // OK, it's hitting the right constructor


    template<class = std::enable_if_t<(dc_type == HDCType::WindowDC)>>
    DCH(_In_ HWND hwnd) noexcept : dc_h{ ::GetDC(hwnd) } {
    }

    DCH(_In_opt_ LPCTSTR lpszDriver, _In_opt_ LPCTSTR lpszDevice, _In_opt_ LPCTSTR lpszOutput,
        _In_opt_ const DEVMODE* lpInitData) noexcept :
        dc_h{ ::CreateDC(lpszDriver, lpszDevice, lpszOutput, lpInitData) } {
    } // end constructor

    template<class = std::enable_if_t<(dc_type == HDCType::EnhMetaFileDC)>>
    DCH(_In_ HDC hdcRef, _In_ LPCTSTR lpFilename, _In_ const RECT* lpRect, _In_ LPCTSTR lpDescription) noexcept :
        dc_h{ ::CreateEnhMetaFile(hdcRef, lpFilename, lpRect, lpDescription) } {
    } // end constructor

    template<class = std::enable_if_t<(dc_type == HDCType::MetafileDC)>>
    explicit DCH(_In_ LPCTSTR lpszFile) noexcept :
        dc_h{ ::CreateMetaFile(lpszFile) } {
    } // end constructor

    DCH(const DCH&) = delete;
    DCH& operator=(const DCH&) = delete;
    DCH(DCH&&) noexcept = default;
    DCH& operator=(DCH&&) noexcept = default;
    constexpr DCH() noexcept = default;
#pragma endregion

    _NODISCARD pointer get() const noexcept { return dc_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { dc_h.reset(p); }
    pointer release() noexcept { return dc_h.release(); }

    // Sometimes it needs to be modified, be careful with it
    _NODISCARD pointer set() noexcept { return dc_h._Myptr(); }

private:
    handle_type dc_h;
};

} // namespace detail  
#pragma endregion

using RegularDCH     = detail::DCH<detail::HDCType::RegularDC>;
using MemoryDCH      = detail::DCH<detail::HDCType::MemoryDC>;
using EnhMetaFileDCH = detail::DCH<detail::HDCType::EnhMetaFileDC>;
using MetaFileDCH    = detail::DCH<detail::HDCType::MetafileDC>;
using WindowDCH      = detail::DCH<detail::HDCType::WindowDC>;

// Need to test two cases
static_assert(is_unique_ptr_v<RegularDCH, HDC>);
static_assert(is_unique_ptr_v<WindowDCH, HDC, HWND>);

#pragma endregion

_NODISCARD RegularDCH CALLBACK make_regulardc(_In_opt_ LPCTSTR lpszDriver, _In_opt_ LPCTSTR lpszDevice,
    _In_opt_ LPCTSTR lpszOutput, _In_opt_ const DEVMODE* lpInitData) noexcept;
_NODISCARD MemoryDCH CALLBACK make_memorydc(_In_ HDC hdc) noexcept;
_NODISCARD EnhMetaFileDCH CALLBACK make_enhanced_metafiledc(_In_ HDC hdcRef, _In_ LPCTSTR lpFilename,
    _In_ const RECT* lpRect, _In_ LPCTSTR lpDescription) noexcept;
_NODISCARD MetaFileDCH CALLBACK make_metafiledc(_In_ LPCTSTR lpszFile) noexcept;
_NODISCARD WindowDCH CALLBACK make_windowdc(_In_ HWND hwnd) noexcept;
_Use_decl_annotations_ _NODISCARD
inline constexpr auto ValidHandle(_In_ HANDLE h) noexcept->
decltype(h != INVALID_HANDLE_VALUE) {
    return { h != INVALID_HANDLE_VALUE };
}

// The most complicated one, Windows themselves...
namespace detail {

enum class WindowType {
    MainWindow,
    ApplicationWindow,
    ControlWindow = ApplicationWindow,
    ModalDialog,
    ModelessDialog
};

template<WindowType window_type>
struct WindowDeleter : detail::IDeleter<HWND>
{

    [[noreturn]] void operator()(_In_ pointer p) const noexcept {
        if (window_type == WindowType::ModalDialog)
            ::EndDialog(p, IDOK);
        else
            ::DestroyWindow(p);
    }
};

template<WindowType window_type>
class WindowH : public detail::IHandle<HWND, WindowDeleter<window_type>
{
    using base_type = typename detail::IHandle<HWND, WindowDeleter<window_type>;
public:

    // Inherit all base constructors
    using base_type::IHandle;

    // Like before optional params will be put at the end
    template<
        class = std::enable_if_t<
        (window_type == WindowType::ApplicationWindow) or
        (window_type == WindowType::ControlWindow)>
    >
        WindowH(
            _In_     DWORD dwExStyle,
            _In_     DWORD dwStyle,
            _In_     int X,
            _In_     int Y,
            _In_     int nWidth,
            _In_     int nHeight,
            _In_opt_ LPCTSTR lpClassName  = nullptr,
            _In_opt_ LPCTSTR lpWindowName = nullptr,
            _In_opt_ HWND hWndParent      = nullptr,
            _In_opt_ HMENU hMenu          = nullptr,
            _In_opt_ HINSTANCE hInstance  = nullptr,
            _In_opt_ LPVOID lpParam       = nullptr) :
        base_type{ ::CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
            hWndParent, hMenu, hInstance, lpParam) } {}


    template<
        class = std::enable_if_t<
        (window_type == WindowType::ModalDialog) or
        (window_type == WindowType::ModelessDialog)>
    >
        _Use_decl_annotations_ WindowH(
            _In_     LPCTSTR lpTemplateName,
            _In_opt_ HINSTANCE hInstance  = nullptr,
            _In_opt_ HWND hWndParent      = nullptr,
            _In_opt_ DLGPROC lpDialogFunc = nullptr)
    {
        // Need to test this but I believe the handle to a modal is the same as the parent
        if (window_type == WindowType::ModalDialog)
        {
            ::DialogBox(hInstance, lpTemplateName, hWndParent, lpDialogFunc);
            reset(hWndParent);
        }
        else if (window_type == WindowType::ModelessDialog)
            reset(CreateDialog(hInstance, lpTemplateName, hWndParent, lpDialogFunc));
    }

    template<class = std::enable_if_t<(window_type == WindowType::ControlWindow)>>
    WindowH(_In_ int nIDDlgItem, _In_opt_ HWND hDlg = nullptr) noexcept : window_h{ ::GetDlgItem(hDlg, nIDDlgItem) } {}

    // No idea why windows says they can be optional
    template<class = std::enable_if_t<(window_type != WindowType::ModalDialog)>>
    WindowH(_In_opt_ LPCTSTR lpClassName = nullptr,
        _In_opt_ LPCTSTR lpWindowName = nullptr) noexcept : window_h{ ::FindWindow(lpClassName, lpWindowName) }
    {
    }
#pragma endregion

#pragma region General Methods
    _NODISCARD pointer get() const noexcept { return window_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { window_h.reset(p); }
    pointer release() noexcept { return window_h.release(); }
#pragma endregion
#pragma region Window Property Retrievers
    // Retrieves the Extended styles associated with a window
    _NODISCARD DWORD GetStylesEx() const noexcept
    {
        if (::GetWindowLongPtr(get(), GWL_EXSTYLE) == 0)
            return ra::ShowLastError();
    }

    // Retrieves the styles associated with a window
    _NODISCARD DWORD GetStyles() const noexcept {
        if (auto ret = ::GetWindowLongPtr(get(), GWL_STYLE); ret == 0)
            return ra::ShowLastError();
        else
            return ra::to_unsigned(ret);
    }

    // Retrieves a handle to the application instance associated with a window
    _NODISCARD HINSTANCE GetInstance() const noexcept {
        if (auto ret = ::GetWindowLongPtr(get(), GWLP_HINSTANCE); ret == 0)
            return ra::ShowLastError();
        else
            return static_cast<HINSTANCE>(ret);
    }

    // Retrieves a handle to the parent window if it exists, useful for controls and dialogs
    _NODISCARD HWND GetParentWindow() const noexcept {
        if (auto ret = ::GetWindowLongPtr(get(), GWLP_HWNDPARENT); ret == 0)
            return ra::ShowLastError();
        else
            return static_cast<HWND>(ret);
    }

    // Get's the WindowID associated with a window
    _NODISCARD int GetID() const noexcept {
        if (auto ret = ::GetWindowLongPtr(get(), GWLP_ID); ret == 0)
            return ra::ShowLastError();
        else
            return static_cast<int>(ret);
    }

    // Get's the current state of the owned window
    _NODISCARD LPVOID GetState() const noexcept {
        if (auto ret = ::GetWindowLongPtr(get(), GWLP_USERDATA); ret == 0)
            return ra::ShowLastError();
        else
            return static_cast<LPVOID>(ret);
    }
#pragma endregion
    // These are const because it doesn't actually affect the handle it does affect it somewhere else
    // There are also stuff for Window Classes but that needs to be a different class
    // One has a default value of GetState(), but it could be null
#pragma region Window Property Modifiers
    // These functions succeed if the return value is the sameas the offset

    // Modifies the Extended styles associated with the owned window
    _NODISCARD LONG_PTR SetStylesEx(DWORD dwStyleEx) const noexcept {
        if (::SetWindowLongPtr(get(), GWL_EXSTYLE, ra::to_signed(dwStyleEx)) != GWL_EXSTYLE)
            return ra::ShowLastError();
    }

    // Modifies the styles associated with the owned window
    _NODISCARD LONG_PTR SetStyles(DWORD dwStyle) const noexcept {
        if (::SetWindowLongPtr(get(), GWL_STYLE, ra::to_signed(dwStyle)) != GWL_STYLE)
            return ra::ShowLastError();
    }

    // Retrieves a handle to the application instance associated with the owned window
    _NODISCARD LONG_PTR SetInstance(HINSTANCE hInstance) const noexcept {
        if (::SetWindowLongPtr(get(), GWLP_HINSTANCE, static_cast<LONG_PTR>(hInstance)) != GWLP_HINSTANCE)
            return ra::ShowLastError();
    }

    // Sets a handle to a new parent window if it exists, useful for controls and dialogs
    _NODISCARD LONG_PTR SetParentWindow(HWND hwndNewParent) const noexcept {
        if (::SetWindowLongPtr(get(), GWLP_HWNDPARENT, static_cast<LONG_PTR>(hwndNewParent)) != GWLP_HWNDPARENT)
            return ra::ShowLastError();
    }

    // Get's the WindowID associated with a window
    _NODISCARD LONG_PTR SetID(int windowID) const noexcept {
        if (::SetWindowLongPtr(get(), GWLP_ID, windowID) != GWLP_ID)
            return ra::ShowLastError();
    }

    // Get's the current state of the owned window
    template<typename LPStateInfo = LPVOID>
    _NODISCARD LONG_PTR SetState(LPStateInfo lpState = GetState()) const noexcept {
        if (::SetWindowLongPtr(get(), GWLP_USERDATA, static_cast<LONG_PTR>(lpState)) != GWLP_USERDATA)
            return ra::ShowLastError();
    }
#pragma endregion
};

} // namespace detail

#pragma region MakeWindow callbacks
// Visual Studio is tripping balls...
using AppWindowH      = ra::detail::WindowH<ra::detail::WindowType::ApplicationWindow>;
using ControlWindowH  = ra::detail::WindowH<ra::detail::WindowType::ControlWindow>;
using ModalDialogH    = ra::detail::WindowH<ra::detail::WindowType::ModalDialog>;
using ModelessDialogH = ra::detail::WindowH<ra::detail::WindowType::ModelessDialog>;

// The class should have enough guards on it, 
_NODISCARD ControlWindowH CALLBACK make_control(
    _In_     DWORD dwExStyle,
    _In_     DWORD dwStyle,
    _In_     int X,
    _In_     int Y,
    _In_     int nWidth,
    _In_     int nHeight,
    _In_opt_ LPCTSTR lpClassName  = nullptr,
    _In_opt_ LPCTSTR lpWindowName = nullptr,
    _In_opt_ HWND hWndParent      = nullptr,
    _In_opt_ HMENU hMenu          = nullptr,
    _In_opt_ HINSTANCE hInstance  = nullptr,
    _In_opt_ LPVOID lpParam       = nullptr) noexcept;

_NODISCARD ControlWindowH CALLBACK make_control(
    _In_     int nIDDlgItem,
    _In_opt_ HWND hDlg = nullptr) noexcept;

_NODISCARD AppWindowH CALLBACK make_window(
    _In_     DWORD dwExStyle,
    _In_     DWORD dwStyle,
    _In_     int X,
    _In_     int Y,
    _In_     int nWidth,
    _In_     int nHeight,
    _In_opt_ LPCTSTR lpClassName  = nullptr,
    _In_opt_ LPCTSTR lpWindowName = nullptr,
    _In_opt_ HWND hWndParent      = nullptr,
    _In_opt_ HMENU hMenu          = nullptr,
    _In_opt_ HINSTANCE hInstance  = nullptr,
    _In_opt_ LPVOID lpParam       = nullptr) noexcept;

_NODISCARD ModalDialogH CALLBACK make_modal(
    _In_     LPCTSTR lpTemplateName,
    _In_opt_ HINSTANCE hInstance  = nullptr,
    _In_opt_ HWND hWndParent      = nullptr,
    _In_opt_ DLGPROC lpDialogFunc = nullptr) noexcept;

_NODISCARD ModelessDialogH CALLBACK make_modeless(
    _In_     LPCTSTR lpTemplateName,
    _In_opt_ HINSTANCE hInstance  = nullptr,
    _In_opt_ HWND hWndParent      = nullptr,
    _In_opt_ DLGPROC lpDialogFunc = nullptr) noexcept;

_NODISCARD AppWindowH CALLBACK make_window(
    _In_opt_ LPCTSTR lpClassName  = nullptr,
    _In_opt_ LPCTSTR lpWindowName = nullptr) noexcept;
#pragma endregion


struct HGlobalDeleter {
    typedef HGLOBAL pointer;

    [[noreturn]] void operator()(pointer p) const noexcept { GlobalFree(p); }
};

class GlobalH {
public:
#pragma region traits
    using handle_type  = std::unique_ptr<std::remove_pointer_t<HGLOBAL>, HGlobalDeleter>;
    using pointer      = typename handle_type::pointer;
    using deleter_type = typename handle_type::deleter_type;
    using element_type = typename handle_type::element_type;
#pragma endregion
    explicit operator bool() const noexcept { return get() != pointer{}; }

    // Forgot to give it a destructor
    ~GlobalH() noexcept {
        if (global_h)
            reset();
    }
    //GlobalAlloc()
    // construct with nullptr
    GlobalH(std::nullptr_t) noexcept : global_h{ pointer{} } {}

    // assign to nullptr
    GlobalH& operator=(std::nullptr_t) noexcept {
        reset();
        return (*this);
    } // end copy assignment

    GlobalH(_In_ UINT uFlags, _In_ SIZE_T dwBytes) noexcept :
        global_h{ ::GlobalAlloc(uFlags, dwBytes) } {}

    GlobalH(const GlobalH&) = delete;
    GlobalH& operator=(const GlobalH&) = delete;
    GlobalH(GlobalH&&) noexcept = default;
    GlobalH& operator=(GlobalH&&) noexcept = default;
    constexpr GlobalH() noexcept = default;


#pragma region General Methods
    _NODISCARD pointer get() const noexcept { return global_h.get(); }
    [[noreturn]] void reset(pointer p = pointer{}) noexcept { global_h.reset(p); }
    pointer release() noexcept { return global_h.release(); }
#pragma endregion

private:
    handle_type global_h;
};

namespace detail {

// This isn't all of them, just the ones I noticed
enum class NTKernelType { File, FileSearch, Mutex, Thread };



template<NTKernelType handle = NTKernelType{}>
struct NTKernelDeleter : public IDeleter<HANDLE>
{
    [[noreturn]] void operator()(_In_ pointer p) const noexcept override
    {
        switch (handle) {
            case NTKernelType::File:
                ::FindClose(p);
                break;
            case NTKernelType::FileSearch:
                ::CloseHandle(p);
                break;
            case NTKernelType::Mutex:
                ::ReleaseMutex(p);
                break;
            case NTKernelType::Thread:
            {
                if (auto myMutex{ ra::make_mutex(FALSE)
                    }; WaitForSingleObject(myMutex.get(), INFINITE) == WAIT_OBJECT_0)
                {
                    ::CloseHandle(p);
                }
            }
        }
    }
};

/// <summary>
///   A wrapper for pointers to kernel objects (<c>HANDLE</c>).
/// </summary>
template<NTKernelType handle = NTKernelType{}>
struct NTKernelH : public IHandle<HANDLE, NTKernelDeleter<handle>>
{
    using base_type    = IHandle<HANDLE, NTKernelDeleter<handle>>;
    using handle_type  = typename base_type::handle_type;
    using pointer      = typename base_type::pointer;
    using element_type = typename base_type::element_type;
    using deleter_type = typename base_type::deleter_type;

    // Let's make sure these pass
    static_assert(is_nullable_pointer_v<pointer>);
    static_assert(is_nullable_pointer_v<std::add_pointer_t<element_type>>);
    static_assert(is_function_object_v<deleter_type, pointer>);
    static_assert(is_unique_ptr_v<handle_type, pointer>);

    // We are using all of the base constructors
    using base_type::IHandle;
    // Params will be rearranged a little to make optional params actually optional

    template<class = std::enable_if_t<(handle == NTKernelType::Mutex)>>
    NTKernelH(_In_ BOOL bInitialOwner, _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes = nullptr,
        _In_opt_ LPCTSTR lpName = nullptr) :
        base_type{ ::CreateMutex(lpMutexAttributes, bInitialOwner, lpName) }
    {
        if (get() == INVALID_HANDLE_VALUE)
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::Thread)>>
    NTKernelH(
        _In_      SIZE_T dwStackSize,
        _In_      LPTHREAD_START_ROUTINE lpStartAddress,
        _In_      DWORD dwCreationFlags,
        _In_opt_  LPSECURITY_ATTRIBUTES lpThreadAttributes = nullptr,
        // this annotation is the most important one (can identify memory leaks during Native code analysis)
        _In_opt_ __deref __drv_aliasesMem LPVOID lpParameter = nullptr,
        _Out_opt_ LPDWORD lpThreadId = nullptr) :
        base_type{ ::CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags,
            lpThreadId) }
    {
        if (get() == INVALID_HANDLE_VALUE)
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::FileSearch)>>
    NTKernelH(_In_ LPCTSTR lpFileName, _Out_ LPWIN32_FIND_DATAA lpFindFileData) :
        base_type{ ::FindFirstFile(lpFileName, lpFindFileData) }
    {
        if (get() == INVALID_HANDLE_VALUE)
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::File)>>
    NTKernelH(_In_ LPCTSTR lpFileName, _In_ DWORD dwDesiredAccess, _In_ DWORD dwShareMode,
        _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
        _In_opt_ HANDLE hTemplateFile = nullptr) :
        base_type{ ::CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile) }
    {
        if (get() == INVALID_HANDLE_VALUE)
            ThrowLastError();
    }

    NTKernelH() noexcept = default;
};

} // namespace detail


using FileH       = detail::NTKernelH<detail::NTKernelType::File>;
using FileSearchH = detail::NTKernelH<detail::NTKernelType::FileSearch>;
using ThreadH     = detail::NTKernelH<detail::NTKernelType::Thread>;
using MutexH      = detail::NTKernelH<detail::NTKernelType::Mutex>;

#pragma region Kernel Resource Callbacks
_NODISCARD FileH CALLBACK make_file(
    _In_     LPCTSTR lpFileName,
    _In_     DWORD dwDesiredAccess,
    _In_     DWORD dwShareMode,
    _In_     DWORD dwCreationDisposition,
    _In_     DWORD dwFlagsAndAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
    _In_opt_ HANDLE hTemplateFile = nullptr) noexcept;

_NODISCARD FileSearchH CALLBACK make_find_file(
    _In_  LPCTSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAA lpFindFileData) noexcept;

_NODISCARD MutexH CALLBACK make_mutex(
    _In_     BOOL bInitialOwner,
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes = nullptr,
    _In_opt_ LPCTSTR lpName = nullptr) noexcept;

_NODISCARD ThreadH CALLBACK make_thread(
    _In_      SIZE_T dwStackSize,
    _In_      LPTHREAD_START_ROUTINE lpStartAddress,
    _In_      DWORD dwCreationFlags,
    _In_opt_  LPSECURITY_ATTRIBUTES lpThreadAttributes = nullptr,
    _In_opt_ __deref __drv_aliasesMem LPVOID lpParameter = nullptr,
    _Out_opt_ LPDWORD lpThreadId = nullptr) noexcept;
#pragma endregion


} // namespace ra


#endif // !RA_HANDLES_H
