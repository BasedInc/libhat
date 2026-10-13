#pragma once

#ifndef LIBHAT_MODULE
    #include <concepts>
    #include <format>
    #include <type_traits>
#endif

#include "export.hpp"

LIBHAT_EXPORT namespace hat {

    template<typename T, typename CharT, template<typename...> class Formatter>
    struct formatter {
        formatter() = delete;
        formatter(const formatter&) = delete;
        formatter& operator=(const formatter&) = delete;
    };

    template<typename T, typename CharT, template<typename...> class Formatter>
    concept formattable = std::semiregular<formatter<std::remove_reference_t<T>, CharT, Formatter>>;

    template<typename T>
    constexpr bool disable_range_formatter = false;
}

template<typename T, typename CharT>
    requires(hat::formattable<T, CharT, std::formatter>)
struct std::formatter<T, CharT> : hat::formatter<T, CharT, std::formatter> {};

#if __cpp_lib_format_ranges >= 202207L
template<typename T>
    requires(hat::disable_range_formatter<T>)
constexpr auto std::format_kind<T> = std::range_format::disabled;
#endif
