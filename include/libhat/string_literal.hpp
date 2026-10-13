#pragma once

#include "export.hpp"
#include "fixed_string.hpp"
#include "formatter.hpp"

LIBHAT_EXPORT namespace hat {

    template<auto str>
    struct basic_string_literal {
        static constexpr auto storage = str;
    };

    template<hat::fixed_string str>
    using string_literal = basic_string_literal<str>;

    template<hat::wfixed_string str>
    using wstring_literal = basic_string_literal<str>;

#ifdef __cpp_lib_char8_t
    template<hat::u8fixed_string str>
    using u8string_literal = basic_string_literal<str>;
#endif

    template<hat::u16fixed_string str>
    using u16string_literal = basic_string_literal<str>;

    template<hat::u32fixed_string str>
    using u32string_literal = basic_string_literal<str>;

    template<auto str, template<typename...> class Formatter>
    struct formatter<basic_string_literal<str>, typename decltype(str)::value_type, Formatter> : Formatter<decltype(str), typename decltype(str)::value_type> {
        template<typename FormatContext>
        constexpr auto format(const string_literal<str>&, FormatContext& ctx) const {
            return Formatter<decltype(str), typename decltype(str)::value_type>::format(str, ctx);
        }
    };
}

LIBHAT_EXPORT namespace hat::inline literals::inline string_literals {

    template<hat::fixed_string str>
    consteval auto operator""_s() noexcept {
        return string_literal<str>();
    }

    template<hat::wfixed_string str>
    consteval auto operator""_ws() noexcept {
        return wstring_literal<str>();
    }

#ifdef __cpp_lib_char8_t
    template<hat::u8fixed_string str>
    consteval auto operator""_u8s() noexcept {
        return u8string_literal<str>();
    }
#endif

    template<hat::u16fixed_string str>
    consteval auto operator""_u16s() noexcept {
        return u16string_literal<str>();
    }

    template<hat::u32fixed_string str>
    consteval auto operator""_u32s() noexcept {
        return u32string_literal<str>();
    }
}
