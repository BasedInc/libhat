#pragma once

#include "FixedString.hpp"

namespace hat {

    template<auto str>
    struct basic_string_literal {
        static constexpr auto storage = str;
    };

    template<hat::fixed_string str>
    using string_literal = basic_string_literal<str>;

    template<hat::wfixed_string str>
    using wstring_literal = basic_string_literal<str>;

    template<hat::u8fixed_string str>
    using u8string_literal = basic_string_literal<str>;

    template<hat::u16fixed_string str>
    using u16string_literal = basic_string_literal<str>;

    template<hat::u32fixed_string str>
    using u32string_literal = basic_string_literal<str>;
}

namespace hat::literals::string_literals {

    template<hat::fixed_string str>
    consteval auto operator""_s() noexcept {
        return string_literal<str>();
    }

    template<hat::wfixed_string str>
    consteval auto operator""_ws() noexcept {
        return wstring_literal<str>();
    }

    template<hat::u8fixed_string str>
    consteval auto operator""_u8s() noexcept {
        return u8string_literal<str>();
    }

    template<hat::u16fixed_string str>
    consteval auto operator""_u16s() noexcept {
        return u16string_literal<str>();
    }

    template<hat::u32fixed_string str>
    consteval auto operator""_u32s() noexcept {
        return u32string_literal<str>();
    }
}