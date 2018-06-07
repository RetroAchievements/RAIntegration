#ifndef RA_HANDLES_H
#define RA_HANDLES_H
#pragma once

#pragma warning(push, 1)
#pragma warning(disable : 4365 4514)
#include <memory>


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#include <WindowsX.h>

#include <ra_type_traits>
#include <ra_utility>
#include "ra_errors.h"
#pragma warning(pop)

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
    _NODISCARD explicit operator bool() const noexcept { return { this->get() != pointer{} }; }
    _NODISCARD explicit operator pointer() const noexcept { return this->get(); }
    _NODISCARD explicit operator std::nullptr_t() const noexcept
    {
        if (!this->get())
            return static_cast<std::nullptr_t>(this);
    }
#pragma endregion

#pragma region Operators
    _NODISCARD std::add_lvalue_reference_t<element_type> operator*() const { return { *this->get() }; }
    _NODISCARD pointer operator->() const noexcept { return { this->get() }; }
#pragma endregion

#pragma region Constructors
    // Visual Studio is tripping.... constructors aren't supposed to return anything, just assignments
#pragma warning (disable : 4508)
    virtual ~IHandle() noexcept
    {
        // Alright window handles seem like they can only be invalidated and not reset
        if (ihandle_)
            this->get_deleter()(this->get());
    } // end destructor


    /// <summary>
    ///   Initializes a new instance of the <see cref="IHandle" /> class with a
    ///   <c>nullptr</c>.
    /// </summary>
    /// <param name="">The <c>nullptr</c> constant.</param>
    IHandle(std::nullptr_t) noexcept : ihandle_{ pointer{} } {}

#pragma warning (default : 4508)
    /// <summary>
    /// Assigns <see cref="ihandle_"/> a <c>nullptr</c>.
    /// </summary>
    /// <param name="">The <c>nullptr</c> constant.</param>
    /// <returns></returns>
    IHandle& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    } // end copy assignment

    explicit IHandle(_In_ pointer p) noexcept : ihandle_{ p } {}


    IHandle(const IHandle&) = delete;
    IHandle& operator=(const IHandle&) = delete;
    IHandle(_In_ IHandle&&) noexcept = default;
    _NODISCARD IHandle& operator=(_In_ IHandle&&) noexcept = default;

#pragma endregion


    _NODISCARD auto get() const noexcept { return ihandle_.get(); }
    _NORETURN auto reset(pointer p = pointer{}) noexcept { ihandle_.reset(p); }
    // the pointer can be discarded if releasing an invalid pointer
    auto release() noexcept { return ihandle_.release(); } 
    _NORETURN auto swap(_Inout_ HandleType& b) noexcept { ihandle_.swap(b.ihandle_); }
    _NODISCARD auto& get_deleter() noexcept { return ihandle_.get_deleter(); }
    _NODISCARD const auto& get_deleter() const noexcept { return ihandle_.get_deleter(); }

    // Do not make this base constructor constexpr even though it can be.
    // It will delete the default constructors of derived classes otherwise
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
_NODISCARD _CONSTANT_FN
operator==(_In_ const IHandle<HandleType, DeleterType>& a, _In_ std::nullptr_t) noexcept
->decltype(!a) { return !a; }


template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
_NODISCARD _CONSTANT_FN
operator==(_In_ std::nullptr_t, _In_ const IHandle<HandleType, DeleterType>& b) noexcept
->decltype(!b) { return !b; }

template<
    typename HandleTypeA,
    typename DeleterTypeA,
    typename HandleTypeB,
    typename DeleterTypeB,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleTypeA> and is_function_object_v<DeleterTypeA>) and
    ((is_nullable_pointer_v<HandleTypeB> and is_function_object_v<DeleterTypeB>))>
>
_NODISCARD _CONSTANT_FN operator==(
    _In_ const IHandle<HandleTypeA, DeleterTypeA>& a,
    _In_ const IHandle<HandleTypeB, DeleterTypeB>& b)->decltype(a.get() == b.get())
{
    return { a.get() == b.get() };
}

template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
_NODISCARD _CONSTANT_FN
operator<(_In_ const IHandle<HandleType, DeleterType>& a, _In_ std::nullptr_t) noexcept
    ->decltype(std::less<typename decltype(a)::pointer>()(a.get(), nullptr))
{
    using pointer = typename decltype(a)::pointer;
    return std::less<pointer>()(a.get(), nullptr);
}

template<
    typename HandleType,
    typename DeleterType,
    typename = std::enable_if_t<(is_nullable_pointer_v<HandleType> and is_function_object_v<DeleterType>)>
>
_NODISCARD _CONSTANT_FN
operator<(_In_ std::nullptr_t, _In_ const IHandle<HandleType, DeleterType>& b) noexcept
    ->decltype(std::less<typename decltype(b)::pointer>()(nullptr, b.get()))
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
_NODISCARD _CONSTANT_FN operator<(
    _In_ const IHandle<HandleTypeA, DeleterTypeA>& a,
    _In_ const IHandle<HandleTypeB, DeleterTypeB>& b)
    ->decltype(std::less<std::common_type_t<typename decltype(a)::pointer,
        typename decltype(b)::pointer>>()(a.get(), b.get()))
{
    using pointer1    = typename decltype(a)::pointer;
    using pointer2    = typename decltype(b)::pointer;
    using common_type = std::common_type_t<pointer1, pointer2>;
    return std::less<common_type>()(a.get(), b.get());
}
#pragma endregion

template<typename HandleType, class = std::enable_if_t<is_nullable_pointer_v<HandleType>>>
struct IDeleter
{
    typedef HandleType pointer;
    inline constexpr IDeleter() noexcept = default;
    virtual ~IDeleter() noexcept = default;
    inline constexpr IDeleter(const IDeleter&) noexcept = delete;
    inline constexpr IDeleter& operator=(const IDeleter&) noexcept = delete;
    // Deleters need to be forwarded, so they still need to be movable
    inline constexpr IDeleter(IDeleter&&) noexcept = default;
    inline constexpr IDeleter& operator=(IDeleter&&) noexcept = default;

    _NORETURN virtual void operator()(_In_ pointer p) const noexcept = 0;
};

} // namespace detail

namespace detail {

// This isn't all of them, just the ones we noticed, there's also pipes,
// semaphores, events, but didn't see them
enum class NTKernelType { File, FileSearch, Mutex, Thread };

template<NTKernelType handle = NTKernelType{}>
struct NTKernelDeleter final : public IDeleter<HANDLE>
{
    using IDeleter<HANDLE>::IDeleter;
    _NORETURN inline void operator()(_In_ pointer p) const noexcept
    {
        switch (handle)
        {
            case NTKernelType::File:
                ::CloseHandle(p);
                break;

            case NTKernelType::FileSearch:
                ::FindClose(p);
                break;

            case NTKernelType::Mutex:
                ::ReleaseMutex(p);
                break;

            case NTKernelType::Thread:
            {
                // This might need a mutex, this one cannot be static or it will cause a deadlock
                if (auto tmp{ make_mutex(FALSE)
                    }; WaitForSingleObject(tmp.get(), INFINITE) == WAIT_OBJECT_0)
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
struct NTKernelH final : public IHandle<HANDLE, NTKernelDeleter<handle>>
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

    _NODISCARD explicit operator base_type() const noexcept { return dynamic_cast<base_type*>(this); }



    // Params will be rearranged a little to make optional params actually optional
    template<class = std::enable_if_t<(handle == NTKernelType::Mutex)>>
    NTKernelH(
        _In_     BOOL bInitialOwner,
        _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes = nullptr,
        _In_opt_ LPCTSTR lpName                          = nullptr)
    {
        if (auto myPtr{ ::CreateMutex(lpMutexAttributes, bInitialOwner, lpName) }; myPtr)
            this->ihandle_ = decltype(ihandle_){myPtr};
        else
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::Thread)>>
    NTKernelH(
        _In_     SIZE_T dwStackSize,
        _In_     LPTHREAD_START_ROUTINE lpStartAddress,
        _In_     DWORD dwCreationFlags,
        _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes   = nullptr,
        // this annotation is the most important one (can identify memory leaks during Native code analysis)
        // We can't use it in the callback though
        _In_opt_ __deref __drv_aliasesMem LPVOID lpParameter = nullptr,
        _Out_opt_ LPDWORD lpThreadId                         = nullptr)
    {
        if (auto myPtr{ ::CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter,
            dwCreationFlags, lpThreadId) }; myPtr)
        {
            this->ihandle_ = decltype(ihandle_){myPtr};
        }
        else
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::FileSearch)>>
    NTKernelH(
        _In_  LPCTSTR lpFileName,
        _Out_ LPWIN32_FIND_DATA lpFindFileData) :
        base_type{ ::FindFirstFile(lpFileName, lpFindFileData) }
    {
        if (not ValidHandle(get()))
            ThrowLastError();
    }

    template<class = std::enable_if_t<(handle == NTKernelType::File)>>
    NTKernelH(
        _In_ LPCTSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
        _In_opt_ HANDLE hTemplateFile                       = nullptr) :
        base_type{ ::CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile) }
    {
        if (not ValidHandle(get()))
            ThrowLastError();
    }

    // We at least need to define the default constructor because of the extra template
    // The rest are automatically handled
    NTKernelH() noexcept = default;
};

// Inheritors with more templates need operator overloading
#pragma region NTKernelH Comparison Operators
template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator==(_In_ const NTKernelH<handle>& a, _In_ std::nullptr_t) noexcept->decltype(!a) { return !a; }

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator==(_In_ std::nullptr_t, _In_ const NTKernelH<handle>& b) noexcept->decltype(!b) { return !b; }

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator!=(_In_ const NTKernelH<handle>& a, _In_ std::nullptr_t) noexcept
->decltype(!(a == nullptr)) {
    return !(a == nullptr);
}

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator!=(_In_ std::nullptr_t, _In_ const NTKernelH<handle>& b) noexcept
->decltype(!(nullptr == b)) {
    return !(nullptr == b);
}

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator==(_In_ const NTKernelH<handle>& a, _In_ const NTKernelH<handle>& b)
->decltype(a.get() == b.get()) {
    return { a.get() == b.get() };
}

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator<(_In_ const NTKernelH<handle>& a, _In_ std::nullptr_t) noexcept
    ->decltype(std::less<typename decltype(a)::pointer>()(a.get(), nullptr))
{
    using pointer = typename decltype(a)::pointer;
    return std::less<pointer>()(a.get(), nullptr);
}

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator<(_In_ std::nullptr_t, _In_ const NTKernelH<handle>& b) noexcept
    ->decltype(std::less<typename decltype(b)::pointer>()(nullptr, b.get()))
{
    using pointer = typename decltype(b)::pointer;
    return std::less<pointer>()(nullptr, b.get());
}

template<NTKernelType handle = NTKernelType{}> _NODISCARD _CONSTANT_VAR
operator<(_In_ const NTKernelH<handle>& a, _In_ const NTKernelH<handle>& b)
    ->decltype(std::less<std::common_type_t<typename decltype(a)::pointer,
        typename decltype(b)::pointer>>()(a.get(), b.get()))
{
    using pointer1    = typename decltype(a)::pointer;
    using pointer2    = typename decltype(b)::pointer;
    using common_type = std::common_type_t<pointer1, pointer2>;
    return std::less<common_type>()(a.get(), b.get());
}
#pragma endregion



} // namespace detail

using FileH       = detail::NTKernelH<detail::NTKernelType::File>;
using FileSearchH = detail::NTKernelH<detail::NTKernelType::FileSearch>;
using MutexH      = detail::NTKernelH<detail::NTKernelType::Mutex>;
using ThreadH     = detail::NTKernelH<detail::NTKernelType::Thread>;

// Visual Studio is tripping, it will say the functions aren't defined even
// though they are, and are still getting called. Go to definition/declaration
// still work too. It doesn't even show up in the error box.
#pragma region Kernel Resource Callbacks
_NODISCARD FileH CALLBACK make_file(
    _In_     LPCTSTR lpFileName,
    _In_     DWORD dwDesiredAccess,
    _In_     DWORD dwShareMode,
    _In_     DWORD dwCreationDisposition,
    _In_     DWORD dwFlagsAndAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
    _In_opt_ HANDLE hTemplateFile                       = nullptr) noexcept;

_NODISCARD FileSearchH CALLBACK make_find_file(
    _In_  LPCTSTR lpFileName,
    _Out_ LPWIN32_FIND_DATA lpFindFileData) noexcept;

_NODISCARD MutexH CALLBACK make_mutex(
    _In_     BOOL bInitialOwner,
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes = nullptr,
    _In_opt_ LPCTSTR lpName                          = nullptr) noexcept;

_NODISCARD ThreadH CALLBACK make_thread(
    _In_      SIZE_T dwStackSize,
    _In_      LPTHREAD_START_ROUTINE lpStartAddress,
    _In_      DWORD dwCreationFlags,
    _In_opt_  LPSECURITY_ATTRIBUTES lpThreadAttributes = nullptr,
    _In_opt_  LPVOID lpParameter                       = nullptr,
    _Out_opt_ LPDWORD lpThreadId                       = nullptr) noexcept;
#pragma endregion

} // namespace ra

#endif // !RA_HANDLES_H
