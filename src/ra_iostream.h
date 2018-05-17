#ifndef RA_IOSTREAM_H
#define RA_IOSTREAM_H
#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

// copy and pasted from my test project

// based from here: http://en.cppreference.com/w/cpp/language/parameter_pack
// C++17 has fold expressions we can make it simpler later, just learning how to do this


namespace ra {

// we need this since '%' for us is a specifier, we need to make it a percent sign
template<
    typename CharT,
    class = std::enable_if_t<(std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>)>
>
auto percent(std::basic_ostream<CharT>& os) noexcept->std::basic_ostream<CharT>& {
    return os << "%";
}

// Aliases for people who don't want to remember what the right type is
// These stream objects will print "%" because the character is reserved for replacement tokens
inline constexpr auto spercent = percent<char>;
inline constexpr auto wpercent = percent<wchar_t>;


template<
    typename CharT,
    class = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
void StreamByCharType(const CharT* format) {
    // we are only considering char and wchar_t for now
    if (std::is_same_v<CharT, char>)
        std::cout << format;
    else if (std::is_same_v<CharT, wchar_t>)
        std::wcout << format;
    else
        throw std::invalid_argument{ "CharT needs to be char or wchar_t!" };
}



template<
    typename CharT,
    class = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
void printf(const CharT* format)
{

    try {
        StreamByCharType(format);
    }
    catch (const std::ios::failure& e) {
        std::cout << e.code() << ": " << e.what() << '\n';
    }
    catch (const std::invalid_argument& ia) {
        std::cout << ia.what() << '\n';
    }
    format = nullptr;
}

// An alias for ra::printf for those who prefer it, doesn't matter which one you use though.
// But for this it will matter, as the char_type has to be wchar_t
inline constexpr auto wprintf = printf<wchar_t>;

template<
    typename CharT,
    class = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
void printf_line(const CharT* format)
{
    std::basic_string<CharT> str{ format };
    auto end_line{ static_cast<CharT>('\n') };
    str += end_line;
    printf(str.c_str());
    format = nullptr;
}

inline constexpr auto wprintf_line = printf_line<wchar_t>;

template<typename CharT, typename ...Args>
void CheckNumFormatSpecs(const CharT*& format, Args&&... args);

template<
    typename CharT,
    typename ValueType,
    typename... Args,
    typename = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
void printf(const CharT* format, ValueType value, Args&&... args)
{
    CheckNumFormatSpecs(format, std::forward<Args>(args)...);


    auto nul{ static_cast<CharT>('\0') };
    auto spec{ static_cast<CharT>('%') };

    for (; *format != nul; format++)
    {
        if (*format == spec)
        {
            if (std::is_same_v<CharT, char>)
                std::cout << value;
            else if (std::is_same_v<CharT, wchar_t>)
                std::wcout << value;

            printf(format + 1, std::forward<Args>(args)...); // recursive call
            return;
        }

        if (std::is_same_v<CharT, char>)
            std::cout << value;
        else if (std::is_same_v<CharT, wchar_t>)
            std::wcout << value;
    }
    format = nullptr;
}

// I'm an idiot... It does work, but had an extra template argument

template<typename CharT, typename ...Args>
void CheckNumFormatSpecs(const CharT*& format, Args&&... args)
{
    std::basic_string<CharT> temp{ format };
    auto count{ 0 };


    // might make it a bit slower (not much) but should still check
    for (auto& i : temp)
    {
        if (i == static_cast<CharT>('%'))
            count++;
    }
    temp.clear();

#if _DEBUG
    assert(sizeof...(args) == (count - 1));
#else
    throw std::invalid_argument{ __FUNCTION__": There are more specifiers than arguments!" };
#endif // _DEBUG
}

template<
    typename CharT,
    typename ValueType,
    typename... Args,
    typename = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
void printf_line(const CharT* format, ValueType value, Args&&... args)
{
    std::basic_string<CharT> str{ format };
    auto end_line{ static_cast<CharT>('\n') };
    str += end_line;
    printf(str.c_str(), value, std::forward<Args>(args)...);
    format = nullptr;
}


template<
    typename CharT,
    class = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
_NODISCARD std::basic_string<CharT> tsprintf(const CharT* format) {
    return format;
}


inline constexpr auto wsprintf = tsprintf<wchar_t>;

template<
    typename CharT,
    typename ValueType,
    typename... Args,
    class = std::enable_if_t<std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>>
>
_NODISCARD std::basic_string<CharT> tsprintf(const CharT* format,
    ValueType value, Args&&... args)
{
    CheckNumFormatSpecs(format, std::forward<Args>(args)...);
    std::basic_ostringstream<CharT> oss;
    
    auto spec{ static_cast<CharT>('%') };
    auto nul{ static_cast<CharT>('\0') };

    for (; *format != nul; format++)
    {
        if (*format == spec)
        {
            oss << value << tsprintf(format + 1, std::forward<Args>(args)...); // recursive call
            return oss.str();
        }
        oss << *format;
    }

    format = nullptr;
    return oss.str();
}


// This will not work with fstream!
template<
    typename CharT,
    typename ValueType,
    typename... Args,
    typename OStreamType = std::basic_ostream<CharT>,
    class = std::enable_if_t<(std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>) &&
    (!std::is_same_v<OStreamType, std::basic_ofstream<CharT>>) &&
    (std::is_same_v<OStreamType, std::basic_ostream<CharT>> ||
        std::is_same_v<OStreamType, std::basic_ostringstream<CharT>>)>
>
void tsprintf(OStreamType& oss, const CharT* format, ValueType value, Args... args)
{
    CheckNumFormatSpecs(format, std::forward<Args>(args)...);
    auto spec{ static_cast<CharT>('%') };
    auto nul{ static_cast<CharT>('\0') };

    for (; *format != nul; format++)
    {
        if (*format == spec)
        {
            oss << value << tsprintf(format + 1, std::forward<Args>(args)...); // recursive call
            return;
        }
        oss << *format;
    }

    format = nullptr;
}

// Default precision will be 3 because that's the default precision of float
// Flags give expected behavior, in fixed decimal notation, by default it's scientific notation,
// Change the flags using the std::ios flags as needed
template<
    typename CharT = char,
    typename FloatT = float,
    class = std::enable_if_t<(std::_Is_character<CharT>::value || std::is_same_v<CharT, wchar_t>) &&
    std::is_floating_point_v<FloatT>>
    >
    double AdjustFloatField(FloatT fp, std::streamsize new_width = 1LL,
        std::streamsize new_precision = 3LL, int flags = std::ios::fixed | std::ios::dec)
{
    std::basic_ostringstream<CharT> oss;
    oss.flags(flags);
    oss.width(new_width);
    oss << std::setprecision(new_precision) << fp;


    return std::stod(oss.str());
}



// use Widen if you need a wide string (UTF-16)
// "i" must be an integral type, flags are for expected behavior, change them if you need something different
template<typename Integral, class = std::enable_if_t<std::is_integral_v<Integral>>>
std::string AdjustHexField(Integral i, std::streamsize new_width = 4,
    int flags = std::ios::uppercase | std::ios::hex)
{
    std::ostringstream oss;
    oss.flags(flags);

    oss << std::setfill('0') << std::setw(new_width) << i;
    return oss.str();
}

template<typename Integral, class = std::enable_if_t<std::is_integral_v<Integral>>>
std::string AdjustIntField(Integral i, std::streamsize new_width = 4,
    int flags = std::ios::dec)
{
    //temp.erase(0, 2);
    return AdjustHexField(i, new_width, flags);;
}



} // namespace ra

#endif // !RA_IOSTREAM
