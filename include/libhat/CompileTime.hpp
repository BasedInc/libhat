#pragma once

#include <algorithm>
#include <string>

#include "Concepts.hpp"
#include "Result.hpp"

namespace hat {

    template<typename Char, size_t N, template<size_t> typename Derived>
    struct basic_string_literal {
        using const_reference   = const Char&;
        using const_pointer     = const Char*;
        using const_iterator    = const_pointer;

        static constexpr auto npos = static_cast<size_t>(-1);

        constexpr basic_string_literal(std::basic_string_view<Char> str) {
            std::copy_n(str.data(), N, value);
        }

        constexpr basic_string_literal(const Char (&str)[N + 1]) {
            std::copy_n(str, N, value);
        }

        [[nodiscard]] constexpr const_iterator begin() const {
            return this->c_str();
        }

        [[nodiscard]] constexpr const_iterator end() const {
            return this->begin() + this->size();
        }

        [[nodiscard]] constexpr const_reference operator[](size_t pos) const {
            return this->value[pos];
        }

        [[nodiscard]] constexpr const_reference at(size_t pos) const {
            return this->value[pos];
        }

        [[nodiscard]] constexpr const_reference front() const {
            return this->value[0];
        }

        [[nodiscard]] constexpr const_reference back() const {
            return this->value[size() - 1];
        }

        [[nodiscard]] constexpr const_pointer c_str() const {
            return static_cast<const Char*>(this->value);
        }

        [[nodiscard]] constexpr const_pointer data() const {
            return this->c_str();
        }

        [[nodiscard]] constexpr size_t size() const {
            return N;
        }

        [[nodiscard]] constexpr bool empty() const {
            return this->size() == 0;
        }

        template<
            size_t Pos = 0,
            size_t Count = npos,
            size_t M = (Count == npos ? N - Pos : Count)
        >
        [[nodiscard]] constexpr auto substr() const -> Derived<M> {
            static_assert(Pos + M <= N);
            Char buf[M + 1]{};
            std::copy_n(this->value + Pos, M, buf);
            return buf;
        }

        template<size_t M, size_t K = N + M>
        constexpr auto operator+(const basic_string_literal<Char, M, Derived>& str) const -> Derived<K> {
            Char buf[K + 1]{};
            std::copy_n(this->value, this->size(), buf);
            std::copy_n(str.value, str.size(), buf + this->size());
            return buf;
        }

        template<size_t M>
        constexpr auto operator+(const Char (&str)[M]) const {
            return *this + Derived<M - 1>{str};
        }

        template<size_t M>
        constexpr bool operator==(const basic_string_literal<Char, M, Derived>& str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(std::basic_string<Char> str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(std::basic_string_view<Char> str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(const Char* str) const {
            return std::equal(this->begin(), this->end(), str, str + std::char_traits<Char>::length(str));
        }

        constexpr std::basic_string<Char> str() const {
            return {this->begin(), this->end()};
        }

        constexpr std::basic_string_view<Char> to_view() const {
            return {this->begin(), this->end()};
        }

        Char value[N + 1]{};
    };

    #define LIBHAT_DEFINE_STRING_LITERAL(name, type)                                                            \
    template<size_t N>                                                                                          \
    struct name : public basic_string_literal<type, N, name> {                                                  \
        using basic_string_literal<type, N, name>::basic_string_literal;                                        \
    };                                                                                                          \
    template<size_t N>                                                                                          \
    name(const type(&str)[N]) -> name<N - 1>;                                                                   \
    template<size_t N, size_t M>                                                                                \
    constexpr inline auto operator+(const type (&cstr)[N], const basic_string_literal<type, M, name>& lstr) {   \
        return name{cstr} + lstr;                                                                               \
    }

    LIBHAT_DEFINE_STRING_LITERAL(string_literal,    char)
    LIBHAT_DEFINE_STRING_LITERAL(wstring_literal,   wchar_t)
    LIBHAT_DEFINE_STRING_LITERAL(u8string_literal,  char8_t)
    LIBHAT_DEFINE_STRING_LITERAL(u16string_literal, char16_t)
    LIBHAT_DEFINE_STRING_LITERAL(u32string_literal, char32_t)

    #undef LIBHAT_DEFINE_STRING_LITERAL

    enum class parse_int_error {
        invalid_base,
        illegal_char
    };

    template<std::integral Integer, detail::char_iterator Iter>
    inline constexpr result<Integer, parse_int_error> parse_int(Iter begin, Iter end, int base = 10) noexcept {
        if (base < 2 || base > 36) {
            return result_error{parse_int_error::invalid_base};
        }

        Integer sign = 1;
        Integer value = 0;
        auto digits = base < 10 ? base : 10;
        auto letters = base > 10 ? base - 10 : 0;

        for (auto iter = begin; iter != end; iter++) {
            char ch = *iter;

            if constexpr (std::is_signed_v<Integer>) {
                if (iter == begin) {
                    if (ch == '+') {
                        continue;
                    } else if (ch == '-') {
                        sign = -1;
                        continue;
                    }
                }
            }

            value *= base;
            if (ch >= '0' && ch < '0' + digits) {
                value += (ch - '0');
            } else if (ch >= 'A' && ch < 'A' + letters) {
                value += (ch - 'A' + 10);
            } else if (ch >= 'a' && ch < 'a' + letters) {
                value += (ch - 'a' + 10);
            } else {
                // Throws an exception at runtime AND prevents constexpr evaluation
                return result_error{parse_int_error::illegal_char};
            }
        }
        return sign * value;
    }

    template<typename Integer>
    inline constexpr result<Integer, parse_int_error> parse_int(std::string_view str, int base = 10) noexcept {
        return parse_int<Integer>(str.cbegin(), str.cend(), base);
    }
}
